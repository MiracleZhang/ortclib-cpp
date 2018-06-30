
#include "impl_org_ortc_RTCIceGathererCandidateEvent.h"
#include "impl_org_ortc_RTCIceCandidate.h"

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

ZS_DECLARE_TYPEDEF_PTR(wrapper::impl::org::ortc::RTCIceGathererCandidateEvent::WrapperImplType, WrapperImplType);
ZS_DECLARE_TYPEDEF_PTR(wrapper::impl::org::ortc::RTCIceGathererCandidateEvent::WrapperType, WrapperType);

//------------------------------------------------------------------------------
wrapper::impl::org::ortc::RTCIceGathererCandidateEvent::RTCIceGathererCandidateEvent() noexcept
{
}

//------------------------------------------------------------------------------
wrapper::org::ortc::RTCIceGathererCandidateEventPtr wrapper::org::ortc::RTCIceGathererCandidateEvent::wrapper_create() noexcept
{
  auto pThis = make_shared<wrapper::impl::org::ortc::RTCIceGathererCandidateEvent>();
  pThis->thisWeak_ = pThis;
  return pThis;
}

//------------------------------------------------------------------------------
wrapper::impl::org::ortc::RTCIceGathererCandidateEvent::~RTCIceGathererCandidateEvent() noexcept
{
  thisWeak_.reset();
}

//------------------------------------------------------------------------------
wrapper::org::ortc::RTCIceCandidatePtr wrapper::impl::org::ortc::RTCIceGathererCandidateEvent::get_candidate() noexcept
{
  return candidate_;
}

//------------------------------------------------------------------------------
String wrapper::impl::org::ortc::RTCIceGathererCandidateEvent::get_url() noexcept
{
  return url_;
}

//------------------------------------------------------------------------------
WrapperImplTypePtr WrapperImplType::toWrapper(
  ::ortc::IICEGathererTypes::CandidatePtr candidate,
  const String &url
) noexcept
{
  auto pThis = make_shared<WrapperImplType>();
  pThis->thisWeak_ = pThis;
  pThis->candidate_ = RTCIceCandidate::toWrapper(candidate);
  pThis->url_ = url;
  return pThis;
}
