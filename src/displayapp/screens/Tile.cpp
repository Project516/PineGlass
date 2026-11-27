#include "displayapp/screens/Tile.h"
#include "displayapp/DisplayApp.h"
#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;

namespace {
  struct Slot {
    lv_coord_t x;
    lv_coord_t y;
    lv_coord_t size;
  };

  // Rows of 2-3-4-3-2. Icons shrink with distance from the center, like the
  // watchOS honeycomb at rest. Centers and diameters in pixels.
  constexpr std::array<Slot, Tile::appsPerScreen> slots {{
    {91, 21, 40},
    {149, 21, 40},
    {62, 70, 46},
    {120, 69, 52},
    {178, 70, 46},
    {33, 120, 44},
    {89, 120, 58},
    {151, 120, 58},
    {207, 120, 44},
    {62, 170, 46},
    {120, 171, 52},
    {178, 170, 46},
    {91, 220, 40},
    {149, 220, 40},
  }};

  constexpr lv_coord_t largeGlyphMinSize = 46;

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
    lv_obj_set_size(button, slot.size, slot.size);
    lv_obj_set_pos(button, slot.x - slot.size / 2, slot.y - slot.size / 2);
    lv_obj_set_style_local_bg_color(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, AppColor(apps[i]));
    lv_obj_set_style_local_transform_width(button, LV_BTN_PART_MAIN, LV_STATE_PRESSED, -4);
    lv_obj_set_style_local_transform_height(button, LV_BTN_PART_MAIN, LV_STATE_PRESSED, -4);
    lv_obj_set_style_local_pad_all(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 0);
    if (slot.size >= largeGlyphMinSize) {
      lv_obj_set_style_local_text_font(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, &app_icons_28);
    }

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
