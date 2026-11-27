#pragma once

#include <lvgl/lvgl.h>
#include <array>
#include <cstdint>
#include "displayapp/screens/Screen.h"
#include "displayapp/apps/Apps.h"
#include "components/datetime/DateTimeController.h"
#include "components/settings/Settings.h"
#include "displayapp/widgets/PageIndicator.h"
#include "utility/DirtyValue.h"

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

        // Rows alternate between 3 and 4 icons, offset like a honeycomb.
        static constexpr std::array<uint8_t, 4> rowSizes {3, 4, 3, 4};
        static constexpr uint8_t appsPerScreen = 14;

        Tile(uint8_t screenID,
             uint8_t numScreens,
             DisplayApp* app,
             Controllers::Settings& settingsController,
             Controllers::DateTime& dateTimeController,
             std::array<Applications, appsPerScreen>& applications);

        ~Tile() override;

        void Refresh() override;
        void OnClick(lv_obj_t* obj);

      private:
        DisplayApp* app;
        Controllers::DateTime& dateTimeController;

        lv_task_t* taskRefresh;
        lv_obj_t* labelTime;
        std::array<lv_obj_t*, appsPerScreen> buttons {};
        std::array<Pinetime::Applications::Apps, appsPerScreen> apps {};

        Widgets::PageIndicator pageIndicator;
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::minutes>> currentDateTime {};
      };
    }
  }
}
