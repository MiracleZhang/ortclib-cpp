
#include "impl_org_ortc_RTCDtlsTransportStateChangeEvent.h"

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

ZS_DECLARE_TYPEDEF_PTR(wrapper::impl::org::ortc::RTCDtlsTransportStateChangeEvent::WrapperImplType, WrapperImplType);
ZS_DECLARE_TYPEDEF_PTR(wrapper::impl::org::ortc::RTCDtlsTransportStateChangeEvent::WrapperType, WrapperType);

//------------------------------------------------------------------------------
wrapper::impl::org::ortc::RTCDtlsTransportStateChangeEvent::RTCDtlsTransportStateChangeEvent() noexcept
{
}

//------------------------------------------------------------------------------
wrapper::org::ortc::RTCDtlsTransportStateChangeEventPtr wrapper::org::ortc::RTCDtlsTransportStateChangeEvent::wrapper_create() noexcept
{
  auto pThis = make_shared<wrapper::impl::org::ortc::RTCDtlsTransportStateChangeEvent>();
  pThis->thisWeak_ = pThis;
  return pThis;
}

//------------------------------------------------------------------------------
wrapper::impl::org::ortc::RTCDtlsTransportStateChangeEvent::~RTCDtlsTransportStateChangeEvent() noexcept
{
  thisWeak_.reset();
}

//------------------------------------------------------------------------------
wrapper::org::ortc::RTCDtlsTransportState wrapper::impl::org::ortc::RTCDtlsTransportStateChangeEvent::get_state() noexcept
{
  return state_;
}

//------------------------------------------------------------------------------
WrapperImplTypePtr WrapperImplType::toWrapper(wrapper::org::ortc::RTCDtlsTransportState state) noexcept
{
  auto pThis = make_shared<WrapperImplType>();
  pThis->thisWeak_ = pThis;
  pThis->state_ = state;
  return pThis;
}
