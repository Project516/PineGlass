#pragma once

#include <array>
#include <cstdint>
#include "displayapp/screens/Screen.h"
#include <lvgl/lvgl.h>
#include "components/brightness/BrightnessController.h"
#include "components/motor/MotorController.h"
#include "components/settings/Settings.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/alarm/AlarmController.h"
#include "systemtask/SystemTask.h"
#include "displayapp/widgets/PageIndicator.h"
#include "utility/DirtyValue.h"

namespace Pinetime {

  namespace Applications {
    namespace Screens {

      // Control Center: a status pill and pages of two-column pill toggles.
      class QuickSettings : public Screen {
      public:
        QuickSettings(DisplayApp* app,
                      const Pinetime::Controllers::Battery& batteryController,
                      Controllers::BrightnessController& brightness,
                      Controllers::MotorController& motorController,
                      Pinetime::Controllers::Settings& settingsController,
                      const Controllers::Ble& bleController,
                      const Controllers::AlarmController& alarmController,
                      System::SystemTask& systemTask);

        ~QuickSettings() override;

        void OnButtonEvent(lv_obj_t* object);
        bool OnTouchEvent(TouchEvents event) override;
        void Refresh() override;

      private:
        enum class Control : uint8_t { Bluetooth, Battery, DoNotDisturb, Sleep, Flashlight, Brightness, Settings, Lock };
        static constexpr uint8_t controlsPerPage = 6;
        static constexpr std::array<Control, 8> controls {Control::Bluetooth,
                                                          Control::Battery,
                                                          Control::DoNotDisturb,
                                                          Control::Sleep,
                                                          Control::Flashlight,
                                                          Control::Brightness,
                                                          Control::Settings,
                                                          Control::Lock};
        static constexpr uint8_t pageCount = (controls.size() + controlsPerPage - 1) / controlsPerPage;

        void ShowPage(uint8_t index);
        void UpdateControl(uint8_t slot);
        void UpdateStatus();
        void Update(bool force);

        DisplayApp* app;
        const Controllers::Battery& batteryController;
        Controllers::BrightnessController& brightness;
        Controllers::MotorController& motorController;
        Controllers::Settings& settingsController;
        const Controllers::Ble& bleController;
        const Controllers::AlarmController& alarmController;
        System::SystemTask& systemTask;

        Utility::DirtyValue<uint8_t> batteryPercent {};
        Utility::DirtyValue<bool> charging {};
        Utility::DirtyValue<bool> bleConnected {};
        Utility::DirtyValue<bool> alarmSet {};

        uint8_t page = 0;
        lv_task_t* taskRefresh;
        lv_obj_t* statusPill = nullptr;
        lv_obj_t* statusLabel = nullptr;
        std::array<lv_obj_t*, controlsPerPage> buttons {};
        std::array<lv_obj_t*, controlsPerPage> labels {};
        Widgets::PageIndicator pageIndicator;
      };
    }
  }
}
