// Generated by zsLibEventingTool

#pragma once

#include "types.h"
#include "generated/org_webrtc_RTCRtpParameters.h"


namespace wrapper {
  namespace impl {
    namespace org {
      namespace webrtc {

        struct RTCRtpParameters : public wrapper::org::webrtc::RTCRtpParameters
        {
          RTCRtpParametersWeakPtr thisWeak_;

          RTCRtpParameters() noexcept;
          virtual ~RTCRtpParameters() noexcept;
          void wrapper_init_org_webrtc_RTCRtpParameters() noexcept override;
        };

      } // webrtc
    } // org
  } // namespace impl
} // namespace wrapper

