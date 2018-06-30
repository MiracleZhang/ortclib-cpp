// Generated by zsLibEventingTool

#include "impl_org_webrtc_RTCDataChannelEvent.h"

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
wrapper::impl::org::webrtc::RTCDataChannelEvent::RTCDataChannelEvent() noexcept
{
}

//------------------------------------------------------------------------------
wrapper::org::webrtc::RTCDataChannelEventPtr wrapper::org::webrtc::RTCDataChannelEvent::wrapper_create() noexcept
{
  auto pThis = make_shared<wrapper::impl::org::webrtc::RTCDataChannelEvent>();
  pThis->thisWeak_ = pThis;
  return pThis;
}

//------------------------------------------------------------------------------
wrapper::impl::org::webrtc::RTCDataChannelEvent::~RTCDataChannelEvent() noexcept
{
  thisWeak_.reset();
}

//------------------------------------------------------------------------------
void wrapper::impl::org::webrtc::RTCDataChannelEvent::wrapper_init_org_webrtc_RTCDataChannelEvent() noexcept
{
}

//------------------------------------------------------------------------------
wrapper::org::webrtc::RTCDataChannelPtr wrapper::impl::org::webrtc::RTCDataChannelEvent::get_channel() noexcept
{
  wrapper::org::webrtc::RTCDataChannelPtr result {};
  return result;
}

