// Generated by zsLibEventingTool

#include "impl_org_webrtc_RTCPeerConnectionIceErrorEvent.h"

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
wrapper::impl::org::webrtc::RTCPeerConnectionIceErrorEvent::RTCPeerConnectionIceErrorEvent() noexcept
{
}

//------------------------------------------------------------------------------
wrapper::org::webrtc::RTCPeerConnectionIceErrorEventPtr wrapper::org::webrtc::RTCPeerConnectionIceErrorEvent::wrapper_create() noexcept
{
  auto pThis = make_shared<wrapper::impl::org::webrtc::RTCPeerConnectionIceErrorEvent>();
  pThis->thisWeak_ = pThis;
  return pThis;
}

//------------------------------------------------------------------------------
wrapper::impl::org::webrtc::RTCPeerConnectionIceErrorEvent::~RTCPeerConnectionIceErrorEvent() noexcept
{
  thisWeak_.reset();
}

//------------------------------------------------------------------------------
void wrapper::impl::org::webrtc::RTCPeerConnectionIceErrorEvent::wrapper_init_org_webrtc_RTCPeerConnectionIceErrorEvent() noexcept
{
}

//------------------------------------------------------------------------------
String wrapper::impl::org::webrtc::RTCPeerConnectionIceErrorEvent::get_hostCandidate() noexcept
{
  String result {};
  return result;
}

//------------------------------------------------------------------------------
String wrapper::impl::org::webrtc::RTCPeerConnectionIceErrorEvent::get_url() noexcept
{
  String result {};
  return result;
}

//------------------------------------------------------------------------------
unsigned short wrapper::impl::org::webrtc::RTCPeerConnectionIceErrorEvent::get_errorCode() noexcept
{
  unsigned short result {};
  return result;
}

//------------------------------------------------------------------------------
String wrapper::impl::org::webrtc::RTCPeerConnectionIceErrorEvent::get_errorText() noexcept
{
  String result {};
  return result;
}

