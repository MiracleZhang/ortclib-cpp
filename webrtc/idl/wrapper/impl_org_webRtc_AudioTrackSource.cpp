
#include "impl_org_webrtc_AudioTrackSource.h"
#include "impl_org_webrtc_helpers.h"
#include "impl_org_webrtc_AudioOptions.h"
#include "impl_org_webrtc_MediaConstraints.h"
#include "impl_org_webrtc_WebrtcLib.h"

#include "impl_org_webrtc_pre_include.h"
#include "impl_org_webrtc_post_include.h"

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

// borrow types from call defintions
ZS_DECLARE_TYPEDEF_PTR(wrapper::impl::org::webrtc::AudioTrackSource::WrapperType, WrapperType);
ZS_DECLARE_TYPEDEF_PTR(wrapper::impl::org::webrtc::AudioTrackSource::WrapperImplType, WrapperImplType);
ZS_DECLARE_TYPEDEF_PTR(wrapper::impl::org::webrtc::AudioTrackSource::NativeType, NativeType);

typedef WrapperImplType::NativeScopedPtr NativeScopedPtr;

typedef wrapper::impl::org::webrtc::WrapperMapper<NativeType, WrapperImplType> UseWrapperMapper;

ZS_DECLARE_TYPEDEF_PTR(wrapper::impl::org::webrtc::WebRtcLib, UseWebrtcLib);
ZS_DECLARE_TYPEDEF_PTR(wrapper::impl::org::webrtc::MediaConstraints, UseMediaConstraints);
ZS_DECLARE_TYPEDEF_PTR(wrapper::impl::org::webrtc::AudioOptions, UseAudioOptions);

//------------------------------------------------------------------------------
static UseWrapperMapper &mapperSingleton()
{
  static UseWrapperMapper singleton;
  return singleton;
}

#if 0
//------------------------------------------------------------------------------
static NativeType *unproxy(NativeType *native)
{
  if (!native) return nullptr;
  auto converted = dynamic_cast<::webrtc::AudioSource *>(native);
  if (!converted) return nullptr;

  return WRAPPER_DEPROXIFY_CLASS(::webrtc, AudioSource, converted);
}
#endif //0

//------------------------------------------------------------------------------
wrapper::impl::org::webrtc::AudioTrackSource::AudioTrackSource() noexcept
{
}

//------------------------------------------------------------------------------
wrapper::org::webrtc::AudioTrackSourcePtr wrapper::org::webrtc::AudioTrackSource::wrapper_create() noexcept
{
  auto pThis = make_shared<wrapper::impl::org::webrtc::AudioTrackSource>();
  pThis->thisWeak_ = pThis;
  return pThis;
}

//------------------------------------------------------------------------------
wrapper::impl::org::webrtc::AudioTrackSource::~AudioTrackSource() noexcept
{
  thisWeak_.reset();
  teardownObserver();
}

//------------------------------------------------------------------------------
wrapper::org::webrtc::AudioTrackSourcePtr wrapper::org::webrtc::AudioTrackSource::create(wrapper::org::webrtc::AudioOptionsPtr options) noexcept
{
  auto factory = UseWebrtcLib::peerConnectionFactory();
  ZS_ASSERT(factory);
  if (!factory) return WrapperTypePtr();

  auto converted = UseAudioOptions::toNative(options);

  return WrapperImplType::toWrapper(factory->CreateAudioSource(*converted));
}
//------------------------------------------------------------------------------
wrapper::org::webrtc::AudioTrackSourcePtr wrapper::org::webrtc::AudioTrackSource::create(wrapper::org::webrtc::MediaConstraintsPtr constraints) noexcept
{
  auto factory = UseWebrtcLib::peerConnectionFactory();
  ZS_ASSERT(factory);
  if (!factory) return WrapperTypePtr();

  auto converted = UseMediaConstraints::toNative(constraints);

  return WrapperImplType::toWrapper(factory->CreateAudioSource(converted.get()));
}

//------------------------------------------------------------------------------
void wrapper::impl::org::webrtc::AudioTrackSource::wrapper_onObserverCountChanged(ZS_MAYBE_USED() size_t count) noexcept
{
  ZS_MAYBE_USED(count);
}

//------------------------------------------------------------------------------
wrapper::org::webrtc::MediaSourceState wrapper::impl::org::webrtc::AudioTrackSource::get_state() noexcept
{
#pragma ZS_BUILD_NOTE("TODO","(robin)")
  wrapper::org::webrtc::MediaSourceState result {};
  return result;
}

//------------------------------------------------------------------------------
bool wrapper::impl::org::webrtc::AudioTrackSource::get_remote() noexcept
{
  ZS_ASSERT(native_);
  if (!native_) return false;
  return native_->remote();
}

//------------------------------------------------------------------------------
double wrapper::impl::org::webrtc::AudioTrackSource::get_volume() noexcept
{
  ZS_ASSERT(native_);
  if (!native_) return 0.0;
  return lastVolume_;
}

//------------------------------------------------------------------------------
void wrapper::impl::org::webrtc::AudioTrackSource::set_volume(double value) noexcept
{
  ZS_ASSERT(native_);
  if (!native_) return;
  lastVolume_ = value;
  native_->SetVolume(value);
}

//------------------------------------------------------------------------------
void WrapperImplType::notifyVolume(double volume) noexcept
{
  onSetVolume(volume);
}


//------------------------------------------------------------------------------
void WrapperImplType::setupObserver()
{
  if (!native_) return;
  if (observer_) return;

  observer_ = std::make_unique<WebrtcObserver>(thisWeak_.lock());
  native_->RegisterAudioObserver(observer_.get());
}


//------------------------------------------------------------------------------
void WrapperImplType::teardownObserver()
{
  if (!observer_) return;
  if (!native_) return;

  native_->UnregisterAudioObserver(observer_.get());
}

//------------------------------------------------------------------------------
WrapperImplTypePtr WrapperImplType::toWrapper(NativeType *native) noexcept
{
  if (!native) return WrapperImplTypePtr();

  // search for original non-proxied pointer in map
  auto wrapper = mapperSingleton().getExistingOrCreateNew(native, [native]() {
    auto result = make_shared<WrapperImplType>();
    result->thisWeak_ = result;
    result->native_ = rtc::scoped_refptr<NativeType>(native); // only use proxy and never original pointer
    result->setupObserver();
    return result;
  });
  return wrapper;
}

//------------------------------------------------------------------------------
rtc::scoped_refptr<NativeType> WrapperImplType::toNative(WrapperTypePtr wrapper) noexcept
{
  if (!wrapper) return rtc::scoped_refptr<NativeType>();
  auto converted = ZS_DYNAMIC_PTR_CAST(WrapperImplType, wrapper);
  if (!converted) return rtc::scoped_refptr<NativeType>();
  return converted->native_;
}