// Generated by zsLibEventingTool

#include "impl_org_webrtc_RTCRtpSendParameters.h"

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
wrapper::impl::org::webrtc::RTCRtpSendParameters::RTCRtpSendParameters() noexcept
{
}

//------------------------------------------------------------------------------
wrapper::org::webrtc::RTCRtpSendParametersPtr wrapper::org::webrtc::RTCRtpSendParameters::wrapper_create() noexcept
{
  auto pThis = make_shared<wrapper::impl::org::webrtc::RTCRtpSendParameters>();
  pThis->thisWeak_ = pThis;
  return pThis;
}

//------------------------------------------------------------------------------
wrapper::impl::org::webrtc::RTCRtpSendParameters::~RTCRtpSendParameters() noexcept
{
  thisWeak_.reset();
}

//------------------------------------------------------------------------------
void wrapper::impl::org::webrtc::RTCRtpSendParameters::wrapper_init_org_webrtc_RTCRtpSendParameters() noexcept
{
}

