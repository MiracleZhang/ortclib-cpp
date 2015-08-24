/*

 Copyright (c) 2015, Hookflash Inc. / Hookflash Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 The views and conclusions contained in the software and documentation are those
 of the authors and should not be interpreted as representing official policies,
 either expressed or implied, of the FreeBSD Project.
 
 */

#include <ortc/internal/ortc_SCTPTransport.h>
#include <ortc/internal/ortc_DTLSTransport.h>
#include <ortc/internal/ortc_DataChannel.h>
#include <ortc/internal/ortc_ICETransport.h>
#include <ortc/internal/ortc_ORTC.h>
#include <ortc/internal/platform.h>

#include <openpeer/services/ISettings.h>
#include <openpeer/services/IHelper.h>
#include <openpeer/services/IHTTP.h>

#include <zsLib/Stringize.h>
#include <zsLib/Log.h>
#include <zsLib/XML.h>

#include <cryptopp/sha.h>

#include <usrsctp.h>

//#include <netinet/sctp_os.h>
#ifdef _WIN32
# include <winsock2.h>
#else
# include <sys/socket.h>
#endif  //_WIN32

#ifdef _DEBUG
#define ASSERT(x) ZS_THROW_BAD_STATE_IF(!(x))
#else
#define ASSERT(x)
#endif //_DEBUG


// The expression ARRAY_SIZE(a) is a compile-time constant of type
// size_t which represents the number of elements of the given
// array. You should only use ARRAY_SIZE on statically allocated
// arrays.

#define ARRAY_SIZE(a)                               \
  ((sizeof(a) / sizeof(*(a))) /                     \
  static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

namespace ortc { ZS_DECLARE_SUBSYSTEM(ortclib) }

namespace ortc
{
  ZS_DECLARE_TYPEDEF_PTR(openpeer::services::ISettings, UseSettings)
  ZS_DECLARE_TYPEDEF_PTR(openpeer::services::IHelper, UseServicesHelper)
  ZS_DECLARE_TYPEDEF_PTR(openpeer::services::IHTTP, UseHTTP)

  typedef openpeer::services::Hasher<CryptoPP::SHA1> SHA1Hasher;

  ZS_DECLARE_INTERACTION_TEAR_AWAY(ISCTPTransport, internal::SCTPTransport::TearAwayData)

  namespace internal
  {
    struct SCTPHelper;

    ZS_DECLARE_TYPEDEF_PTR(SCTPHelper, UseSCTPHelper)

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark (helpers)
    #pragma mark

    const uint32_t kMaxSctpSid = 1023;
    static const size_t kSctpMtu = 1200;

    enum PreservedErrno {
      SCTP_EINPROGRESS = EINPROGRESS,
      SCTP_EWOULDBLOCK = EWOULDBLOCK
    };

    //-------------------------------------------------------------------------
    const char *toString(SCTPPayloadProtocolIdentifier ppid)
    {
      switch (ppid) {
        case SCTP_PPID_NONE:           return "NONE";
        case SCTP_PPID_CONTROL:        return "CONTROL";
        case SCTP_PPID_BINARY_PARTIAL: return "BINARY_PARTIAL";
        case SCTP_PPID_BINARY_LAST:    return "BINARY_LAST";
        case SCTP_PPID_TEXT_PARTIAL:   return "TEXT_PARTIAL";
        case SCTP_PPID_TEXT_LAST:      return "TEXT_LAST";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark SCTPPacket
    #pragma mark

    //---------------------------------------------------------------------------
    ElementPtr SCTPPacket::toDebug() const
    {
      ElementPtr resultEl = Element::create("ortc::SCTPPacket");

      UseServicesHelper::debugAppend(resultEl, "type", toString(mType));
      UseServicesHelper::debugAppend(resultEl, "tuple id", mTupleID);
      UseServicesHelper::debugAppend(resultEl, "sessiond id", mSessionID);
      UseServicesHelper::debugAppend(resultEl, "sequence number", mSequenceNumber);
      UseServicesHelper::debugAppend(resultEl, "timestamp", mTimestamp);
      UseServicesHelper::debugAppend(resultEl, "flags", mFlags);
      UseServicesHelper::debugAppend(resultEl, "buffer", mBuffer ? mBuffer->SizeInBytes() : 0);

      return resultEl;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark SCTPHelper
    #pragma mark

    struct SCTPHelper
    {
      enum Directions
      {
        Direction_Incoming,
        Direction_Outgoing,
      };

      //-------------------------------------------------------------------------
      static DWORD createTuple(
                               WORD localPort,
                               WORD remotePort
                               )
      {
        DWORD result {};

        WORD *pLocal = &(((WORD *)(&result))[0]);
        WORD *pRemote = &(((WORD *)(&result))[1]);

        memcpy(pLocal, &localPort, sizeof(localPort));
        memcpy(pRemote, &remotePort, sizeof(remotePort));

        return result;
      }

      //-------------------------------------------------------------------------
      static DWORD getLocalRemoteTuple(
                                       const BYTE *packet,
                                       size_t bufferLengthInBytes,
                                       Directions direction,
                                       WORD *outLocalPort = NULL,
                                       WORD *outRemotePort = NULL
                                       )
      {
        if (outLocalPort) *outLocalPort = 0;
        if (outRemotePort) *outRemotePort = 0;
        if (bufferLengthInBytes < sizeof(DWORD)) {
          ZS_LOG_WARNING(Trace, slog("SCTP packet is too small to be valid") + ZS_PARAM("buffer length", bufferLengthInBytes))
          return 0;
        }

        WORD *pSourcePort = &(((WORD *)packet)[0]);
        WORD *pDestPort = &(((WORD *)packet)[1]);

        WORD sourcePort = 0;
        WORD destPort = 0;

        // perform memcpy to extract data (as not all processors like accessing
        // buffers at non-32 byte boundaries)
        memcpy(&sourcePort, pSourcePort, sizeof(sourcePort));
        memcpy(&destPort, pDestPort, sizeof(destPort));

        sourcePort = ntohs(sourcePort);
        destPort = ntohs(destPort);

        WORD localPort {};
        WORD remotePort {};

        switch (direction) {
          case Direction_Incoming:
          {
            localPort = destPort;
            remotePort = sourcePort;
            break;
          }
          case Direction_Outgoing:
          {
            localPort = sourcePort;
            remotePort = destPort;
            break;
          }
        }

        if (outLocalPort) {
          memcpy(outLocalPort, &localPort, sizeof(localPort));
        }
        if (outRemotePort) {
          memcpy(outRemotePort, &remotePort, sizeof(remotePort));
        }

        return createTuple(localPort, remotePort);
      }

      //-------------------------------------------------------------------------
      static Log::Params slog(const char *message)
      {
        ElementPtr objectEl = Element::create("ortc::SCTPHelper");
        return Log::Params(message, objectEl);
      }

    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark SCTPInit
    #pragma mark

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark SCTPInit
    #pragma mark

    // code borrowed from:
    // https://chromium.googlesource.com/external/webrtc/+/master/talk/media/sctp/sctpdataengine.cc
    // https://chromium.googlesource.com/external/webrtc/+/master/talk/media/sctp/sctpdataengine.h

    class SCTPInit : public ISingletonManagerDelegate
    {
    public:
      friend class SCTPTransport;

    protected:
      struct make_private {};

    public:
      ZS_DECLARE_TYPEDEF_PTR(IDataChannelForSCTPTransport, UseDataChannel)

    public:
      //-----------------------------------------------------------------------
      SCTPInit(const make_private &)
      {
        ZS_LOG_BASIC(log("created"))
      }

    protected:
      //-----------------------------------------------------------------------
      void init()
      {
        AutoRecursiveLock lock(mLock);

        // First argument is udp_encapsulation_port, which is not releveant for our
        // AF_CONN use of sctp.
        usrsctp_init(0, OnSctpOutboundPacket, debug_sctp_printf);

        // To turn on/off detailed SCTP debugging. You will also need to have the
        // SCTP_DEBUG cpp defines flag.
        // usrsctp_sysctl_set_sctp_debug_on(SCTP_DEBUG_ALL);
        // TODO(ldixon): Consider turning this on/off.
        usrsctp_sysctl_set_sctp_ecn_enable(0);
        // TODO(ldixon): Consider turning this on/off.
        // This is not needed right now (we don't do dynamic address changes):
        // If SCTP Auto-ASCONF is enabled, the peer is informed automatically
        // when a new address is added or removed. This feature is enabled by
        // default.
        // usrsctp_sysctl_set_sctp_auto_asconf(0);
        // TODO(ldixon): Consider turning this on/off.
        // Add a blackhole sysctl. Setting it to 1 results in no ABORTs
        // being sent in response to INITs, setting it to 2 results
        // in no ABORTs being sent for received OOTB packets.
        // This is similar to the TCP sysctl.
        //
        // See: http://lakerest.net/pipermail/sctp-coders/2012-January/009438.html
        // See: http://svnweb.freebsd.org/base?view=revision&revision=229805
        // usrsctp_sysctl_set_sctp_blackhole(2);
        // Set the number of default outgoing streams.  This is the number we'll
        // send in the SCTP INIT message.  The 'appropriate default' in the
        // second paragraph of
        // http://tools.ietf.org/html/draft-ietf-rtcweb-data-channel-05#section-6.2
        // is cricket::kMaxSctpSid.
        usrsctp_sysctl_set_sctp_nr_outgoing_streams_default(static_cast<uint32_t>(UseSettings::getUInt(ORTC_SETTING_SCTP_TRANSPORT_MAX_SESSIONS_PER_PORT)));

        mInitialized = true;
      }

      //-----------------------------------------------------------------------
      static SCTPInitPtr create()
      {
        SCTPInitPtr pThis(make_shared<SCTPInit>(make_private{}));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

    public:
      //-----------------------------------------------------------------------
      ~SCTPInit()
      {
        mThisWeak.reset();
        ZS_LOG_BASIC(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      static SCTPInitPtr singleton()
      {
        static SingletonLazySharedPtr<SCTPInit> singleton(create());
        SCTPInitPtr result = singleton.singleton();

        static zsLib::SingletonManager::Register registerSingleton("openpeer::ortc::SCTPInit", result);

        if (!result) {
          ZS_LOG_WARNING(Detail, slog("singleton gone"))
        }
        
        return result;
      }

    protected:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark SCTPInit => ISingletonManagerDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      virtual void notifySingletonCleanup() override
      {
        cancel();
      }

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark SCTPInit => usrscpt callbacks
      #pragma mark

      //-----------------------------------------------------------------------
      // This is the callback usrsctp uses when there's data to send on the network
      // that has been wrapped appropriatly for the SCTP protocol.
      static int OnSctpOutboundPacket(
                                      void* addr,
                                      void* data,
                                      size_t length,
                                      uint8_t tos,
                                      uint8_t set_df
                                      )
      {
        ZS_DECLARE_TYPEDEF_PTR(SCTPTransport::SocketInfo, SocketInfo)

        SocketInfoPtr socketInfo = *(static_cast<SocketInfoPtr *>(addr));

        ZS_LOG_TRACE(slog("on sctp output backpet") + ZS_PARAM("address", ((PTRNUMBER)addr)) + ZS_PARAM("length", length) + ZS_PARAM("tos", tos) + ZS_PARAM("set_df", set_df))

        if (ZS_IS_LOGGING(Insane)) {
          String str = UseServicesHelper::getDebugString((const BYTE *)data, length);
          ZS_LOG_INSANE(slog("sctp outgoing packet") + ZS_PARAM("raw", "\n" + str))
        }

        auto transport = socketInfo->mTransport.lock();

        if (!transport) {
          ZS_LOG_WARNING(Trace, slog("transport is gone (thus cannot send packet)") + ZS_PARAM("address", ((PTRNUMBER)addr)) + ZS_PARAM("length", length) + ZS_PARAM("tos", tos) + ZS_PARAM("set_df", set_df))
#define FIX_RETURN_RESULT 1
#define FIX_RETURN_RESULT 2
          return 0;
        }

        bool result = transport->notifySendSCTPPacket((const BYTE *)data, length);
        if (!result) {
          ZS_LOG_WARNING(Trace, slog("transport is unable to send packet") + ZS_PARAM("address", ((PTRNUMBER)addr)) + ZS_PARAM("length", length) + ZS_PARAM("tos", tos) + ZS_PARAM("set_df", set_df))
#define FIX_RETURN_RESULT 1
#define FIX_RETURN_RESULT 2
          return 0;
        }

        return 0;
      }

      //-----------------------------------------------------------------------
      static void debug_sctp_printf(const char *format, ...)
      {
        char s[1024] {};

        va_list ap;
        va_start(ap, format);
#ifdef _WIN32
        vsnprintf_s(s, sizeof(s), format, ap);
#else
        vsnprintf(s, sizeof(s), format, ap);
#endif  //_WIN32

        ZS_LOG_BASIC(slog("debug") + ZS_PARAM("message", s))
        va_end(ap);
      }

      //-------------------------------------------------------------------------
      static int OnSctpInboundPacket(
                                     struct socket* sock,
                                     union sctp_sockstore addr,
                                     void* data,
                                     size_t length,
                                     struct sctp_rcvinfo rcv,
                                     int flags,
                                     void* ulp_info
                                     )
      {
        ZS_DECLARE_TYPEDEF_PTR(SCTPTransport::SocketInfo, SocketInfo)
        SocketInfoPtr socketInfo = *(static_cast<SocketInfoPtr *>(ulp_info));

        const SCTPPayloadProtocolIdentifier ppid = static_cast<SCTPPayloadProtocolIdentifier>(ntohl(rcv.rcv_ppid));

        switch (ppid) {
          case SCTP_PPID_CONTROL:
          case SCTP_PPID_BINARY_PARTIAL:
          case SCTP_PPID_BINARY_LAST:
          case SCTP_PPID_TEXT_PARTIAL:
          case SCTP_PPID_TEXT_LAST:
          {
            break;
          }
          case SCTP_PPID_NONE:
          default: {
            ZS_LOG_WARNING(Trace, slog("incoming protocol identifier type was not understood (dropping packet)") + ZS_PARAM("ppid", ppid))
            break;
          }
        }

        SCTPPacketPtr packet(make_shared<SCTPPacket>());

        packet->mTupleID = socketInfo->mTupleID;
        packet->mType = ppid;
        packet->mSessionID = rcv.rcv_sid;
        packet->mSequenceNumber = rcv.rcv_ssn;
        packet->mTimestamp = rcv.rcv_tsn;
        packet->mFlags = flags;
        packet->mBuffer = UseServicesHelper::convertToBuffer((const BYTE *)data, length);

        auto transport = socketInfo->mTransport.lock();

        if (!transport) {
          ZS_LOG_WARNING(Trace, slog("transport is gone (thus cannot receive packet)") + ZS_PARAM("socket", ((PTRNUMBER)sock)) + ZS_PARAM("length", length) + ZS_PARAM("flags", flags) + ZS_PARAM("ulp", ((PTRNUMBER)ulp_info)))
#define FIX_RETURN_RESULT 1
#define FIX_RETURN_RESULT 2
          return 0;
        }

        bool result = transport->notifySendSCTPPacket((const BYTE *)data, length);
        if (!result) {
          ZS_LOG_WARNING(Trace, slog("transport is unable to receive packet") + ZS_PARAM("socket", ((PTRNUMBER)sock)) + ZS_PARAM("length", length) + ZS_PARAM("flags", flags) + ZS_PARAM("ulp", ((PTRNUMBER)ulp_info)))
#define FIX_RETURN_RESULT 1
#define FIX_RETURN_RESULT 2
          return 0;
        }

        ISCTPTransportAsyncDelegateProxy::create(transport)->onIncomingPacket(packet);
        return 1;
      }
      
    protected:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark SCTPInit => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      Log::Params log(const char *message) const
      {
        ElementPtr objectEl = Element::create("ortc::SCTPInit");
        UseServicesHelper::debugAppend(objectEl, "id", mID);
        return Log::Params(message, objectEl);
      }

      //-----------------------------------------------------------------------
      static Log::Params slog(const char *message)
      {
        return Log::Params(message, "ortc::SCTPInit");
      }

      //-----------------------------------------------------------------------
      Log::Params debug(const char *message) const
      {
        return Log::Params(message, toDebug());
      }

      //-----------------------------------------------------------------------
      virtual ElementPtr toDebug() const
      {
        AutoRecursiveLock lock(mLock);
        ElementPtr resultEl = Element::create("ortc::SCTPInit");

        UseServicesHelper::debugAppend(resultEl, "id", mID);

        return resultEl;
      }

      //-----------------------------------------------------------------------
      void cancel()
      {
        ZS_LOG_DEBUG(log("cancel called"))

        bool initialized = mInitialized.exchange(false);
        if (!initialized) return;

        int count = 0;

        while ((0 != usrsctp_finish()) &&
               (count < 300))
        {
          std::this_thread::sleep_for(Milliseconds(10));
          ++count;
        }
      }

    protected:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark SCTPInit => (data)
      #pragma mark

      AutoPUID mID;
      mutable RecursiveLock mLock;
      SCTPInitWeakPtr mThisWeak;

      std::atomic<bool> mInitialized{ false };
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ISCTPTransportForSettings
    #pragma mark

    //-------------------------------------------------------------------------
    void ISCTPTransportForSettings::applyDefaults()
    {
      //https://tools.ietf.org/html/draft-ietf-rtcweb-data-channel-13#section-6.6
      UseSettings::setUInt(ORTC_SETTING_SCTP_TRANSPORT_MAX_MESSAGE_SIZE, 16*1024);
      // http://tools.ietf.org/html/draft-ietf-rtcweb-data-channel-05#section-6.2
      UseSettings::setUInt(ORTC_SETTING_SCTP_TRANSPORT_MAX_SESSIONS_PER_PORT, kMaxSctpSid);

      // 1/2 of port range for even vs odd ports and then only 1/2 filled for that possible range
      UseSettings::setUInt(ORTC_SETTING_SCTP_TRANSPORT_MAX_PORTS, (65535-5000)/2/2);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ISCTPTransportForDataChannel
    #pragma mark

    //-------------------------------------------------------------------------
    ElementPtr ISCTPTransportForDataChannel::toDebug(ForDataChannelPtr transport)
    {
      if (!transport) return ElementPtr();
      return ZS_DYNAMIC_PTR_CAST(SCTPTransport, transport)->toDebug();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark SCTPTransport
    #pragma mark
    
    //---------------------------------------------------------------------------
    const char *SCTPTransport::toString(States state)
    {
      switch (state) {
        case State_Pending:       return "pending";
        case State_Ready:         return "ready";
        case State_Disconnected:  return "disconnected";
        case State_ShuttingDown:  return "shutting down";
        case State_Shutdown:      return "shutdown";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    SCTPTransport::SCTPTransport(
                                 const make_private &,
                                 IMessageQueuePtr queue,
                                 UseSecureTransportPtr secureTransport
                                 ) :
      MessageQueueAssociator(queue),
      SharedRecursiveLock(SharedRecursiveLock::create()),
      mSCTPInit(SCTPInit::singleton()),
      mMaxSessionsPerPort(UseSettings::getUInt(ORTC_SETTING_SCTP_TRANSPORT_MAX_SESSIONS_PER_PORT)),
      mMaxPorts(UseSettings::getUInt(ORTC_SETTING_SCTP_TRANSPORT_MAX_PORTS)),
      mSecureTransport(DTLSTransport::convert(secureTransport))
    {
      ORTC_THROW_INVALID_PARAMETERS_IF(!secureTransport)

      ZS_LOG_DETAIL(debug("created"))

      mSecureTransportReady = secureTransport->notifyWhenReady();
      mSecureTransportClosed = secureTransport->notifyWhenClosed();
      mICETransport = ICETransport::convert(secureTransport->getICETransport());

      ORTC_THROW_INVALID_STATE_IF(!mSCTPInit)
    }

    //-------------------------------------------------------------------------
    void SCTPTransport::init()
    {
      AutoRecursiveLock lock(*this);
      //IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
      
      ZS_LOG_DETAIL(debug("SCTP init"))

      mSecureTransportReady->thenWeak(mThisWeak.lock());
      mSecureTransportClosed->thenWeak(mThisWeak.lock());

      auto iceTransport = mICETransport.lock();
      if (iceTransport) {
        mICETransportSubscription = iceTransport->subscribe(mThisWeak.lock());
      }

      IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
    }

    //-------------------------------------------------------------------------
    SCTPTransport::~SCTPTransport()
    {
      if (isNoop()) return;

      ZS_LOG_DETAIL(log("destroyed"))
      mThisWeak.reset();

      cancel();
    }

    //-------------------------------------------------------------------------
    SCTPTransportPtr SCTPTransport::convert(ISCTPTransportPtr object)
    {
      ISCTPTransportPtr original = ISCTPTransportTearAway::original(object);
      return ZS_DYNAMIC_PTR_CAST(SCTPTransport, object);
    }

    //-------------------------------------------------------------------------
    SCTPTransportPtr SCTPTransport::convert(IDataTransportPtr object)
    {
      return ZS_DYNAMIC_PTR_CAST(SCTPTransport, object);
    }

    //-------------------------------------------------------------------------
    SCTPTransportPtr SCTPTransport::convert(ForSettingsPtr object)
    {
      return ZS_DYNAMIC_PTR_CAST(SCTPTransport, object);
    }

    //-------------------------------------------------------------------------
    SCTPTransportPtr SCTPTransport::convert(ForSecureTransportPtr object)
    {
      return ZS_DYNAMIC_PTR_CAST(SCTPTransport, object);
    }


    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark SCTPTransport => IStatsProvider
    #pragma mark

    //-------------------------------------------------------------------------
    IStatsProvider::PromiseWithStatsReportPtr SCTPTransport::getStats() const throw(InvalidStateError)
    {
#define TODO_COMPLETE 1
#define TODO_COMPLETE 2
      return PromiseWithStatsReportPtr();
    }


    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark SCTPTransport => ISCTPTransport
    #pragma mark
    
    //-------------------------------------------------------------------------
    ElementPtr SCTPTransport::toDebug(SCTPTransportPtr transport)
    {
      if (!transport) return ElementPtr();
      return transport->toDebug();
    }

    //-------------------------------------------------------------------------
    ISCTPTransportPtr SCTPTransport::create(
                                            ISCTPTransportDelegatePtr delegate,
                                            IDTLSTransportPtr transport
                                            )
    {
      ORTC_THROW_INVALID_PARAMETERS_IF(!transport)

      UseSecureTransportPtr useSecureTransport = DTLSTransport::convert(transport);
      ASSERT(((bool)useSecureTransport))

      auto dataTransport = useSecureTransport->getDataTransport();
      ORTC_THROW_INVALID_STATE_IF(!dataTransport)

      auto sctpTransport = SCTPTransport::convert(dataTransport);
      ORTC_THROW_INVALID_STATE_IF(!sctpTransport)

      auto tearAway = ISCTPTransportTearAway::create(sctpTransport, make_shared<TearAwayData>());
      ORTC_THROW_INVALID_STATE_IF(!tearAway)

      auto tearAwayData = ISCTPTransportTearAway::data(tearAway);
      ORTC_THROW_INVALID_STATE_IF(!tearAwayData)

      tearAwayData->mSecureTransport = useSecureTransport;

      if (delegate) {
        tearAwayData->mDefaultSubscription = sctpTransport->subscribe(delegate);
      }
      
      return tearAway;
    }

    //-------------------------------------------------------------------------
    ISCTPTransportTypes::CapabilitiesPtr SCTPTransport::getCapabilities()
    {
      CapabilitiesPtr result(make_shared<Capabilities>());
      result->mMaxMessageSize = UseSettings::getUInt(ORTC_SETTING_SCTP_TRANSPORT_MAX_MESSAGE_SIZE);
      return result;
    }

    //-------------------------------------------------------------------------
    void SCTPTransport::start(const Capabilities &remoteCapabilities)
    {
#if 0
      struct socket *sock;
      short mediaPort = 0, remoteUDPEncapPort = 0;
      const int on = 1;
      unsigned int i;
      struct sctp_assoc_value av;
      struct sctp_udpencaps encaps;
      struct sctp_event event;
      uint16_t event_types[] = {SCTP_ASSOC_CHANGE,
        SCTP_PEER_ADDR_CHANGE,
        SCTP_REMOTE_ERROR,
        SCTP_SHUTDOWN_EVENT,
        SCTP_ADAPTATION_INDICATION,
        SCTP_PARTIAL_DELIVERY_EVENT};
#endif
      
      AutoRecursiveLock lock(*this);
      if ((isShuttingDown()) ||
          (isShutdown())) {
#define WARNING_WHAT_IF_SCTP_ERROR_CAUSED_CLOSE 1
#define WARNING_WHAT_IF_SCTP_ERROR_CAUSED_CLOSE 2
        ORTC_THROW_INVALID_STATE("already shutting down")
      }
      
      mCapabilities = remoteCapabilities;

#if 0
      //Retrieve media port over which SCTP to be established
#define TODO_FIND_MEDIA_PORT 1
#define TODO_FIND_MEDIA_PORT 2
      usrsctp_init(mediaPort, NULL, NULL);
#ifdef SCTP_DEBUG
      usrsctp_sysctl_set_sctp_debug_on(0);
#endif
      //Enable SCTP blackholing
      usrsctp_sysctl_set_sctp_blackhole(SCTPCTL_BLACKHOLE_MAX);
      
      //Pass DTLS packet handler to usrsctp socket; DTLS will pass it to SCTP after decryption
      if ((sock = usrsctp_socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP,
                                 mDTLSTransport->handleReceivedPacket, NULL, 0)) == NULL) {
        setError(errno);
      }
      if (usrsctp_setsockopt(sock, IPPROTO_SCTP, SCTP_I_WANT_MAPPED_V4_ADDR,
                             (const void*)&on, (socklen_t)sizeof(int)) < 0) {
        setError(errno);
      }
      memset(&av, 0, sizeof(struct sctp_assoc_value));
      av.assoc_id = SCTP_ALL_ASSOC;
      av.assoc_value = 47;
      
      if (usrsctp_setsockopt(sock, IPPROTO_SCTP, SCTP_CONTEXT, (const void*)&av,
                             (socklen_t)sizeof(struct sctp_assoc_value)) < 0) {
        setError(errno, "SCTP ");
      }
      
#define TODO_REMOTE_UDP_ENCAPS 1
#define TODO_REMOTE_UDP_ENCAPS 2
      if (/* UDP encapsulation enabled */ false) {
        memset(&encaps, 0, sizeof(struct sctp_udpencaps));
        encaps.sue_address.ss_family = AF_INET6;
        encaps.sue_port = htons(remoteUDPEncapPort);
        if (usrsctp_setsockopt(sock, IPPROTO_SCTP, SCTP_REMOTE_UDP_ENCAPS_PORT,
                               (const void*)&encaps,
                               (socklen_t)sizeof(struct sctp_udpencaps)) < 0) {
          setError(errno, "SCTP Remote UDP Encapsulation port config failed!!!");
        }
      }
      memset(&event, 0, sizeof(event));
      event.se_assoc_id = SCTP_FUTURE_ASSOC;
      event.se_on = 1;
      for (i = 0; i < (unsigned int)(sizeof(event_types)/sizeof(uint16_t)); i++) {
        event.se_type = event_types[i];
        if (usrsctp_setsockopt(sock, IPPROTO_SCTP, SCTP_EVENT, &event,
                               sizeof(struct sctp_event)) < 0) {
          setError(errno, "SCTP Event association failed!!!");
        }
      }
#endif
    }

    //-------------------------------------------------------------------------
    void SCTPTransport::stop()
    {
      AutoRecursiveLock lock(*this);
#define TODO 1
#define TODO 2
    }

    //-------------------------------------------------------------------------
    ISCTPTransportSubscriptionPtr SCTPTransport::subscribe(ISCTPTransportDelegatePtr originalDelegate)
    {
      ZS_LOG_DETAIL(log("subscribing to transport state"))

      AutoRecursiveLock lock(*this);

      ISCTPTransportSubscriptionPtr subscription = mSubscriptions.subscribe(originalDelegate, IORTCForInternal::queueDelegate());

      ISCTPTransportDelegatePtr delegate = mSubscriptions.delegate(subscription, true);

      if (delegate) {
        SCTPTransportPtr pThis = mThisWeak.lock();

        for (auto iter = mAnnouncedIncomingDataChannels.begin(); iter != mAnnouncedIncomingDataChannels.end(); ++iter) {
          // NOTE: ID of data channels are always greater than last so order
          // should be guarenteed.
          auto dataChannel = (*iter).second;
          delegate->onSCTPTransportDataChannel(mThisWeak.lock(), DataChannel::convert(dataChannel));
        }

#define TODO_DO_WE_NEED_TO_TELL_ABOUT_ANY_MISSED_EVENTS 1
#define TODO_DO_WE_NEED_TO_TELL_ABOUT_ANY_MISSED_EVENTS 2
      }

      if (isShutdown()) {
        mSubscriptions.clear();
      }

      return subscription;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark SCTPTransport => ISCTPTransportForSecureTransport
    #pragma mark

    //-------------------------------------------------------------------------
    SCTPTransport::ForSecureTransportPtr SCTPTransport::create(UseSecureTransportPtr transport)
    {
      SCTPTransportPtr pThis(make_shared<SCTPTransport>(make_private {}, IORTCForInternal::queueORTC(), transport));
      pThis->mThisWeak = pThis;
      pThis->init();
      return pThis;
    }

    //-------------------------------------------------------------------------
    bool SCTPTransport::handleDataPacket(
                                         const BYTE *buffer,
                                         size_t bufferLengthInBytes
                                         )
    {
      if (bufferLengthInBytes < sizeof(DWORD)) {
        ZS_LOG_WARNING(Trace, log("packet length is too small to be an SCTP packet"))
        return false;
      }

      WORD localPort {};
      WORD remotePort {};
      DWORD tupleID = UseSCTPHelper::getLocalRemoteTuple(buffer, bufferLengthInBytes, UseSCTPHelper::Direction_Incoming, &localPort, &remotePort);
      if (0 == tupleID) {
        ZS_LOG_WARNING(Trace, log("incoming packet is not valid") + ZS_PARAM("buffer length", bufferLengthInBytes))
        return false;
      }

      {
        AutoRecursiveLock lock(*this);

        if ((mPendingIncomingData.size() > 0) ||
            (State_Pending == mCurrentState)) {
          ZS_LOG_WARNING(Insane, log("buffering incoming packet until SCTP transport is ready"))
          SecureByteBlockPtr queueBuffer = UseServicesHelper::convertToBuffer(buffer, bufferLengthInBytes);
          mPendingIncomingData.push(queueBuffer);
          return true;
        }

        SocketInfoPtr socketInfo;

        auto found = mSockets.find(tupleID);
        if (found == mSockets.end()) {
          if (mAllocatedLocalPorts.end() != mAllocatedLocalPorts.find(localPort)) {
            ZS_LOG_WARNING(Debug, log("local port is already in use (thus will ignore incoming SCTP packet on mismatched local:remote port mapping)") + ZS_PARAM("local port", localPort) + ZS_PARAM("remote port", remotePort))
            return false;
          }

          socketInfo = openIncomingSocket(localPort, remotePort, tupleID);
          if (!socketInfo) {
            ZS_LOG_WARNING(Debug, log("unable to open socket"))
            return false;
          }
        } else {
          socketInfo = (*found).second;
        }

        usrsctp_conninput(socketInfo->mThis, buffer, bufferLengthInBytes, 0);

        attemptAccept(*socketInfo);
      }

      return false;
    }


    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark SCTPTransport => ISCTPTransportForDataChannel
    #pragma mark

    //-------------------------------------------------------------------------
    ISCTPTransportForDataChannelSubscriptionPtr SCTPTransport::subscribe(ISCTPTransportForDataChannelDelegatePtr originalDelegate)
    {
      ZS_LOG_DETAIL(log("datachannel subscribing to SCTP Transport"))
      
      AutoRecursiveLock lock(*this);
      
      ISCTPTransportForDataChannelSubscriptionPtr subscription = mDataChannelSubscriptions.subscribe(originalDelegate, IORTCForInternal::queueORTC());
      
      ISCTPTransportForDataChannelDelegatePtr delegate = mDataChannelSubscriptions.delegate(subscription, true);
      
      if (delegate) {
        SCTPTransportPtr pThis = mThisWeak.lock();

        if (State_Pending != mCurrentState) {
          delegate->onSCTPTransportStateChanged();
        }
      }
      
      if (isShutdown()) {
        mSubscriptions.clear();
      }
      
      return subscription;
    }

    //-------------------------------------------------------------------------
    bool SCTPTransport::isReady() const
    {
      return State_Ready == mCurrentState;
    }

    //-------------------------------------------------------------------------
    WORD SCTPTransport::allocateLocalPort(UseDataChannelPtr channel)
    {
#define TODO_REMOVE 1
#define TODO_REMOVE 2
#if 0
      AutoRecursiveLock lock(*this);

      if (mSockets.size() > mMaxPorts) return 0;

      while (true) {

        {
          auto found = mAllocatedLocalPorts.find(mCurrentAllocationPort);
          if (found != mAllocatedLocalPorts.end()) goto next_port;
        }
        {
          auto found = mAllocatedLocalPorts.find(mCurrentAllocationPort);
          if (found != mAllocatedLocalPorts.end()) goto next_port;
        }

      next_port:
        mCurrentAllocationPort = mCurrentAllocationPort + mNextAllocationIncremement;

        if (mCurrentAllocationPort < mMinAllocationPort) mCurrentAllocationPort = mMinAllocationPort + (mCurrentAllocationPort % 2);
        if (mCurrentAllocationPort > mMaxAllocationPort) mCurrentAllocationPort = mMinAllocationPort + (mCurrentAllocationPort % 2);
      }

      allocatePort(mAllocatedLocalPorts, mCurrentAllocationPort);
      allocatePort(mAllocatedRemotePorts, mCurrentAllocationPort);

//      mDataChannelTuples[UseSCTPHelper::createTuple(mCurrentAllocationPort, mCurrentAllocationPort)] = channel;

      return mCurrentAllocationPort;
#endif //0
      return 0;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark SCTPTransport => friend SCTPInit
    #pragma mark

    //-------------------------------------------------------------------------
    bool SCTPTransport::notifySendSCTPPacket(
                                             const BYTE *buffer,
                                             size_t bufferLengthInBytes
                                             )
    {
      UseSecureTransportPtr transport;

      {
        AutoRecursiveLock lock(*this);
        transport = mSecureTransport.lock();
      }

      if (!transport) {
        ZS_LOG_WARNING(Trace, log("secure transport is gone (thus send packet is not available)") + ZS_PARAM("buffer length", bufferLengthInBytes))
        return false;
      }

      return transport->sendDataPacket(buffer, bufferLengthInBytes);
    }


    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark SCTPTransport => IWakeDelegate
    #pragma mark

    //-------------------------------------------------------------------------
    void SCTPTransport::onWake()
    {
      ZS_LOG_DEBUG(log("wake"))

      AutoRecursiveLock lock(*this);
      step();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark SCTPTransport => ITimerDelegate
    #pragma mark

    //-------------------------------------------------------------------------
    void SCTPTransport::onTimer(TimerPtr timer)
    {
      ZS_LOG_DEBUG(log("timer") + ZS_PARAM("timer id", timer->getID()))

      AutoRecursiveLock lock(*this);
#define TODO 1
#define TODO 2
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark SCTPTransport => ISCTPTransportAsyncDelegate
    #pragma mark


    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark SCTPTransport => IICETransportDelegate
    #pragma mark

    //-------------------------------------------------------------------------
    void SCTPTransport::onICETransportStateChanged(
                                                   IICETransportPtr transport,
                                                   IICETransport::States state
                                                   )
    {
      ZS_LOG_DEBUG(log("ice transport state changed") + ZS_PARAM("ice transport id", transport->getID()) + ZS_PARAM("state", IICETransport::toString(state)))

      AutoRecursiveLock lock(*this);
      step();
    }

    //-------------------------------------------------------------------------
    void SCTPTransport::onICETransportCandidatePairAvailable(
                                                             IICETransportPtr transport,
                                                             CandidatePairPtr candidatePair
                                                             )
    {
      // ignored
    }

    //-------------------------------------------------------------------------
    void SCTPTransport::onICETransportCandidatePairGone(
                                                        IICETransportPtr transport,
                                                        CandidatePairPtr candidatePair
                                                        )
    {
      // ignored
    }

    //-------------------------------------------------------------------------
    void SCTPTransport::onICETransportCandidatePairChanged(
                                                           IICETransportPtr transport,
                                                           CandidatePairPtr candidatePair
                                                           )
    {
      // ignored
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark SCTPTransport => IPromiseSettledDelegate
    #pragma mark

    //-------------------------------------------------------------------------
    void SCTPTransport::onPromiseSettled(PromisePtr promise)
    {
#define TODO 1
#define TODO 2
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark SCTPTransport => (internal)
    #pragma mark

    //-------------------------------------------------------------------------
    Log::Params SCTPTransport::log(const char *message) const
    {
      ElementPtr objectEl = Element::create("ortc::SCTPTransport");
      UseServicesHelper::debugAppend(objectEl, "id", mID);
      return Log::Params(message, objectEl);
    }

    //-------------------------------------------------------------------------
    Log::Params SCTPTransport::slog(const char *message)
    {
      ElementPtr objectEl = Element::create("ortc::SCTPTransport");
      return Log::Params(message, objectEl);
    }

    //-------------------------------------------------------------------------
    Log::Params SCTPTransport::debug(const char *message) const
    {
      return Log::Params(message, toDebug());
    }

    //-------------------------------------------------------------------------
    ElementPtr SCTPTransport::toDebug() const
    {
      AutoRecursiveLock lock(*this);

      ElementPtr resultEl = Element::create("ortc::SCTPTransport");

      UseServicesHelper::debugAppend(resultEl, "id", mID);

      UseServicesHelper::debugAppend(resultEl, "graceful shutdown", (bool)mGracefulShutdownReference);

      UseServicesHelper::debugAppend(resultEl, "subscribers", mSubscriptions.size());

      UseServicesHelper::debugAppend(resultEl, "state", toString(mCurrentState));

      UseServicesHelper::debugAppend(resultEl, "error", mLastError);
      UseServicesHelper::debugAppend(resultEl, "error reason", mLastErrorReason);

      auto secureTransport = mSecureTransport.lock();
      UseServicesHelper::debugAppend(resultEl, "secure transport", secureTransport ? secureTransport->getID() : 0);
      UseServicesHelper::debugAppend(resultEl, "secure transport ready promise", (bool)mSecureTransportReady);

      auto iceTransport = mICETransport.lock();
      UseServicesHelper::debugAppend(resultEl, "ice transport", iceTransport ? iceTransport->getID() : 0);
      UseServicesHelper::debugAppend(resultEl, "ice transport subscription", mICETransportSubscription ? mICETransportSubscription->getID() : 0);

      UseServicesHelper::debugAppend(resultEl, mCapabilities.toDebug());

      return resultEl;
    }

    //-------------------------------------------------------------------------
    bool SCTPTransport::isShuttingDown() const
    {
      return State_ShuttingDown == mCurrentState;
    }

    //-------------------------------------------------------------------------
    bool SCTPTransport::isShutdown() const
    {
      return State_Shutdown == mCurrentState;
    }

    //-------------------------------------------------------------------------
    void SCTPTransport::step()
    {
      ZS_LOG_DEBUG(debug("step"))

      if ((isShuttingDown()) ||
          (isShutdown())) {
        ZS_LOG_DEBUG(debug("step forwarding to cancel"))
        cancel();
        return;
      }

      // ... other steps here ...
      if (!stepSecureTransport()) goto not_ready;
      if (!stepICETransport()) goto not_ready;
      if (!stepDeliverIncomingPackets()) goto not_ready;
      // ... other steps here ...

      goto ready;

    not_ready:
      {
        ZS_LOG_TRACE(debug("SCTP is NOT ready!!!"))
        return;
      }

    ready:
      {
        ZS_LOG_TRACE(log("SCTP is ready!!!"))
        setState(State_Ready);
        
      }
    }

    //-------------------------------------------------------------------------
    bool SCTPTransport::stepSecureTransport()
    {
      if (mSecureTransportClosed->isSettled()) {
        ZS_LOG_WARNING(Detail, log("secure transport is now closed (thus must shutdown)"))
        cancel();
        return false;
      }

      if (!mSecureTransportReady) {
        ZS_LOG_TRACE(log("secure transport already notified ready"))
        return true;
      }

      if (!mSecureTransportReady->isSettled()) {
        ZS_LOG_TRACE(log("waiting for secure transport to notify ready"))
        return false;
      }

      ZS_LOG_DEBUG(log("secure transport notified ready"))
      mSecureTransportReady.reset();

      auto secureTransport = mSecureTransport.lock();
      if (!secureTransport) {
        ZS_LOG_WARNING(Detail, log("secure transport is gone (thus must shutdown)"))
        cancel();
        return false;
      }

      auto clientRole = secureTransport->isClientRole();
      if (clientRole) {
        mCurrentAllocationPort = mMinAllocationPort;    // client picks even port numbers
      } else {
        mCurrentAllocationPort = mMinAllocationPort+1;  // server picks odd port numbers
      }

      return true;
    }

    //-------------------------------------------------------------------------
    bool SCTPTransport::stepICETransport()
    {
      auto iceTransport = mICETransport.lock();

      if (!iceTransport) {
        ZS_LOG_WARNING(Detail, log("ice transport is gone (thus must shutdown)"))
        cancel();
        return false;
      }

      auto state = iceTransport->state();
      switch (state) {
        case IICETransport::State_New:
        case IICETransport::State_Checking:
        {
          ZS_LOG_TRACE(log("ice is not ready"))
          setState(State_Disconnected);
          break;
        }
        case IICETransport::State_Connected:
        case IICETransport::State_Completed:
        {
          ZS_LOG_TRACE(log("ice is ready"))
          setState(State_Ready);
          return true;
        }
        case IICETransport::State_Disconnected:
        case IICETransport::State_Failed:
        {
          ZS_LOG_TRACE(log("ice is disconnected"))
          setState(State_Disconnected);
          break;
        }
        case IICETransport::State_Closed:
        {
          ZS_LOG_WARNING(Detail, log("ice is closed"))
          cancel();
          break;
        }
      }

      return false;
    }

    //-------------------------------------------------------------------------
    bool SCTPTransport::stepDeliverIncomingPackets()
    {
      if (mPendingIncomingData.size() < 1) {
        ZS_LOG_TRACE(log("no pending packets to deliver"))
        return true;
      }

      BufferedQueue pending = mPendingIncomingData;
      mPendingIncomingData = BufferedQueue();

      while (pending.size() > 0) {
        SecureByteBlockPtr buffer = pending.front();
        handleDataPacket(buffer->BytePtr(), buffer->SizeInBytes());
        pending.pop();
      }

      return true;
    }

    //-------------------------------------------------------------------------
    void SCTPTransport::cancel()
    {
      //.......................................................................
      // try to gracefully shutdown

      if (isShutdown()) return;

      if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

      if (mGracefulShutdownReference) {
#define TODO_OBJECT_IS_BEING_KEPT_ALIVE_UNTIL_SCTP_SESSION_IS_SHUTDOWN 1
#define TODO_OBJECT_IS_BEING_KEPT_ALIVE_UNTIL_SCTP_SESSION_IS_SHUTDOWN 2

        // grace shutdown process done here

        return;
      }

      //.......................................................................
      // final cleanup

      setState(State_Shutdown);

      mSubscriptions.clear();

      if (mICETransportSubscription) {
        mICETransportSubscription->cancel();
        mICETransportSubscription.reset();
      }

      // make sure to cleanup any final reference to self
      mGracefulShutdownReference.reset();
    }

    //-------------------------------------------------------------------------
    void SCTPTransport::setState(States state)
    {
      if (state == mCurrentState) return;

      ZS_LOG_DETAIL(debug("state changed") + ZS_PARAM("new state", toString(state)) + ZS_PARAM("old state", toString(mCurrentState)))

      mCurrentState = state;
      mDataChannelSubscriptions.delegate()->onSCTPTransportStateChanged();

//      SCTPTransportPtr pThis = mThisWeak.lock();
//      if (pThis) {
//        mSubscriptions.delegate()->onSCTPTransportStateChanged(pThis, mCurrentState);
//      }
    }

    //-------------------------------------------------------------------------
    void SCTPTransport::setError(WORD errorCode, const char *inReason)
    {
      String reason(inReason);
      if (reason.isEmpty()) {
        reason = UseHTTP::toString(UseHTTP::toStatusCode(errorCode));
      }

      if (0 != mLastError) {
        ZS_LOG_WARNING(Detail, debug("error already set thus ignoring new error") + ZS_PARAM("new error", errorCode) + ZS_PARAM("new reason", reason))
        return;
      }

      mLastError = errorCode;
      mLastErrorReason = reason;

      ZS_LOG_WARNING(Detail, debug("error set") + ZS_PARAM("error", mLastError) + ZS_PARAM("reason", mLastErrorReason))
    }

    //-------------------------------------------------------------------------
    void SCTPTransport::allocatePort(
                                     AllocatedPortMap &useMap,
                                     WORD port
                                     )
    {
      auto found = useMap.find(port);
      if (found == useMap.end()) {
        useMap[port] = 1;
        return;
      }

      size_t &total = (*found).second;
      ++total;
    }

    //-------------------------------------------------------------------------
    void SCTPTransport::deallocatePort(
                                       AllocatedPortMap &useMap,
                                       WORD port
                                       )
    {
      auto found = useMap.find(port);
      if (found == useMap.end()) {
        ZS_LOG_ERROR(Debug, log("allocation was not found") + ZS_PARAM("port", port))
        return;
      }

      size_t &total = (*found).second;
      --total;

      if (0 != total) return;
      useMap.erase(found);
    }
    
    //-------------------------------------------------------------------------
    SCTPTransport::SocketInfoPtr SCTPTransport::openIncomingSocket(
                                                                   WORD localPort,
                                                                   WORD remotePort,
                                                                   LocalRemoteTupleID tupleID
                                                                   )
    {
      SocketInfoPtr socketInfo(make_shared<SocketInfo>());
      socketInfo->mThis = new SocketInfoPtr(socketInfo);
      socketInfo->mTransport = mThisWeak.lock();
      socketInfo->mIncoming = true;
      socketInfo->mLocalPort = localPort;
      socketInfo->mRemotePort = remotePort;
      socketInfo->mTupleID = tupleID;
      socketInfo->mCurrentAllocationSessionID = 0 + (mCurrentAllocationPort % 2); // even vs odd allocation

      if (!openListenSCTPSocket(*socketInfo)) {
        ZS_LOG_ERROR(Debug, log("failed to open incoming socket") + socketInfo->toDebug())
        socketInfo->close();
        return SocketInfoPtr();
      }

      ZS_LOG_DEBUG(log("successfully opened incoming listen socket") + socketInfo->toDebug())
      return socketInfo;
    }

    //-------------------------------------------------------------------------
    bool SCTPTransport::openListenSCTPSocket(SocketInfo &info)
    {
      if (!openSCTPSocket(info)) {
        ZS_LOG_ERROR(Detail, log("failed to open listen socket"))
        return false;
      }

      if (usrsctp_listen(info.mSocket, 1)) {
        ZS_LOG_ERROR(Detail, log("failed to listen on SCTP socket"))
        return false;
      }

      return true;
    }

    //-------------------------------------------------------------------------
    bool SCTPTransport::openSCTPSocket(SocketInfo &info)
    {
      info.mSocket = usrsctp_socket(AF_CONN, SOCK_STREAM, IPPROTO_SCTP, SCTPInit::OnSctpInboundPacket, NULL, 0,info.mThis);
      if (!info.mSocket) {
        ZS_LOG_ERROR(Detail, log("failed to create sctp socket"))
        return false;
      }

      if (!prepareSocket(info.mSocket)) {
        ZS_LOG_ERROR(Detail, log("failed to setup socket"))
        return false;
      }

      usrsctp_register_address(info.mThis);
      return true;
    }

    //-------------------------------------------------------------------------
    bool SCTPTransport::prepareSocket(struct socket *sock)
    {
      // Make the socket non-blocking. Connect, close, shutdown etc will not
      // block the thread waiting for the socket operation to complete.

      if (usrsctp_set_non_blocking(sock, 1) < 0) {
        ZS_LOG_ERROR(Detail, log("failed to set SCTP socket to non-blocking"))
        return false;
      }

      // This ensures that the usrsctp close call deletes the association. This
      // prevents usrsctp from calling OnSctpOutboundPacket with references to
      // this class as the address.
      linger linger_opt {};
      linger_opt.l_onoff = 1;
      linger_opt.l_linger = 0;
      if (usrsctp_setsockopt(sock, SOL_SOCKET, SO_LINGER, &linger_opt, sizeof(linger_opt))) {
        ZS_LOG_ERROR(Detail, log("failed to set SCTP socket SO_LINGER"))
        return false;
      }

      // Enable stream ID resets.
      struct sctp_assoc_value stream_rst {};
      stream_rst.assoc_id = SCTP_ALL_ASSOC;
      stream_rst.assoc_value = 1;
      if (usrsctp_setsockopt(sock, IPPROTO_SCTP, SCTP_ENABLE_STREAM_RESET, &stream_rst, sizeof(stream_rst))) {
        ZS_LOG_ERROR(Detail, log("failed to set SCTP_ENABLE_STREAM_RESET"))
        return false;
      }

      // Nagle.
      uint32_t nodelay = 1;
      if (usrsctp_setsockopt(sock, IPPROTO_SCTP, SCTP_NODELAY, &nodelay, sizeof(nodelay))) {
        ZS_LOG_ERROR(Detail, log("failed to set SCTP_NODELAY"))
        return false;
      }

      struct sctp_paddrparams params = {{0}};
      params.spp_assoc_id = 0;
      params.spp_flags = SPP_PMTUD_DISABLE;
      params.spp_pathmtu = kSctpMtu;
      if (usrsctp_setsockopt(sock, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS, &params, sizeof(params))) {
        ZS_LOG_ERROR(Detail, log("failed to set SCTP_PEER_ADDR_PARAMS"))
        return false;
      }

      int event_types[] = {
        SCTP_ASSOC_CHANGE,
        SCTP_PEER_ADDR_CHANGE,
        SCTP_SEND_FAILED_EVENT,
        SCTP_SENDER_DRY_EVENT,
        SCTP_STREAM_RESET_EVENT
      };
      struct sctp_event event = {0};
      event.se_assoc_id = SCTP_ALL_ASSOC;
      event.se_on = 1;
      for (size_t i = 0; i < ARRAY_SIZE(event_types); i++) {
        event.se_type = event_types[i];
        if (usrsctp_setsockopt(sock, IPPROTO_SCTP, SCTP_EVENT, &event, sizeof(event)) < 0) {
          ZS_LOG_ERROR(Detail, log("failed to set SCTP_EVENT type") + ZS_PARAM("event", event.se_type))
          return false;
        }
      }

      return true;
    }

    //-------------------------------------------------------------------------
    void SCTPTransport::attemptAccept(SocketInfo &info)
    {
      if (!info.mIncoming) return;    // only applies to incoming sockets
      if (info.mAcceptSocket) return; // already accepted

      sockaddr_conn localAddr = info.getSockAddr(true);

      socklen_t addressLength = sizeof(localAddr);
      if (usrsctp_accept(info.mSocket, reinterpret_cast<sockaddr *>(&localAddr), &addressLength)) {
        if (SCTP_EWOULDBLOCK == errno) {
          ZS_LOG_TRACE(log("not ready to accept incoming connection yet") + info.toDebug())
          return;
        }

        ZS_LOG_ERROR(Debug, log("cannot accept socket at this time") + info.toDebug())
        return;
      }

    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ISCTPTransportFactory
    #pragma mark

    //-------------------------------------------------------------------------
    ISCTPTransportFactory &ISCTPTransportFactory::singleton()
    {
      return SCTPTransportFactory::singleton();
    }

    //-------------------------------------------------------------------------
    ISCTPTransportPtr ISCTPTransportFactory::create(
                                                    ISCTPTransportDelegatePtr delegate,
                                                    IDTLSTransportPtr transport
                                                    )
    {
      if (this) {}
      return internal::SCTPTransport::create(delegate, transport);
    }

    //-------------------------------------------------------------------------
    ISCTPTransportFactory::ForSecureTransportPtr ISCTPTransportFactory::create(UseSecureTransportPtr transport)
    {
      if (this) {}
      return internal::SCTPTransport::create(transport);
    }

    //-------------------------------------------------------------------------
    ISCTPTransportFactory::CapabilitiesPtr ISCTPTransportFactory::getCapabilities()
    {
      if (this) {}
      return SCTPTransport::getCapabilities();
    }

  } // internal namespace


  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark ISCTPTransportTypes::Parameters
  #pragma mark

  //---------------------------------------------------------------------------
  ElementPtr ISCTPTransportTypes::Capabilities::toDebug() const
  {
    ElementPtr resultEl = Element::create("ortc::ISCTPTransportTypes::Capabilities");

    UseServicesHelper::debugAppend(resultEl, "max message size", mMaxMessageSize);

    return resultEl;
  }

  //---------------------------------------------------------------------------
  String ISCTPTransportTypes::Capabilities::hash() const
  {
    SHA1Hasher hasher;

    hasher.update("ISCTPTransportTypes:Capabilities:");
    hasher.update(mMaxMessageSize);
    return hasher.final();
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark ISCTPTransport
  #pragma mark

  //---------------------------------------------------------------------------
  ElementPtr ISCTPTransport::toDebug(ISCTPTransportPtr transport)
  {
    return internal::SCTPTransport::toDebug(internal::SCTPTransport::convert(transport));
  }

  //---------------------------------------------------------------------------
  ISCTPTransportPtr ISCTPTransport::create(
                                           ISCTPTransportDelegatePtr delegate,
                                           IDTLSTransportPtr transport
                                           )
  {
    return internal::ISCTPTransportFactory::singleton().create(delegate, transport);
  }

  //---------------------------------------------------------------------------
  ISCTPTransportTypes::CapabilitiesPtr ISCTPTransport::getCapabilities()
  {
    return internal::ISCTPTransportFactory::singleton().getCapabilities();
  }

}
