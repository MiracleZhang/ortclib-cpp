// Generated by zsLibEventingTool

#pragma once

#include "types.h"
#include "generated/org_webrtc_RTCStatsProvider.h"


namespace wrapper {
  namespace impl {
    namespace org {
      namespace webrtc {

        struct RTCStatsProvider : public wrapper::org::webrtc::RTCStatsProvider
        {
          RTCStatsProviderWeakPtr thisWeak_;

          RTCStatsProvider() noexcept;
          virtual ~RTCStatsProvider() noexcept;

          // methods RTCStatsProvider
          shared_ptr< PromiseWithHolderPtr< wrapper::org::webrtc::RTCStatsReportPtr > > getStats(wrapper::org::webrtc::RTCStatsTypeSetPtr statTypes) noexcept(false) override; // throws wrapper::org::webrtc::RTCErrorPtr
        };

      } // webrtc
    } // org
  } // namespace impl
} // namespace wrapper

