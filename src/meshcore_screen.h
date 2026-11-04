#pragma once
#include <lvgl.h>

void meshcore_screen_create();
void meshcore_screen_show();
bool meshcore_screen_is_active();
void meshcore_screen_refresh();  // called by meshcore.cpp when a new message arrives
