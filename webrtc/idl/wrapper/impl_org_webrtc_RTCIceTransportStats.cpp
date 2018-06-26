// Generated by zsLibEventingTool

#include "impl_org_webrtc_RTCIceTransportStats.h"

using ::zsLib::String;
using ::zsLib::Optional;
using ::zsLib::Any;
using ::zsLib::AnyPtr;
using ::zsLib::AnyHolder;
using ::zsLib::Promise;
using ::zsLib::PromisePtr;
using ::zsLib::PromiseWithHolder;
using ::zsLib::PromiseWithHolderPtr;
using ::zsLib::eventing::SecureByteBlock;
using ::zsLib::eventing::SecureByteBlockPtr;
using ::std::shared_ptr;
using ::std::weak_ptr;
using ::std::make_shared;
using ::std::list;
using ::std::set;
using ::std::map;

//------------------------------------------------------------------------------
wrapper::impl::org::webrtc::RTCIceTransportStats::RTCIceTransportStats() noexcept
{
}

//------------------------------------------------------------------------------
wrapper::org::webrtc::RTCIceTransportStatsPtr wrapper::org::webrtc::RTCIceTransportStats::wrapper_create() noexcept
{
  auto pThis = make_shared<wrapper::impl::org::webrtc::RTCIceTransportStats>();
  pThis->thisWeak_ = pThis;
  return pThis;
}

//------------------------------------------------------------------------------
wrapper::impl::org::webrtc::RTCIceTransportStats::~RTCIceTransportStats()
{
}

//------------------------------------------------------------------------------
::zsLib::Time wrapper::impl::org::webrtc::RTCIceTransportStats::get_timestamp() noexcept
{
  ::zsLib::Time result {};
  return result;
}

//------------------------------------------------------------------------------
Optional< wrapper::org::webrtc::RTCStatsType > wrapper::impl::org::webrtc::RTCIceTransportStats::get_statsType() noexcept
{
  Optional< wrapper::org::webrtc::RTCStatsType > result {};
  return result;
}

//------------------------------------------------------------------------------
String wrapper::impl::org::webrtc::RTCIceTransportStats::get_statsTypeOther() noexcept
{
  String result {};
  return result;
}

//------------------------------------------------------------------------------
String wrapper::impl::org::webrtc::RTCIceTransportStats::get_id() noexcept
{
  String result {};
  return result;
}

//------------------------------------------------------------------------------
void wrapper::impl::org::webrtc::RTCIceTransportStats::wrapper_init_org_webrtc_RTCIceTransportStats() noexcept
{
}

//------------------------------------------------------------------------------
void wrapper::impl::org::webrtc::RTCIceTransportStats::wrapper_init_org_webrtc_RTCIceTransportStats(wrapper::org::webrtc::RTCIceTransportStatsPtr source) noexcept
{
}

//------------------------------------------------------------------------------
unsigned long long wrapper::impl::org::webrtc::RTCIceTransportStats::get_bytesSent() noexcept
{
  unsigned long long result {};
  return result;
}

//------------------------------------------------------------------------------
unsigned long long wrapper::impl::org::webrtc::RTCIceTransportStats::get_bytesReceived() noexcept
{
  unsigned long long result {};
  return result;
}

//------------------------------------------------------------------------------
String wrapper::impl::org::webrtc::RTCIceTransportStats::get_rtcpTransportStatsId() noexcept
{
  String result {};
  return result;
}

//------------------------------------------------------------------------------
bool wrapper::impl::org::webrtc::RTCIceTransportStats::get_activeConnection() noexcept
{
  bool result {};
  return result;
}

//------------------------------------------------------------------------------
String wrapper::impl::org::webrtc::RTCIceTransportStats::get_selectedCandidatePairId() noexcept
{
  String result {};
  return result;
}


