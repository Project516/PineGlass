#include "displayapp/screens/WatchFaceModular.h"

#include <cctype>
#include <cstdio>
#include "displayapp/DisplayApp.h"
#include "displayapp/InfiniTimeTheme.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/screens/WeatherSymbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"

using namespace Pinetime::Applications::Screens;

namespace {
  constexpr lv_coord_t complicationSize = 64;
  constexpr lv_coord_t complicationTop = 166;
  constexpr lv_coord_t ringWidth = 5;

  void ClickHandler(lv_obj_t* obj, lv_event_t event) {
    if (event == LV_EVENT_CLICKED) {
      static_cast<WatchFaceModular*>(obj->user_data)->OnComplicationClicked(obj);
    }
  }

  const char* ConditionName(Pinetime::Controllers::SimpleWeatherService::Icons icon) {
    using Icons = Pinetime::Controllers::SimpleWeatherService::Icons;
    switch (icon) {
      case Icons::Sun:
        return "Clear";
      case Icons::CloudsSun:
        return "Partly Cloudy";
      case Icons::Clouds:
        return "Cloudy";
      case Icons::BrokenClouds:
        return "Mostly Cloudy";
      case Icons::CloudShowerHeavy:
        return "Heavy Rain";
      case Icons::CloudSunRain:
        return "Rain";
      case Icons::Thunderstorm:
        return "Thunderstorm";
      case Icons::Snow:
        return "Snow";
      case Icons::Smog:
        return "Fog";
      default:
        return "";
    }
  }

  void MakeClickable(lv_obj_t* obj, WatchFaceModular* screen) {
    obj->user_data = screen;
    lv_obj_set_click(obj, true);
    lv_obj_set_event_cb(obj, ClickHandler);
  }
}

WatchFaceModular::WatchFaceModular(DisplayApp* app,
                                   Controllers::DateTime& dateTimeController,
                                   const Controllers::Battery& batteryController,
                                   Controllers::NotificationManager& notificationManager,
                                   Controllers::Settings& settingsController,
                                   Controllers::HeartRateController& heartRateController,
                                   Controllers::MotionController& motionController,
                                   Controllers::SimpleWeatherService& weatherService)
  : app {app},
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController},
    weatherService {weatherService} {

  labelDay = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(labelDay, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::red);
  labelDate = lv_label_create(lv_scr_act(), nullptr);

  labelTime = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(labelTime, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &inter_medium_42);

  notificationDot = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(notificationDot, 8, 8);
  lv_obj_align(notificationDot, nullptr, LV_ALIGN_IN_TOP_MID, 0, 4);
  lv_obj_set_style_local_radius(notificationDot, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_RADIUS_CIRCLE);
  lv_obj_set_style_local_bg_color(notificationDot, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, Colors::red);
  lv_obj_set_hidden(notificationDot, true);

  weatherModule = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(weatherModule, LV_HOR_RES - 24, 90);
  lv_obj_set_pos(weatherModule, 12, 64);
  lv_obj_set_style_local_bg_opa(weatherModule, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
  MakeClickable(weatherModule, this);

  weatherHeader = lv_label_create(weatherModule, nullptr);
  lv_obj_set_style_local_text_color(weatherHeader, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::cyan);
  lv_label_set_long_mode(weatherHeader, LV_LABEL_LONG_DOT);
  lv_obj_set_width(weatherHeader, LV_HOR_RES - 24);
  lv_obj_set_pos(weatherHeader, 0, 0);

  weatherIcon = lv_label_create(weatherModule, nullptr);
  lv_obj_set_style_local_text_font(weatherIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &fontawesome_weathericons);
  lv_label_set_text_static(weatherIcon, "");
  lv_obj_set_pos(weatherIcon, 0, 30);

  weatherMain = lv_label_create(weatherModule, nullptr);
  lv_label_set_long_mode(weatherMain, LV_LABEL_LONG_DOT);
  lv_obj_set_width(weatherMain, LV_HOR_RES - 24 - 36);
  lv_obj_set_pos(weatherMain, 36, 31);

  weatherDetail = lv_label_create(weatherModule, nullptr);
  lv_obj_set_style_local_text_color(weatherDetail, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::secondaryLabel);
  lv_obj_set_pos(weatherDetail, 0, 60);

  steps = CreateComplication(0, Colors::move);
  heart = CreateComplication(1, Colors::red);
  battery = CreateComplication(2, Colors::green);

  heartIcon = lv_label_create(heart.base, nullptr);
  lv_obj_set_style_local_text_color(heartIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::red);
  lv_label_set_text_static(heartIcon, Symbols::heartBeat);
  lv_obj_align(heartIcon, nullptr, LV_ALIGN_CENTER, 0, -11);

  Refresh();
  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
}

WatchFaceModular::~WatchFaceModular() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

WatchFaceModular::Complication WatchFaceModular::CreateComplication(uint8_t index, lv_color_t color) {
  constexpr lv_coord_t gap = (LV_HOR_RES_MAX - 24 - 3 * complicationSize) / 2;
  Complication c {};

  c.base = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(c.base, complicationSize, complicationSize);
  lv_obj_set_pos(c.base, 12 + index * (complicationSize + gap), complicationTop);
  lv_obj_set_style_local_radius(c.base, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_RADIUS_CIRCLE);
  lv_obj_set_style_local_bg_color(c.base, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, Colors::bgDark);
  MakeClickable(c.base, this);

  // Heart rate shows an icon instead of a ring.
  if (index != 1) {
    c.arc = lv_arc_create(c.base, nullptr);
    lv_obj_set_size(c.arc, complicationSize, complicationSize);
    lv_obj_align(c.arc, nullptr, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_click(c.arc, false);
    lv_arc_set_bg_angles(c.arc, 0, 360);
    lv_arc_set_rotation(c.arc, 270);
    lv_arc_set_range(c.arc, 0, 100);
    lv_obj_set_style_local_bg_opa(c.arc, LV_ARC_PART_BG, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_pad_all(c.arc, LV_ARC_PART_BG, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_line_width(c.arc, LV_ARC_PART_BG, LV_STATE_DEFAULT, ringWidth);
    lv_obj_set_style_local_line_color(c.arc, LV_ARC_PART_BG, LV_STATE_DEFAULT, lv_color_mix(color, LV_COLOR_BLACK, LV_OPA_30));
    lv_obj_set_style_local_line_width(c.arc, LV_ARC_PART_INDIC, LV_STATE_DEFAULT, ringWidth);
    lv_obj_set_style_local_line_color(c.arc, LV_ARC_PART_INDIC, LV_STATE_DEFAULT, color);
  }

  c.value = lv_label_create(c.base, nullptr);
  lv_label_set_text_static(c.value, "");
  return c;
}

void WatchFaceModular::OnComplicationClicked(lv_obj_t* obj) {
  Apps target = Apps::None;
  if (obj == weatherModule) {
    target = Apps::Weather;
  } else if (obj == steps.base) {
    target = Apps::Steps;
  } else if (obj == heart.base) {
    target = Apps::HeartRate;
  } else if (obj == battery.base) {
    target = Apps::BatteryInfo;
  }
  if (target != Apps::None && app->IsAppAvailable(target)) {
    app->StartApp(target, DisplayApp::FullRefreshDirections::Up);
  }
}

void WatchFaceModular::Refresh() {
  currentDateTime = std::chrono::time_point_cast<std::chrono::minutes>(dateTimeController.CurrentDateTime());
  if (currentDateTime.IsUpdated()) {
    uint8_t hour = dateTimeController.Hours();
    if (settingsController.GetClockType() == Controllers::Settings::ClockType::H12) {
      hour = hour % 12 == 0 ? 12 : hour % 12;
      lv_label_set_text_fmt(labelTime, "%d:%02d", hour, dateTimeController.Minutes());
    } else {
      lv_label_set_text_fmt(labelTime, "%02d:%02d", hour, dateTimeController.Minutes());
    }
    lv_obj_align(labelTime, nullptr, LV_ALIGN_IN_TOP_RIGHT, -10, 2);

    currentDate = std::chrono::time_point_cast<std::chrono::days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      lv_label_set_text_static(labelDay, dateTimeController.DayOfWeekShortToString());
      lv_label_set_text_fmt(labelDate, "%d", dateTimeController.Day());
      lv_obj_set_pos(labelDay, 12, 18);
      lv_obj_align(labelDate, labelDay, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    }
  }

  notificationState = notificationManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    lv_obj_set_hidden(notificationDot, !notificationState.Get());
  }

  currentWeather = weatherService.Current();
  isNight = weatherService.IsNight();
  if (currentWeather.IsUpdated() | isNight.IsUpdated()) {
    UpdateWeather();
  }

  stepCount = motionController.NbSteps();
  if (stepCount.IsUpdated()) {
    const uint32_t count = stepCount.Get();
    const uint32_t goal = settingsController.GetStepsGoal();
    lv_arc_set_value(steps.arc, goal == 0 ? 0 : std::min<uint32_t>(count * 100 / goal, 100));
    if (count >= 10000) {
      lv_label_set_text_fmt(steps.value, "%luk", count / 1000);
    } else {
      lv_label_set_text_fmt(steps.value, "%lu", count);
    }
    lv_obj_align(steps.value, nullptr, LV_ALIGN_CENTER, 0, 0);
  }

  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get() && heartbeat.Get() > 0) {
      lv_label_set_text_fmt(heart.value, "%d", heartbeat.Get());
    } else {
      lv_label_set_text_static(heart.value, "--");
    }
    lv_obj_align(heart.value, nullptr, LV_ALIGN_CENTER, 0, 12);
  }

  batteryPercent = batteryController.PercentRemaining();
  charging = batteryController.IsCharging();
  if (batteryPercent.IsUpdated() || charging.IsUpdated()) {
    const uint8_t percent = batteryPercent.Get();
    lv_arc_set_value(battery.arc, percent);
    const lv_color_t color = (percent <= 20 && !charging.Get()) ? Colors::red : Colors::green;
    lv_obj_set_style_local_line_color(battery.arc, LV_ARC_PART_INDIC, LV_STATE_DEFAULT, color);
    lv_obj_set_style_local_line_color(battery.arc, LV_ARC_PART_BG, LV_STATE_DEFAULT, lv_color_mix(color, LV_COLOR_BLACK, LV_OPA_30));
    lv_label_set_text_fmt(battery.value, "%d", percent);
    lv_obj_align(battery.value, nullptr, LV_ALIGN_CENTER, 0, 0);
  }
}

void WatchFaceModular::UpdateWeather() {
  const auto& weather = currentWeather.Get();
  if (!weather) {
    lv_label_set_text_static(weatherHeader, "WEATHER");
    lv_label_set_text_static(weatherIcon, "");
    lv_label_set_text_static(weatherMain, "No data");
    lv_obj_set_x(weatherMain, 0);
    lv_label_set_text_static(weatherDetail, "Open the companion app");
    return;
  }

  const bool imperial = settingsController.GetWeatherFormat() == Controllers::Settings::WeatherFormat::Imperial;
  auto degrees = [imperial](const Controllers::SimpleWeatherService::Temperature& t) {
    return imperial ? t.Fahrenheit() : t.Celsius();
  };

  if (weather->location[0] == '\0') {
    lv_label_set_text_static(weatherHeader, "WEATHER");
  } else {
    lv_label_set_text(weatherHeader, weather->location.data());
    for (char* c = lv_label_get_text(weatherHeader); *c != '\0'; c++) {
      *c = static_cast<char>(std::toupper(static_cast<unsigned char>(*c)));
    }
    lv_label_refr_text(weatherHeader);
  }

  lv_label_set_text_static(weatherIcon, Symbols::GetSymbol(weather->iconId, weatherService.IsNight()));
  lv_label_set_text_fmt(weatherMain, "%d° %s", degrees(weather->temperature), ConditionName(weather->iconId));
  lv_obj_set_x(weatherMain, 36);
  lv_label_set_text_fmt(weatherDetail, "H:%d°  L:%d°", degrees(weather->maxTemperature), degrees(weather->minTemperature));
}
