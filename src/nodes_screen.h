#pragma once
#include <lvgl.h>

void nodes_screen_create();
void nodes_screen_show();
bool nodes_screen_is_active();
void nodes_screen_refresh();  // called by meshcore.cpp when a node changes
