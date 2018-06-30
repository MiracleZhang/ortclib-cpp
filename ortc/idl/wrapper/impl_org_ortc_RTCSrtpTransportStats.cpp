
#include "impl_org_ortc_RTCSrtpTransportStats.h"
#include "impl_org_ortc_Helper.h"
#include "impl_org_ortc_Json.h"

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
wrapper::impl::org::ortc::RTCSrtpTransportStats::RTCSrtpTransportStats() noexcept
{
}

//------------------------------------------------------------------------------
wrapper::org::ortc::RTCSrtpTransportStatsPtr wrapper::org::ortc::RTCSrtpTransportStats::wrapper_create() noexcept
{
  auto pThis = make_shared<wrapper::impl::org::ortc::RTCSrtpTransportStats>();
  pThis->thisWeak_ = pThis;
  return pThis;
}

//------------------------------------------------------------------------------
wrapper::impl::org::ortc::RTCSrtpTransportStats::~RTCSrtpTransportStats() noexcept
{
  thisWeak_.reset();
}

//------------------------------------------------------------------------------
wrapper::org::ortc::JsonPtr wrapper::impl::org::ortc::RTCSrtpTransportStats::toJson() noexcept
{
  return Json::toWrapper(native_->createElement("RTCSrtpTransportStats"));
}

//------------------------------------------------------------------------------
String wrapper::impl::org::ortc::RTCSrtpTransportStats::hash() noexcept
{
  return native_->hash();
}

//------------------------------------------------------------------------------
::zsLib::Time wrapper::impl::org::ortc::RTCSrtpTransportStats::get_timestamp() noexcept
{
  return native_->mTimestamp;
}

//------------------------------------------------------------------------------
Optional< wrapper::org::ortc::RTCStatsType > wrapper::impl::org::ortc::RTCSrtpTransportStats::get_statsType() noexcept
{
  if (!native_->mStatsType.hasValue()) return Optional< wrapper::org::ortc::RTCStatsType >();
  return Helper::toWrapper(native_->mStatsType.value());
}

//------------------------------------------------------------------------------
String wrapper::impl::org::ortc::RTCSrtpTransportStats::get_statsTypeOther() noexcept
{
  return native_->mStatsTypeOther;
}

//------------------------------------------------------------------------------
String wrapper::impl::org::ortc::RTCSrtpTransportStats::get_id() noexcept
{
  return native_->mID;
}

//------------------------------------------------------------------------------
void wrapper::impl::org::ortc::RTCSrtpTransportStats::wrapper_init_org_ortc_RTCSrtpTransportStats() noexcept
{
  native_ = make_shared<NativeStats>();
}

//------------------------------------------------------------------------------
void wrapper::impl::org::ortc::RTCSrtpTransportStats::wrapper_init_org_ortc_RTCSrtpTransportStats(wrapper::org::ortc::RTCSrtpTransportStatsPtr source) noexcept
{
  if (!source) {
    wrapper_init_org_ortc_RTCSrtpTransportStats();
    return;
  }
  native_ = NativeStats::create(*toNative(source));
}

//------------------------------------------------------------------------------
void wrapper::impl::org::ortc::RTCSrtpTransportStats::wrapper_init_org_ortc_RTCSrtpTransportStats(wrapper::org::ortc::JsonPtr json) noexcept
{
  native_ = NativeStats::create(Json::toNative(json));
}


//------------------------------------------------------------------------------
wrapper::impl::org::ortc::RTCSrtpTransportStatsPtr wrapper::impl::org::ortc::RTCSrtpTransportStats::toWrapper(NativeStatsPtr native) noexcept
{
  if (!native) return RTCSrtpTransportStatsPtr();

  auto pThis = make_shared<wrapper::impl::org::ortc::RTCSrtpTransportStats>();
  pThis->thisWeak_ = pThis;
  pThis->native_ = native;
  return pThis;
}

//------------------------------------------------------------------------------
wrapper::impl::org::ortc::RTCSrtpTransportStats::NativeStatsPtr wrapper::impl::org::ortc::RTCSrtpTransportStats::toNative(wrapper::org::ortc::RTCSrtpTransportStatsPtr wrapper) noexcept
{
  if (!wrapper) return NativeStatsPtr();
  return std::dynamic_pointer_cast<RTCSrtpTransportStats>(wrapper)->native_;
}

