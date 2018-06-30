
#include "impl_org_webrtc_VideoCapturer.h"
#include "impl_org_webrtc_WebrtcLib.h"

#include "impl_org_webrtc_pre_include.h"
#ifdef WINUWP
#include "media/engine/webrtcvideocapturer.h"
#endif //WINUWP
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
ZS_DECLARE_TYPEDEF_PTR(wrapper::impl::org::webrtc::VideoCapturer::WrapperType, WrapperType);
ZS_DECLARE_TYPEDEF_PTR(wrapper::impl::org::webrtc::VideoCapturer::WrapperImplType, WrapperImplType);
ZS_DECLARE_TYPEDEF_PTR(wrapper::impl::org::webrtc::VideoCapturer::NativeType, NativeType);

ZS_DECLARE_TYPEDEF_PTR(wrapper::impl::org::webrtc::WebRtcLib, UseWebrtcLib);

//------------------------------------------------------------------------------
wrapper::impl::org::webrtc::VideoCapturer::VideoCapturer() noexcept
{
}

//------------------------------------------------------------------------------
wrapper::org::webrtc::VideoCapturerPtr wrapper::org::webrtc::VideoCapturer::wrapper_create() noexcept
{
  auto pThis = make_shared<wrapper::impl::org::webrtc::VideoCapturer>();
  pThis->thisWeak_ = pThis;
  return pThis;
}

//------------------------------------------------------------------------------
wrapper::impl::org::webrtc::VideoCapturer::~VideoCapturer() noexcept
{
  thisWeak_.reset();
}

//------------------------------------------------------------------------------
wrapper::org::webrtc::VideoCapturerPtr wrapper::org::webrtc::VideoCapturer::create(
  String name,
  String id
  ) noexcept
{
  auto factory = UseWebrtcLib::videoDeviceCaptureFactory();
  if (!factory) return WrapperTypePtr();

  ::cricket::Device device(name, id);
  auto native = factory->Create(device);
  if (!native) return WrapperTypePtr();

  auto result = make_shared<WrapperImplType>();
  result->thisWeak_ = result;
  result->native_ = std::move(native);
  return result;
}

//------------------------------------------------------------------------------
shared_ptr< list< wrapper::org::webrtc::VideoFormatPtr > > wrapper::impl::org::webrtc::VideoCapturer::getSupportedFormats() noexcept
{
  if (!native_) {
    ZS_ASSERT_FAIL("Cannot call into VideoCapturer after VideoCapturer has been passed into to a VideoTrackSource.");
    return make_shared< list< wrapper::org::webrtc::VideoFormatPtr > >();
  }
#pragma ZS_BUILD_NOTE("TODO","(robin)")

  shared_ptr< list< wrapper::org::webrtc::VideoFormatPtr > > result {};
  return result;
}

//------------------------------------------------------------------------------
wrapper::org::webrtc::VideoFormatPtr wrapper::impl::org::webrtc::VideoCapturer::getBestCaptureFormat(wrapper::org::webrtc::VideoFormatPtr desired) noexcept
{
  if (!native_) {
    ZS_ASSERT_FAIL("Cannot call into VideoCapturer after VideoCapturer has been passed into to a VideoTrackSource.");
    return wrapper::org::webrtc::VideoFormatPtr();
  }
#pragma ZS_BUILD_NOTE("TODO","(robin)")

  wrapper::org::webrtc::VideoFormatPtr result {};
  return result;
}

//------------------------------------------------------------------------------
wrapper::org::webrtc::VideoCaptureState wrapper::impl::org::webrtc::VideoCapturer::start(wrapper::org::webrtc::VideoFormatPtr captureFormat) noexcept
{
  if (!native_) {
    ZS_ASSERT_FAIL("Cannot call into VideoCapturer after VideoCapturer has been passed into to a VideoTrackSource.");
    return wrapper::org::webrtc::VideoCaptureState::VideoCaptureState_failed;
  }
#pragma ZS_BUILD_NOTE("TODO","(robin)")

  wrapper::org::webrtc::VideoCaptureState result {};
  return result;
}

//------------------------------------------------------------------------------
wrapper::org::webrtc::VideoFormatPtr wrapper::impl::org::webrtc::VideoCapturer::getCaptureFormat() noexcept
{
  if (!native_) {
    ZS_ASSERT_FAIL("Cannot call into VideoCapturer after VideoCapturer has been passed into to a VideoTrackSource.");
    return wrapper::org::webrtc::VideoFormatPtr();
  }
#pragma ZS_BUILD_NOTE("TODO","(robin)")
  wrapper::org::webrtc::VideoFormatPtr result {};
  return result;
}

//------------------------------------------------------------------------------
void wrapper::impl::org::webrtc::VideoCapturer::stop() noexcept
{
  if (!native_) {
    ZS_ASSERT_FAIL("Cannot call into VideoCapturer after VideoCapturer has been passed into to a VideoTrackSource.");
    return;
  }
  native_->Stop();
}

//------------------------------------------------------------------------------
void wrapper::impl::org::webrtc::VideoCapturer::constrainSupportedFormats(wrapper::org::webrtc::VideoFormatPtr maxFormat) noexcept
{
  if (!native_) {
    ZS_ASSERT_FAIL("Cannot call into VideoCapturer after VideoCapturer has been passed into to a VideoTrackSource.");
    return;
  }
}

//------------------------------------------------------------------------------
String wrapper::impl::org::webrtc::VideoCapturer::get_id() noexcept
{
  if (!native_) {
    ZS_ASSERT_FAIL("Cannot call into VideoCapturer after VideoCapturer has been passed into to a VideoTrackSource.");
    return String();
  }
  return native_->GetId();
}

//------------------------------------------------------------------------------
bool wrapper::impl::org::webrtc::VideoCapturer::get_enableCameraList() noexcept
{
  if (!native_) {
    ZS_ASSERT_FAIL("Cannot call into VideoCapturer after VideoCapturer has been passed into to a VideoTrackSource.");
    return false;
  }
  return native_->enable_camera_list();
}

//------------------------------------------------------------------------------
void wrapper::impl::org::webrtc::VideoCapturer::set_enableCameraList(bool value) noexcept
{
  if (!native_) {
    ZS_ASSERT_FAIL("Cannot call into VideoCapturer after VideoCapturer has been passed into to a VideoTrackSource.");
    return;
  }
  return native_->set_enable_camera_list(value);
}

//------------------------------------------------------------------------------
bool wrapper::impl::org::webrtc::VideoCapturer::get_enableVideoAdapter() noexcept
{
  if (!native_) {
    ZS_ASSERT_FAIL("Cannot call into VideoCapturer after VideoCapturer has been passed into to a VideoTrackSource.");
    return false;
  }
  return native_->enable_video_adapter();
}

//------------------------------------------------------------------------------
void wrapper::impl::org::webrtc::VideoCapturer::set_enableVideoAdapter(bool value) noexcept
{
  if (!native_) {
    ZS_ASSERT_FAIL("Cannot call into VideoCapturer after VideoCapturer has been passed into to a VideoTrackSource.");
    return;
  }
  native_->set_enable_video_adapter(value);
}

//------------------------------------------------------------------------------
bool wrapper::impl::org::webrtc::VideoCapturer::get_isRunning() noexcept
{
  if (!native_) {
    ZS_ASSERT_FAIL("Cannot call into VideoCapturer after VideoCapturer has been passed into to a VideoTrackSource.");
    return false;
  }
  return native_->IsRunning();
}

//------------------------------------------------------------------------------
bool wrapper::impl::org::webrtc::VideoCapturer::get_applyRotation() noexcept
{
  if (!native_) {
    ZS_ASSERT_FAIL("Cannot call into VideoCapturer after VideoCapturer has been passed into to a VideoTrackSource.");
    return false;
  }
  return native_->apply_rotation();
}

//------------------------------------------------------------------------------
bool wrapper::impl::org::webrtc::VideoCapturer::get_isScreencast() noexcept
{
  if (!native_) {
    ZS_ASSERT_FAIL("Cannot call into VideoCapturer after VideoCapturer has been passed into to a VideoTrackSource.");
    return false;
  }
  return native_->IsScreencast();
}

//------------------------------------------------------------------------------
wrapper::org::webrtc::VideoCapturerInputSizePtr wrapper::impl::org::webrtc::VideoCapturer::get_inputSize() noexcept
{
  if (!native_) {
    ZS_ASSERT_FAIL("Cannot call into VideoCapturer after VideoCapturer has been passed into to a VideoTrackSource.");
    return wrapper::org::webrtc::VideoCapturerInputSizePtr();
  }
#pragma ZS_BUILD_NOTE("TODO","(robin)")
  wrapper::org::webrtc::VideoCapturerInputSizePtr result {};
  return result;
}

//------------------------------------------------------------------------------
wrapper::org::webrtc::VideoCaptureState wrapper::impl::org::webrtc::VideoCapturer::get_state() noexcept
{
  if (!native_) {
    ZS_ASSERT_FAIL("Cannot call into VideoCapturer after VideoCapturer has been passed into to a VideoTrackSource.");
    return wrapper::org::webrtc::VideoCaptureState::VideoCaptureState_failed;
  }
#pragma ZS_BUILD_NOTE("TODO","(robin)")
  wrapper::org::webrtc::VideoCaptureState result {};
  return result;
}

//------------------------------------------------------------------------------
WrapperImplTypePtr WrapperImplType::toWrapper(NativeTypeUniPtr native) noexcept
{
  if (!native) return WrapperImplTypePtr();

  auto result = make_shared<WrapperImplType>();
  result->thisWeak_ = result;
  result->native_ = std::move(native);
  return result;
}

//------------------------------------------------------------------------------
NativeTypeUniPtr WrapperImplType::toNative(WrapperTypePtr wrapper) noexcept
{
  if (!wrapper) return NativeTypeUniPtr();
  auto converted = ZS_DYNAMIC_PTR_CAST(WrapperImplType, wrapper);
  if (!converted) return NativeTypeUniPtr();
  return NativeTypeUniPtr(std::move(converted->native_));
}