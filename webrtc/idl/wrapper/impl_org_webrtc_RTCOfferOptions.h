// Generated by zsLibEventingTool

#pragma once

#include "types.h"
#include "generated/org_webrtc_RTCOfferOptions.h"


namespace wrapper {
  namespace impl {
    namespace org {
      namespace webrtc {

        struct RTCOfferOptions : public wrapper::org::webrtc::RTCOfferOptions
        {
          RTCOfferOptionsWeakPtr thisWeak_;

          RTCOfferOptions() noexcept;
          virtual ~RTCOfferOptions() noexcept;

          // methods RTCOfferOptions
          void wrapper_init_org_webrtc_RTCOfferOptions() noexcept override;
          void wrapper_init_org_webrtc_RTCOfferOptions(wrapper::org::webrtc::RTCOfferOptionsPtr source) noexcept override;

          // properties RTCOfferOptions
          bool get_iceRestart() noexcept override;
          void set_iceRestart(bool value) noexcept override;
          Optional< bool > get_offerToReceiveVideo() noexcept override;
          void set_offerToReceiveVideo(Optional< bool > value) noexcept override;
          Optional< bool > get_offerToReceiveAudio() noexcept override;
          void set_offerToReceiveAudio(Optional< bool > value) noexcept override;
          bool get_useRtpMux() noexcept override;
          void set_useRtpMux(bool value) noexcept override;
        };

      } // webrtc
    } // org
  } // namespace impl
} // namespace wrapper

