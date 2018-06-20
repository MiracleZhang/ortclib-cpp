
#pragma once

#include "types.h"

#include "Org/Ortc/MediaControl.g.h"
#include <wrapper/generated/org_ortc_MediaControl.h>

namespace winrt {
  namespace Org {
    namespace Ortc {
      namespace implementation {


        /// <summary>
        /// Interface for controlling the behavior of media.
        /// </summary>
        struct MediaControl : MediaControlT<MediaControl>
        {
          // internal
          wrapper::org::ortc::MediaControlPtr native_;

          struct WrapperCreate {};
          MediaControl(const WrapperCreate &) {}

          static winrt::com_ptr< Org::Ortc::implementation::MediaControl > ToCppWinrtImpl(wrapper::org::ortc::MediaControlPtr value);
          static winrt::com_ptr< Org::Ortc::implementation::MediaControl > ToCppWinrtImpl(Org::Ortc::MediaControl const & value);
          static winrt::com_ptr< Org::Ortc::implementation::MediaControl > ToCppWinrtImpl(winrt::com_ptr< Org::Ortc::implementation::MediaControl > const & value) { return value; }
          static winrt::com_ptr< Org::Ortc::implementation::MediaControl > ToCppWinrtImpl(Org::Ortc::IMediaControl const & value);
          static Org::Ortc::MediaControl ToCppWinrt(wrapper::org::ortc::MediaControlPtr value);
          static Org::Ortc::MediaControl ToCppWinrt(Org::Ortc::MediaControl const & value) { return value; }
          static Org::Ortc::MediaControl ToCppWinrt(winrt::com_ptr< Org::Ortc::implementation::MediaControl > const & value);
          static Org::Ortc::MediaControl ToCppWinrt(Org::Ortc::IMediaControl const & value);
          static Org::Ortc::IMediaControl ToCppWinrtInterface(wrapper::org::ortc::MediaControlPtr value);
          static Org::Ortc::IMediaControl ToCppWinrtInterface(Org::Ortc::MediaControl const & value);
          static Org::Ortc::IMediaControl ToCppWinrtInterface(winrt::com_ptr< Org::Ortc::implementation::MediaControl > const & value);
          static Org::Ortc::IMediaControl ToCppWinrtInterface(Org::Ortc::IMediaControl const & value) { return value; }
          static wrapper::org::ortc::MediaControlPtr FromCppWinrt(wrapper::org::ortc::MediaControlPtr value) { return value; }
          static wrapper::org::ortc::MediaControlPtr FromCppWinrt(winrt::com_ptr< Org::Ortc::implementation::MediaControl > const & value);
          static wrapper::org::ortc::MediaControlPtr FromCppWinrt(Org::Ortc::MediaControl const & value);
          static wrapper::org::ortc::MediaControlPtr FromCppWinrt(Org::Ortc::IMediaControl const & value);



        public:
          /// <summary>
          /// Cast from Org::Ortc::IMediaControl to MediaControl
          /// </summary>
          static Org::Ortc::MediaControl CastFromIMediaControl(Org::Ortc::IMediaControl const & value);

          /// <summary>
          /// Gets or sets the media engine the application orientation has
          /// changed.
          /// </summary>
          static Windows::Foundation::IInspectable DisplayOrientation();
          static void DisplayOrientation(Windows::Foundation::IInspectable const & value);

        };

      } // namepsace implementation

      namespace factory_implementation {

        struct MediaControl : MediaControlT<MediaControl, implementation::MediaControl>
        {
        };

      } // namespace factory_implementation

    } // namespace Ortc
  } // namespace Org
} // namespace winrt