#pragma once

#include <lvgl/lvgl.h>
#include <chrono>
#include <cstdint>
#include <optional>
#include "displayapp/screens/Screen.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "components/datetime/DateTimeController.h"
#include "components/ble/SimpleWeatherService.h"
#include "utility/DirtyValue.h"

namespace Pinetime {
  namespace Controllers {
    class Settings;
    class Battery;
    class HeartRateController;
    class MotionController;
    class NotificationManager;
  }

  namespace Applications {
    class DisplayApp;

    namespace Screens {

      // watchOS Modular style: date and time on top, a large weather module,
      // and a row of circular complications for steps, heart rate and battery.
      class WatchFaceModular : public Screen {
      public:
        WatchFaceModular(DisplayApp* app,
                         Controllers::DateTime& dateTimeController,
                         const Controllers::Battery& batteryController,
                         Controllers::NotificationManager& notificationManager,
                         Controllers::Settings& settingsController,
                         Controllers::HeartRateController& heartRateController,
                         Controllers::MotionController& motionController,
                         Controllers::SimpleWeatherService& weatherService);
        ~WatchFaceModular() override;

        void Refresh() override;
        void OnComplicationClicked(lv_obj_t* obj);

      private:
        struct Complication {
          lv_obj_t* base;
          lv_obj_t* arc;
          lv_obj_t* value;
        };

        Complication CreateComplication(uint8_t index, lv_color_t color);
        void UpdateWeather();

        DisplayApp* app;
        Controllers::DateTime& dateTimeController;
        const Controllers::Battery& batteryController;
        Controllers::NotificationManager& notificationManager;
        Controllers::Settings& settingsController;
        Controllers::HeartRateController& heartRateController;
        Controllers::MotionController& motionController;
        Controllers::SimpleWeatherService& weatherService;

        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::minutes>> currentDateTime {};
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::days>> currentDate {};
        Utility::DirtyValue<std::optional<Controllers::SimpleWeatherService::CurrentWeather>> currentWeather {};
        Utility::DirtyValue<bool> isNight {};
        Utility::DirtyValue<uint32_t> stepCount {};
        Utility::DirtyValue<uint8_t> heartbeat {};
        Utility::DirtyValue<bool> heartbeatRunning {};
        Utility::DirtyValue<uint8_t> batteryPercent {};
        Utility::DirtyValue<bool> charging {};
        Utility::DirtyValue<bool> notificationState {};

        lv_obj_t* labelDay;
        lv_obj_t* labelDate;
        lv_obj_t* labelTime;
        lv_obj_t* notificationDot;

        lv_obj_t* weatherModule;
        lv_obj_t* weatherHeader;
        lv_obj_t* weatherIcon;
        lv_obj_t* weatherMain;
        lv_obj_t* weatherDetail;

        Complication steps;
        Complication heart;
        lv_obj_t* heartIcon;
        Complication battery;

        lv_task_t* taskRefresh;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::Modular> {
      static constexpr WatchFace watchFace = WatchFace::Modular;
      static constexpr const char* name = "Modular";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceModular(controllers.displayApp,
                                             controllers.dateTimeController,
                                             controllers.batteryController,
                                             controllers.notificationManager,
                                             controllers.settingsController,
                                             controllers.heartRateController,
                                             controllers.motionController,
                                             *controllers.weatherController);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      }
    };
  }
}
