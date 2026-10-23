#pragma once
// Two colour sets for the panel: the dark one written in panel.yaml, and her pink one.
// Switching walks every widget and shared style and swaps each colour for its partner.
#include <lvgl.h>

namespace theme {

struct Pair {
  uint32_t dark, pink;
};

// Every colour that changes. Heating orange, air quality and connection colours keep their meaning
// and stay. No pink colour may also appear as a dark one, or switching back would hit it too
// (hence 0xFFFCFE for the cards instead of pure white, which the heating chip's text uses).
static constexpr Pair PALETTE[] = {
    {0xF5B942, 0xF0559A},  // accent: living room, lights on
    {0x8E9BFF, 0x8E6BEA},  // bedroom
    {0x0F1419, 0xFFEAF3},  // page
    {0x1B2129, 0xFFFCFE},  // cards
    {0x2A323D, 0xFAD3E4},  // round buttons
    {0x3A4350, 0xF2B5CF},  // inactive page dots, pressed buttons
    {0x8A94A6, 0x9A6F86},  // muted text
    {0xF2F4F7, 0x3B1F30},  // text
};

inline bool pink = false;

inline uint32_t swap(uint32_t rgb, bool to_pink) {
  for (const auto &p : PALETTE) {
    if ((to_pink ? p.dark : p.pink) == rgb)
      return to_pink ? p.pink : p.dark;
  }
  return rgb;
}

// Use instead of lv_color_hex() for a colour from the dark set in panel.yaml
inline lv_color_t color(uint32_t dark) { return lv_color_hex(pink ? swap(dark, true) : dark); }

static constexpr lv_style_prop_t PROPS[] = {LV_STYLE_BG_COLOR, LV_STYLE_BG_GRAD_COLOR, LV_STYLE_TEXT_COLOR,
                                            LV_STYLE_BORDER_COLOR};

inline lv_style_value_t swapped(lv_style_value_t v, bool to_pink) {
  v.color = lv_color_hex(swap(lv_color_to_u32(v.color) & 0xFFFFFF, to_pink));
  return v;
}

inline void swap_style(lv_style_t *style, bool to_pink) {
  for (auto prop : PROPS) {
    lv_style_value_t v;
    if (lv_style_get_prop(style, prop, &v) == LV_STYLE_RES_FOUND)
      lv_style_set_prop(style, prop, swapped(v, to_pink));
  }
  lv_obj_report_style_change(style);
}

// Colours set on a widget itself (in panel.yaml or by a lambda), in the parts and states the panel uses
inline void swap_tree(lv_obj_t *obj, bool to_pink) {
  static constexpr lv_style_selector_t PARTS[] = {LV_PART_MAIN, LV_PART_INDICATOR, LV_PART_KNOB};
  static constexpr lv_style_selector_t STATES[] = {LV_STATE_DEFAULT, LV_STATE_CHECKED, LV_STATE_PRESSED};
  for (auto part : PARTS) {
    for (auto state : STATES) {
      for (auto prop : PROPS) {
        lv_style_value_t v;
        if (lv_obj_get_local_style_prop(obj, prop, &v, part | state) == LV_STYLE_RES_FOUND)
          lv_obj_set_local_style_prop(obj, prop, swapped(v, to_pink), part | state);
      }
    }
  }
  for (uint32_t i = 0; i < lv_obj_get_child_count(obj); i++)
    swap_tree(lv_obj_get_child(obj, i), to_pink);
}

}  // namespace theme
