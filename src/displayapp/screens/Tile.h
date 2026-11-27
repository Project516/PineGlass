#pragma once

#include <lvgl/lvgl.h>
#include <array>
#include <cstdint>
#include "displayapp/screens/Screen.h"
#include "displayapp/apps/Apps.h"
#include "components/settings/Settings.h"
#include "displayapp/widgets/PageIndicator.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {
      // One page of the honeycomb app grid.
      class Tile : public Screen {
      public:
        struct Applications {
          const char* icon;
          Pinetime::Applications::Apps application;
          bool enabled;
        };

        static constexpr uint8_t appsPerScreen = 14;

        Tile(uint8_t screenID,
             uint8_t numScreens,
             DisplayApp* app,
             Controllers::Settings& settingsController,
             std::array<Applications, appsPerScreen>& applications);

        ~Tile() override;

        void OnClick(lv_obj_t* obj);

      private:
        DisplayApp* app;

        std::array<lv_obj_t*, appsPerScreen> buttons {};
        std::array<Pinetime::Applications::Apps, appsPerScreen> apps {};

        Widgets::PageIndicator pageIndicator;
      };
    }
  }
}
