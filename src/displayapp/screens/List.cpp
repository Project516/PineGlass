#include "displayapp/screens/List.h"
#include "displayapp/DisplayApp.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;

namespace {
  void ButtonEventHandler(lv_obj_t* obj, lv_event_t event) {
    auto* screen = static_cast<List*>(obj->user_data);
    screen->OnButtonEvent(obj, event);
  }

  lv_color_t IconColor(Pinetime::Applications::Apps app) {
    using Pinetime::Applications::Apps;
    switch (app) {
      case Apps::SettingDisplay:
      case Apps::SettingBluetooth:
        return Colors::blue;
      case Apps::SettingWakeUp:
      case Apps::SettingSteps:
        return Colors::green;
      case Apps::SettingWatchFace:
        return Colors::indigo;
      case Apps::SettingHeartRate:
      case Apps::SettingChimes:
        return Colors::red;
      case Apps::SettingWeatherFormat:
        return Colors::cyan;
      case Apps::SettingShakeThreshold:
        return Colors::orange;
      case Apps::BatteryInfo:
        return Colors::green;
      default:
        return Colors::gray;
    }
  }
}

List::List(uint8_t screenID,
           uint8_t numScreens,
           DisplayApp* app,
           Controllers::Settings& settingsController,
           std::array<Applications, MAXLISTITEMS>& applications)
  : app {app}, settingsController {settingsController}, pageIndicator(screenID, numScreens) {

  // Set the background to Black
  lv_obj_set_style_local_bg_color(lv_scr_act(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(0, 0, 0));

  settingsController.SetSettingsMenu(screenID);

  pageIndicator.Create();

  lv_obj_t* container = lv_cont_create(lv_scr_act(), nullptr);

  lv_obj_set_style_local_bg_opa(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
  static constexpr int innerPad = 4;
  lv_obj_set_style_local_pad_inner(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, innerPad);
  lv_obj_set_style_local_border_width(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);

  lv_obj_set_pos(container, 0, 0);
  lv_obj_set_width(container, LV_HOR_RES - 8);
  lv_obj_set_height(container, LV_VER_RES);
  lv_cont_set_layout(container, LV_LAYOUT_COLUMN_LEFT);

  for (int i = 0; i < MAXLISTITEMS; i++) {
    apps[i] = applications[i].application;
    if (applications[i].application != Apps::None) {

      static constexpr int btnHeight = (LV_HOR_RES_MAX - ((MAXLISTITEMS - 1) * innerPad)) / MAXLISTITEMS;
      itemApps[i] = lv_btn_create(container, nullptr);
      lv_obj_set_style_local_radius(itemApps[i], LV_BTN_PART_MAIN, LV_STATE_DEFAULT, btnHeight / 3);
      lv_obj_set_style_local_bg_color(itemApps[i], LV_BTN_PART_MAIN, LV_STATE_DEFAULT, Colors::bgAlt);
      lv_obj_set_width(itemApps[i], LV_HOR_RES - 8);
      lv_obj_set_height(itemApps[i], btnHeight);
      lv_obj_set_event_cb(itemApps[i], ButtonEventHandler);
      lv_btn_set_layout(itemApps[i], LV_LAYOUT_OFF);
      itemApps[i]->user_data = this;
      lv_obj_set_style_local_clip_corner(itemApps[i], LV_BTN_PART_MAIN, LV_STATE_DEFAULT, true);

      static constexpr lv_coord_t iconSize = 34;
      lv_obj_t* icon = lv_obj_create(itemApps[i], nullptr);
      lv_obj_set_size(icon, iconSize, iconSize);
      lv_obj_set_style_local_radius(icon, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_RADIUS_CIRCLE);
      lv_obj_set_style_local_bg_color(icon, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, IconColor(applications[i].application));
      lv_obj_set_click(icon, false);
      lv_obj_align(icon, nullptr, LV_ALIGN_IN_LEFT_MID, 10, 0);
      lv_obj_t* glyph = lv_label_create(icon, nullptr);
      lv_label_set_text_static(glyph, applications[i].icon);
      lv_obj_align(glyph, nullptr, LV_ALIGN_CENTER, 0, 0);

      lv_obj_t* text = lv_label_create(itemApps[i], nullptr);
      lv_label_set_text_fmt(text, "%s", applications[i].name);
      lv_obj_align(text, icon, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    }
  }
}

List::~List() {
  lv_obj_clean(lv_scr_act());
}

void List::OnButtonEvent(lv_obj_t* object, lv_event_t event) {
  if (event == LV_EVENT_CLICKED) {
    for (int i = 0; i < MAXLISTITEMS; i++) {
      if (apps[i] != Apps::None && object == itemApps[i]) {
        app->StartApp(apps[i], DisplayApp::FullRefreshDirections::Up);
        running = false;
        return;
      }
    }
  }
}
