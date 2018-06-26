// Generated by zsLibEventingTool

#pragma once

#include "types.h"
#include "generated/org_webrtc_EventQueue.h"


namespace wrapper {
  namespace impl {
    namespace org {
      namespace webrtc {

        struct EventQueue : public wrapper::org::webrtc::EventQueue
        {
          EventQueueWeakPtr thisWeak_;

          EventQueue() noexcept;
          virtual ~EventQueue() noexcept;

          // methods EventQueue
          void wrapper_init_org_webrtc_EventQueue(AnyPtr queue) noexcept override;

          // properties EventQueue
          AnyPtr get_queue() noexcept override;
        };

      } // webrtc
    } // org
  } // namespace impl
} // namespace wrapper

