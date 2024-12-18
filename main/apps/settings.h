#pragma once
#include <TSGL_gui.h>

typedef struct {
    bool sound_enable_load;
    bool sound_enable_shutdown;
    bool sound_enable_click;
    bool sound_enable_connect;
    bool sound_enable_disconnect;

    float sound_volume_load;
    float sound_volume_shutdown;
    float sound_volume_click;
    float sound_volume_connect;
    float sound_volume_disconnect;
} app_settings_struct;

extern app_settings_struct currentSettings;

void app_settings_init();
void app_settings_open();