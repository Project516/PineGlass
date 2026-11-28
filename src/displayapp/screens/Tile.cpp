#include "displayapp/screens/Tile.h"
#include "displayapp/DisplayApp.h"
#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;

namespace {
  struct Slot {
    lv_coord_t x;
    lv_coord_t y;
  };

  // Honeycomb rows of 3-2-3. Centers in pixels.
  constexpr std::array<Slot, Tile::appsPerScreen> slots {{
    {42, 52},
    {120, 52},
    {198, 52},
    {81, 120},
    {159, 120},
    {42, 188},
    {120, 188},
    {198, 188},
  }};

  constexpr lv_coord_t iconSize = 70;

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
           std::array<Applications, appsPerScreen>& applications)
  : app {app}, pageIndicator(screenID, numScreens) {

  settingsController.SetAppMenu(screenID);

  if (numScreens > 1) {
    pageIndicator.Create();
  }

  for (uint8_t i = 0; i < appsPerScreen; i++) {
    apps[i] = applications[i].application;
    if (apps[i] == Apps::None) {
      continue;
    }

    const Slot& slot = slots[i];
    lv_obj_t* button = lv_btn_create(lv_scr_act(), nullptr);
    lv_obj_set_size(button, iconSize, iconSize);
    lv_obj_set_pos(button, slot.x - iconSize / 2, slot.y - iconSize / 2);
    lv_obj_set_style_local_bg_color(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, AppColor(apps[i]));
    lv_obj_set_style_local_transform_width(button, LV_BTN_PART_MAIN, LV_STATE_PRESSED, -4);
    lv_obj_set_style_local_transform_height(button, LV_BTN_PART_MAIN, LV_STATE_PRESSED, -4);
    lv_obj_set_style_local_pad_all(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_text_font(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, &app_icons_36);

    lv_obj_t* icon = lv_label_create(button, nullptr);
    lv_label_set_text_static(icon, applications[i].icon);

    if (!applications[i].enabled) {
      lv_obj_set_style_local_bg_opa(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_30);
      lv_obj_set_style_local_text_opa(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_50);
      lv_obj_set_click(button, false);
    }

    button->user_data = this;
    lv_obj_set_event_cb(button, event_handler);
    buttons[i] = button;
  }
}

Tile::~Tile() {
  lv_obj_clean(lv_scr_act());
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
