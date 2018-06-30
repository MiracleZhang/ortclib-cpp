// Generated by zsLibEventingTool

#include "impl_org_webrtc_RTCStatsReport.h"

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
wrapper::impl::org::webrtc::RTCStatsReport::RTCStatsReport() noexcept
{
}

//------------------------------------------------------------------------------
wrapper::org::webrtc::RTCStatsReportPtr wrapper::org::webrtc::RTCStatsReport::wrapper_create() noexcept
{
  auto pThis = make_shared<wrapper::impl::org::webrtc::RTCStatsReport>();
  pThis->thisWeak_ = pThis;
  return pThis;
}

//------------------------------------------------------------------------------
wrapper::impl::org::webrtc::RTCStatsReport::~RTCStatsReport() noexcept
{
  thisWeak_.reset();
}

//------------------------------------------------------------------------------
wrapper::org::webrtc::RTCStatsPtr wrapper::impl::org::webrtc::RTCStatsReport::getStats(String id) noexcept
{
  wrapper::org::webrtc::RTCStatsPtr result {};
  return result;
}

//------------------------------------------------------------------------------
uint64_t wrapper::impl::org::webrtc::RTCStatsReport::get_objectId() noexcept
{
  uint64_t result {};
  return result;
}

//------------------------------------------------------------------------------
shared_ptr< list< String > > wrapper::impl::org::webrtc::RTCStatsReport::get_statsIds() noexcept
{
  shared_ptr< list< String > > result {};
  return result;
}

