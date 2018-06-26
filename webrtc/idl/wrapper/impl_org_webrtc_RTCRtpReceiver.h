// Generated by zsLibEventingTool

#pragma once

#include "types.h"
#include "generated/org_webrtc_RTCRtpReceiver.h"


namespace wrapper {
  namespace impl {
    namespace org {
      namespace webrtc {

        struct RTCRtpReceiver : public wrapper::org::webrtc::RTCRtpReceiver
        {
          RTCRtpReceiverWeakPtr thisWeak_;

          RTCRtpReceiver() noexcept;
          virtual ~RTCRtpReceiver() noexcept;

          // methods RTCRtpReceiver
          wrapper::org::webrtc::RTCRtpReceiveParametersPtr getParameters() noexcept override;
          shared_ptr< list< wrapper::org::webrtc::RTCRtpContributingSourcePtr > > getContributingSources() noexcept override;
          shared_ptr< list< wrapper::org::webrtc::RTCRtpSynchronizationSourcePtr > > getSynchronizationSources() noexcept override;
          void wrapper_init_org_webrtc_RTCRtpReceiver() noexcept override;

          // properties RTCRtpReceiver
          wrapper::org::webrtc::MediaStreamTrackPtr get_track() noexcept override;
          void set_track(wrapper::org::webrtc::MediaStreamTrackPtr value) noexcept override;
        };

      } // webrtc
    } // org
  } // namespace impl
} // namespace wrapper

