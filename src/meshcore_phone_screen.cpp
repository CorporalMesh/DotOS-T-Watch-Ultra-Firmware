#include "meshcore_phone_screen.h"
#include "meshcore.h"
#include "meshcore_screen.h"
#include "device_mode.h"
#include "theme.h"
#include <lvgl.h>
#include <cstdio>
static lv_obj_t* screen;
static lv_obj_t* state;
static void update() {
 char text[200];
 snprintf(text,sizeof(text),"%s\nPair: MeshCore-%s\nPIN: %06lu",
  meshcore_phone_connected()?"Phone connected":meshcore_phone_enabled()?"Waiting for phone":"Phone sync off",
  meshcore_get_long_name(),(unsigned long)meshcore_phone_pin());
 lv_label_set_text(state,text);
}
static void toggle(lv_event_t*) {
 if (!meshcore_phone_enabled()) device_mode_set(DeviceMode::FieldTool);
 bool ok=meshcore_set_phone_enabled(!meshcore_phone_enabled());
 update();
 if(!ok) lv_label_set_text(state,"Enable LoRa first.\nTurn off WiFi, Notify,\nBluetooth scans/detectors.\nThen try Enable sync.");
}
void meshcore_phone_screen_show() {
 if(!screen) {
  screen=lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(screen,lv_color_black(),0);
  auto* panel=lv_obj_create(screen);
  lv_obj_set_size(panel,286,322);lv_obj_center(panel);
  lv_obj_set_style_bg_color(panel,lv_color_black(),0);
  lv_obj_set_style_text_color(panel,lv_color_white(),0);
  lv_obj_set_layout(panel,LV_LAYOUT_FLEX);lv_obj_set_flex_flow(panel,LV_FLEX_FLOW_COLUMN);
  auto* title=lv_label_create(panel);lv_label_set_text(title,"MeshCore phone sync");
  state=lv_label_create(panel);lv_obj_set_width(state,250);lv_label_set_long_mode(state,LV_LABEL_LONG_WRAP);
  auto* hint=lv_label_create(panel);lv_obj_set_width(hint,250);lv_label_set_long_mode(hint,LV_LABEL_LONG_WRAP);
  lv_label_set_text(hint,"Connect in the MeshCore app.\nBLE reserved until reboot;\nNotify and scanners pause.");
  auto* btn=lv_button_create(panel);auto* label=lv_label_create(btn);lv_label_set_text(label,"Enable / disable sync");
  lv_obj_add_event_cb(btn,toggle,LV_EVENT_CLICKED,nullptr);
  auto* back=lv_button_create(panel);label=lv_label_create(back);lv_label_set_text(label,"Back");
  lv_obj_add_event_cb(back,[](lv_event_t*) { meshcore_screen_show(); },LV_EVENT_CLICKED,nullptr);
  lv_timer_create([](lv_timer_t*){ if(screen&&lv_screen_active()==screen)update(); },1000,nullptr);
 }
 update();lv_scr_load(screen);
}
