#include "displayapp/screens/settings/QuickSettings.h"
#include <cstdio>
#include "displayapp/DisplayApp.h"
#include "displayapp/Messages.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;

namespace {
  constexpr lv_coord_t pillWidth = 108;
  constexpr lv_coord_t pillHeight = 60;
  constexpr lv_coord_t gap = 8;
  constexpr lv_coord_t gridTop = 38;

  void ButtonEventHandler(lv_obj_t* obj, lv_event_t event) {
    if (event == LV_EVENT_CLICKED) {
      static_cast<QuickSettings*>(obj->user_data)->OnButtonEvent(obj);
    }
  }

  // Off: gray pill with a white glyph. On: filled pill, either a color with a
  // white glyph or white with a colored glyph, like the system toggles.
  void Style(lv_obj_t* button, lv_obj_t* label, bool on, lv_color_t fill, lv_color_t glyph) {
    lv_obj_set_style_local_bg_color(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, on ? fill : Colors::button);
    lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, on ? glyph : LV_COLOR_WHITE);
  }
}

QuickSettings::QuickSettings(Pinetime::Applications::DisplayApp* app,
                             const Pinetime::Controllers::Battery& batteryController,
                             Controllers::BrightnessController& brightness,
                             Controllers::MotorController& motorController,
                             Pinetime::Controllers::Settings& settingsController,
                             const Controllers::Ble& bleController,
                             const Controllers::AlarmController& alarmController,
                             System::SystemTask& systemTask)
  : app {app},
    batteryController {batteryController},
    brightness {brightness},
    motorController {motorController},
    settingsController {settingsController},
    bleController {bleController},
    alarmController {alarmController},
    systemTask {systemTask},
    pageIndicator(0, pageCount) {
  ShowPage(0);
  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
}

QuickSettings::~QuickSettings() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
  settingsController.SaveSettings();
}

void QuickSettings::ShowPage(uint8_t index) {
  lv_obj_clean(lv_scr_act());
  page = index;
  buttons.fill(nullptr);
  labels.fill(nullptr);

  statusPill = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_radius(statusPill, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_RADIUS_CIRCLE);
  lv_obj_set_style_local_bg_color(statusPill, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, Colors::platter);
  statusLabel = lv_label_create(statusPill, nullptr);
  lv_label_set_recolor(statusLabel, true);

  for (uint8_t slot = 0; slot < controlsPerPage; slot++) {
    const size_t control = page * controlsPerPage + slot;
    if (control >= controls.size()) {
      break;
    }
    lv_obj_t* button = lv_btn_create(lv_scr_act(), nullptr);
    button->user_data = this;
    lv_obj_set_event_cb(button, ButtonEventHandler);
    lv_obj_set_size(button, pillWidth, pillHeight);
    lv_obj_set_pos(button,
                   (LV_HOR_RES - 2 * pillWidth - gap) / 2 + (slot % 2) * (pillWidth + gap),
                   gridTop + (slot / 2) * (pillHeight + gap));
    lv_obj_set_style_local_pad_all(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_transform_width(button, LV_BTN_PART_MAIN, LV_STATE_PRESSED, -3);
    lv_obj_set_style_local_transform_height(button, LV_BTN_PART_MAIN, LV_STATE_PRESSED, -3);

    lv_obj_t* label = lv_label_create(button, nullptr);
    if (controls[control] != Control::Battery) {
      lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &material_icons_28);
    }
    buttons[slot] = button;
    labels[slot] = label;
    UpdateControl(slot);
  }

  if (pageCount > 1) {
    pageIndicator.Create();
    pageIndicator.SetPageIndicatorPosition(page);
  }
  Update(true);
}

void QuickSettings::UpdateControl(uint8_t slot) {
  using Notification = Controllers::Settings::Notification;
  lv_obj_t* button = buttons[slot];
  lv_obj_t* label = labels[slot];
  const auto notifications = settingsController.GetNotificationStatus();

  switch (controls[page * controlsPerPage + slot]) {
    case Control::Bluetooth: {
      const bool on = settingsController.GetBleRadioEnabled();
      lv_label_set_text_static(label, on ? Symbols::bluetoothOn : Symbols::bluetoothOff);
      Style(button, label, on, Colors::blue, LV_COLOR_WHITE);
    } break;
    case Control::Battery:
      Style(button, label, false, Colors::button, LV_COLOR_WHITE);
      break;
    case Control::DoNotDisturb:
      lv_label_set_text_static(label, Symbols::doNotDisturb);
      Style(button, label, notifications == Notification::Off, LV_COLOR_WHITE, Colors::indigo);
      break;
    case Control::Sleep:
      lv_label_set_text_static(label, Symbols::bed);
      Style(button, label, notifications == Notification::Sleep, LV_COLOR_WHITE, Colors::teal);
      break;
    case Control::Flashlight:
      lv_label_set_text_static(label, Symbols::flashlight);
      Style(button, label, false, Colors::button, LV_COLOR_WHITE);
      break;
    case Control::Brightness:
      lv_label_set_text_static(label, brightness.GetIcon());
      Style(button, label, false, Colors::button, LV_COLOR_WHITE);
      break;
    case Control::Settings:
      lv_label_set_text_static(label, Symbols::settings);
      Style(button, label, false, Colors::button, LV_COLOR_WHITE);
      break;
    case Control::Lock:
      lv_label_set_text_static(label, Symbols::lock);
      Style(button, label, false, Colors::button, LV_COLOR_WHITE);
      break;
  }
}

void QuickSettings::UpdateStatus() {
  char text[96];
  int length = snprintf(text,
                        sizeof(text),
                        "#%s %s#",
                        bleConnected.Get() ? "30D158" : "8E8E93",
                        bleConnected.Get() ? Symbols::phone : Symbols::phoneSlash);
  if (charging.Get()) {
    length += snprintf(text + length, sizeof(text) - length, "  #30D158 %s#", Symbols::plug);
  }
  if (alarmSet.Get()) {
    snprintf(text + length, sizeof(text) - length, "  #FF9F0A %s#", Symbols::bell);
  }
  lv_label_set_text(statusLabel, text);
  lv_obj_set_size(statusPill, lv_obj_get_width(statusLabel) + 28, 28);
  lv_obj_align(statusPill, nullptr, LV_ALIGN_IN_TOP_MID, 0, 4);
  lv_obj_align(statusLabel, nullptr, LV_ALIGN_CENTER, 0, 0);
}

void QuickSettings::Refresh() {
  Update(false);
}

void QuickSettings::Update(bool force) {
  bleConnected = bleController.IsConnected();
  alarmSet = alarmController.AnyAlarmEnabled();
  charging = batteryController.IsCharging();
  batteryPercent = batteryController.PercentRemaining();
  const bool statusChanged = bleConnected.IsUpdated() | alarmSet.IsUpdated();
  const bool chargingChanged = charging.IsUpdated();
  const bool percentChanged = batteryPercent.IsUpdated();

  if (force || statusChanged || chargingChanged) {
    UpdateStatus();
  }
  if (!(force || chargingChanged || percentChanged)) {
    return;
  }
  for (uint8_t slot = 0; slot < controlsPerPage; slot++) {
    if (labels[slot] == nullptr || controls[page * controlsPerPage + slot] != Control::Battery) {
      continue;
    }
    lv_label_set_text_fmt(labels[slot], "%d%%", batteryPercent.Get());
    lv_color_t color = LV_COLOR_WHITE;
    if (charging.Get()) {
      color = Colors::green;
    } else if (batteryPercent.Get() <= 20) {
      color = Colors::red;
    }
    lv_obj_set_style_local_text_color(labels[slot], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color);
  }
}

bool QuickSettings::OnTouchEvent(TouchEvents event) {
  if (event == TouchEvents::SwipeUp && page + 1 < pageCount) {
    app->SetFullRefresh(DisplayApp::FullRefreshDirections::Up);
    ShowPage(page + 1);
    return true;
  }
  if (event == TouchEvents::SwipeDown && page > 0) {
    app->SetFullRefresh(DisplayApp::FullRefreshDirections::Down);
    ShowPage(page - 1);
    return true;
  }
  return false;
}

void QuickSettings::OnButtonEvent(lv_obj_t* object) {
  using Notification = Controllers::Settings::Notification;

  uint8_t slot = 0;
  while (slot < controlsPerPage && buttons[slot] != object) {
    slot++;
  }
  if (slot == controlsPerPage) {
    return;
  }

  switch (controls[page * controlsPerPage + slot]) {
    case Control::Bluetooth:
      settingsController.SetBleRadioEnabled(!settingsController.GetBleRadioEnabled());
      app->PushMessage(Display::Messages::BleRadioEnableToggle);
      break;
    case Control::Battery:
      app->StartApp(Apps::BatteryInfo, DisplayApp::FullRefreshDirections::Up);
      return;
    case Control::DoNotDisturb:
    case Control::Sleep: {
      const Notification mode = controls[page * controlsPerPage + slot] == Control::Sleep ? Notification::Sleep : Notification::Off;
      const bool on = settingsController.GetNotificationStatus() == mode;
      settingsController.SetNotificationStatus(on ? Notification::On : mode);
      if (on) {
        motorController.RunForDuration(35);
      }
      // Do Not Disturb and Sleep share one setting, so refresh both.
      for (uint8_t i = 0; i < controlsPerPage; i++) {
        if (buttons[i] != nullptr) {
          UpdateControl(i);
        }
      }
      return;
    }
    case Control::Flashlight:
      app->StartApp(Apps::FlashLight, DisplayApp::FullRefreshDirections::Up);
      return;
    case Control::Brightness:
      brightness.Step();
      settingsController.SetBrightness(brightness.Level());
      break;
    case Control::Settings:
      settingsController.SetSettingsMenu(0);
      app->StartApp(Apps::Settings, DisplayApp::FullRefreshDirections::Up);
      return;
    case Control::Lock:
      systemTask.PushMessage(System::Messages::GoToSleep);
      return;
  }
  UpdateControl(slot);
}
