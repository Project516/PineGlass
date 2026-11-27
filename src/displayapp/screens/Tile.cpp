#include "displayapp/screens/Tile.h"
#include "displayapp/DisplayApp.h"
#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;

namespace {
  constexpr lv_coord_t iconSize = 52;
  constexpr lv_coord_t gap = 6;
  // Vertical distance between honeycomb rows: (iconSize + gap) * sin(60)
  constexpr lv_coord_t rowPitch = 50;
  constexpr lv_coord_t gridTop = 30;

  void event_handler(lv_obj_t* obj, lv_event_t event) {
    if (event == LV_EVENT_CLICKED) {
      static_cast<Tile*>(obj->user_data)->OnClick(obj);
    }
  }

  lv_color_t AppColor(Pinetime::Applications::Apps app) {
    using Pinetime::Applications::Apps;
    switch (app) {
      case Apps::StopWatch:
      case Apps::Alarm:
      case Apps::Timer:
        return Colors::orange;
      case Apps::Steps:
        return Colors::green;
      case Apps::HeartRate:
        return Colors::red;
      case Apps::Music:
        return Colors::pink;
      case Apps::Paint:
        return Colors::purple;
      case Apps::Paddle:
        return Colors::indigo;
      case Apps::Twos:
        return Colors::yellow;
      case Apps::Dice:
        return Colors::teal;
      case Apps::Metronome:
        return Colors::brown;
      case Apps::Navigation:
        return Colors::blue;
      case Apps::Weather:
        return Colors::cyan;
      case Apps::Motion:
        return Colors::mint;
      default:
        return Colors::gray;
    }
  }
}

Tile::Tile(uint8_t screenID,
           uint8_t numScreens,
           DisplayApp* app,
           Controllers::Settings& settingsController,
           Controllers::DateTime& dateTimeController,
           std::array<Applications, appsPerScreen>& applications)
  : app {app}, dateTimeController {dateTimeController}, pageIndicator(screenID, numScreens) {

  settingsController.SetAppMenu(screenID);

  labelTime = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(labelTime, nullptr, LV_ALIGN_IN_TOP_RIGHT, -10, 2);

  if (numScreens > 1) {
    pageIndicator.Create();
  }

  uint8_t index = 0;
  for (uint8_t row = 0; row < rowSizes.size(); row++) {
    const uint8_t count = rowSizes[row];
    const lv_coord_t rowWidth = count * iconSize + (count - 1) * gap;
    const lv_coord_t x0 = (LV_HOR_RES - rowWidth) / 2;
    const lv_coord_t y = gridTop + row * rowPitch;

    for (uint8_t col = 0; col < count; col++, index++) {
      apps[index] = applications[index].application;
      if (apps[index] == Apps::None) {
        continue;
      }

      lv_obj_t* button = lv_btn_create(lv_scr_act(), nullptr);
      lv_obj_set_size(button, iconSize, iconSize);
      lv_obj_set_pos(button, x0 + col * (iconSize + gap), y);
      lv_obj_set_style_local_bg_color(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, AppColor(apps[index]));
      lv_obj_set_style_local_transform_width(button, LV_BTN_PART_MAIN, LV_STATE_PRESSED, -4);
      lv_obj_set_style_local_transform_height(button, LV_BTN_PART_MAIN, LV_STATE_PRESSED, -4);
      lv_obj_set_style_local_pad_all(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 0);

      lv_obj_t* icon = lv_label_create(button, nullptr);
      lv_label_set_text_static(icon, applications[index].icon);

      if (!applications[index].enabled) {
        lv_obj_set_style_local_bg_opa(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_30);
        lv_obj_set_style_local_text_opa(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_50);
        lv_obj_set_click(button, false);
      }

      button->user_data = this;
      lv_obj_set_event_cb(button, event_handler);
      buttons[index] = button;
    }
  }

  Refresh();
  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
}

Tile::~Tile() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void Tile::Refresh() {
  currentDateTime = std::chrono::time_point_cast<std::chrono::minutes>(dateTimeController.CurrentDateTime());
  if (currentDateTime.IsUpdated()) {
    lv_label_set_text(labelTime, dateTimeController.FormattedTime().c_str());
    lv_obj_align(labelTime, nullptr, LV_ALIGN_IN_TOP_RIGHT, -10, 2);
  }
}

void Tile::OnClick(lv_obj_t* obj) {
  for (uint8_t i = 0; i < appsPerScreen; i++) {
    if (buttons[i] == obj) {
      app->StartApp(apps[i], DisplayApp::FullRefreshDirections::Up);
      running = false;
      return;
    }
  }
}
