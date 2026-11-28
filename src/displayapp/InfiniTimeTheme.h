#pragma once

#include <lvgl/lvgl.h>

namespace Colors {
  // watchOS dark system colors
  static constexpr lv_color_t red = LV_COLOR_MAKE(0xff, 0x45, 0x3a);
  static constexpr lv_color_t orange = LV_COLOR_MAKE(0xff, 0x9f, 0x0a);
  static constexpr lv_color_t yellow = LV_COLOR_MAKE(0xff, 0xd6, 0x0a);
  static constexpr lv_color_t green = LV_COLOR_MAKE(0x30, 0xd1, 0x58);
  static constexpr lv_color_t mint = LV_COLOR_MAKE(0x66, 0xd4, 0xcf);
  static constexpr lv_color_t teal = LV_COLOR_MAKE(0x40, 0xcb, 0xe0);
  static constexpr lv_color_t cyan = LV_COLOR_MAKE(0x64, 0xd2, 0xff);
  static constexpr lv_color_t blue = LV_COLOR_MAKE(0x0a, 0x84, 0xff);
  static constexpr lv_color_t indigo = LV_COLOR_MAKE(0x5e, 0x5c, 0xe6);
  static constexpr lv_color_t purple = LV_COLOR_MAKE(0xbf, 0x5a, 0xf2);
  static constexpr lv_color_t pink = LV_COLOR_MAKE(0xff, 0x37, 0x5f);
  static constexpr lv_color_t brown = LV_COLOR_MAKE(0xac, 0x8e, 0x68);

  // Activity ring colors
  static constexpr lv_color_t move = LV_COLOR_MAKE(0xfa, 0x11, 0x4f);
  static constexpr lv_color_t exercise = LV_COLOR_MAKE(0xa6, 0xff, 0x00);
  static constexpr lv_color_t stand = LV_COLOR_MAKE(0x1e, 0xea, 0xef);

  static constexpr lv_color_t secondaryLabel = LV_COLOR_MAKE(0x8e, 0x8e, 0x93);
  static constexpr lv_color_t button = LV_COLOR_MAKE(0x3a, 0x3a, 0x3c);
  static constexpr lv_color_t platter = LV_COLOR_MAKE(0x24, 0x24, 0x26);

  static constexpr lv_color_t deepOrange = red;
  static constexpr lv_color_t lightGray = secondaryLabel;
  static constexpr lv_color_t gray = LV_COLOR_MAKE(0x48, 0x48, 0x4a);

  static constexpr lv_color_t bg = button;
  static constexpr lv_color_t bgAlt = platter;
  static constexpr lv_color_t bgDark = LV_COLOR_MAKE(0x1c, 0x1c, 0x1e);
  static constexpr lv_color_t highlight = green;
};

/**
 * Initialize the default
 * @param color_primary the primary color of the theme
 * @param color_secondary the secondary color for the theme
 * @param flags ORed flags starting with `LV_THEME_DEF_FLAG_...`
 * @param font_small pointer to a small font
 * @param font_normal pointer to a normal font
 * @param font_subtitle pointer to a large font
 * @param font_title pointer to a extra large font
 * @return a pointer to reference this theme later
 */
lv_theme_t* lv_pinetime_theme_init();
