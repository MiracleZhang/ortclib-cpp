/*

 Copyright (c) 2014, Hookflash Inc. / Hookflash Inc.
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

#pragma once

#include <ortc/internal/types.h>

#include <ortc/ICertificate.h>

#include <openpeer/services/IWakeDelegate.h>
#include <zsLib/MessageQueueAssociator.h>

#include <openssl/evp.h>
#include <openssl/x509.h>

#define ORTC_SETTING_CERTIFICATE_KEY_LENGTH_IN_BITS "ortc/certificate/key-length-in-bits"
#define ORTC_SETTING_CERTIFICATE_SERIAL_RANDOM_BITS "ortc/certificate/serial-random-bits"
#define ORTC_SETTING_CERTIFICATE_LIFETIME_IN_SECONDS  "ortc/certificate/lifetime-in-seconds"
#define ORTC_SETTING_CERTIFICATE_NOT_BEFORE_WINDOW_IN_SECONDS "ortc/certificate/not-before-window-in-seconds"

namespace ortc
{
  namespace internal
  {
    ZS_DECLARE_INTERACTION_PTR(ICertificateForSettings)
    ZS_DECLARE_INTERACTION_PTR(ICertificateForDTLSTransport)

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ICertificateForSettings
    #pragma mark

    interaction ICertificateForSettings
    {
      ZS_DECLARE_TYPEDEF_PTR(ICertificateForSettings, ForSettings)

      static void applyDefaults();

      virtual ~ICertificateForSettings() {}
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ICertificateForDTLSTransport
    #pragma mark

    interaction ICertificateForDTLSTransport
    {
      ZS_DECLARE_TYPEDEF_PTR(ICertificateForDTLSTransport, ForDTLSTransport)

      static ElementPtr toDebug(ForDTLSTransportPtr certificate);

      typedef EVP_PKEY * KeyPairType;
      typedef x509_st * CertificateObjectType; // not sure of type to use

      virtual PUID getID() const = 0;

      virtual KeyPairType getKeyPair() const = 0;
      virtual CertificateObjectType getCertificate() const = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Certificate
    #pragma mark
    
    class Certificate : public Noop,
                        public MessageQueueAssociator,
                        public SharedRecursiveLock,
                        public ICertificate,
                        public ICertificateForSettings,
                        public ICertificateForDTLSTransport,
                        public IWakeDelegate
    {
    protected:
      struct make_private {};

    public:
      friend interaction ICertificate;
      friend interaction ICertificateFactory;
      friend interaction ICertificateForSettings;
      friend interaction ICertificateForDTLSTransport;

      ZS_DECLARE_STRUCT_PTR(PromiseCertificateHolder)

      typedef EVP_PKEY * KeyPairType;
      typedef x509_st * CertificateObjectType; // not sure of type to use

    public:
      Certificate(
                  const make_private &,
                  IMessageQueuePtr queue,
                  AlgorithmIdentifier algorithm
                  );

    protected:
      Certificate(Noop) :
        Noop(true),
        MessageQueueAssociator(IMessageQueuePtr()),
        SharedRecursiveLock(SharedRecursiveLock::create())
      {}

      void init();

    public:
      virtual ~Certificate();

      static CertificatePtr convert(ICertificatePtr object);
      static CertificatePtr convert(ForSettingsPtr object);
      static CertificatePtr convert(ForDTLSTransportPtr object);

    protected:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Certificate => ICertificate
      #pragma mark

      static ElementPtr toDebug(CertificatePtr certificate);

      static PromiseWithCertificatePtr generateCertificate(AlgorithmIdentifier algorithm);

      virtual PUID getID() const override {return mID;}

      virtual Time expires() const override;

      virtual FingerprintListPtr fingerprints() const override;

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Certificate => ICertificateForDTLSTransport
      #pragma mark

      // (duplicate) virtual PUID getID() const;

      virtual KeyPairType getKeyPair() const override;
      virtual CertificateObjectType getCertificate() const override;

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Certificate => IWakeDelegate
      #pragma mark

      virtual void onWake() override;

    public:
      struct PromiseCertificateHolder : public PromiseWithCertificate
      {
        PromiseCertificateHolder(IMessageQueuePtr queue = IMessageQueuePtr()) :
          PromiseWithCertificate(Promise::make_private{}, queue)
        {}

        ~PromiseCertificateHolder() {}

        CertificatePtr mCertificate;
      };

    protected:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Certificate => (internal)
      #pragma mark

      Log::Params log(const char *message) const;
      Log::Params debug(const char *message) const;
      virtual ElementPtr toDebug() const;

      void cancel();

      evp_pkey_st* MakeKey();
      X509* MakeCertificate(EVP_PKEY* pkey);

    protected:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Certificate => (data)
      #pragma mark

      AutoPUID mID;
      CertificateWeakPtr mThisWeak;
      CertificatePtr mGracefulShutdownReference;

      AlgorithmIdentifier mAlgorithm;

      PromiseCertificateHolderPtr mPromise;
      PromiseCertificateHolderWeakPtr mPromiseWeak;

      size_t mKeyLength;
      size_t mRandomBits;
      Seconds mLifetime;
      Seconds mNotBeforeWindow;

      Time mExpires;

      KeyPairType mKeyPair;
      CertificateObjectType mCertificate;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ICertificateFactory
    #pragma mark

    interaction ICertificateFactory
    {
      static ICertificateFactory &singleton();

      typedef ICertificateTypes::AlgorithmIdentifier AlgorithmIdentifier;
      ZS_DECLARE_TYPEDEF_PTR(ICertificateTypes::PromiseWithCertificate, PromiseWithCertificate)

      virtual PromiseWithCertificatePtr generateCertificate(ICertificateTypes::AlgorithmIdentifier algorithm);
    };

    class CertificateFactory : public IFactory<ICertificateFactory> {};
  }
}
