#include "settings.h"
#include "../system.h"
#include "../apps.h"

#include <TSGL_gui.h>
#include <TSGL_gui/button.h>
#include <TSGL_gui/sprite.h>
#include <TSGL_gui/colorpicker.h>
#include <TSGL_gui/text.h>

#define SETTINGS_PATH "/storage/settings.cfg"

static tsgl_gui* scene;

app_settings_struct defaultSettings = {
    .sound_enable_load = true,
    .sound_enable_shutdown = true,
    .sound_enable_click = true,
    .sound_enable_connect = true,
    .sound_enable_disconnect = true,

    .sound_volume_load = 0.5,
    .sound_volume_shutdown = 0.5,
    .sound_volume_click = 0.1,
    .sound_volume_connect = 1,
    .sound_volume_disconnect = 1
};

static uint8_t currentVersion = 0;
app_settings_struct currentSettings;

static void* callback_openDesktop(tsgl_gui* self, int arg0, void* arg1, void* userArg) {
    if (arg0 == 0) {
        system_playSoundFromList(system_sound_disconnect, false);
        app_desktop_open();
    }
    return NULL;
}

void app_settings_init() {
    FILE* file = tsgl_filesystem_open(SETTINGS_PATH, "rb");
    if (file) {
        uint8_t realVersion;
        fread(&realVersion, 1, 1, file);
        if (realVersion == currentVersion) {
            fread(&currentSettings, 1, sizeof(app_settings_struct), file);
        } else {
            memcpy(&currentSettings, &defaultSettings, sizeof(app_settings_struct));
        }
        fclose(file);
    } else {
        memcpy(&currentSettings, &defaultSettings, sizeof(app_settings_struct));
    }

    scene = tsgl_gui_addObject(gui);
    scene->color = tsgl_color_raw(tsgl_color_fromHex(0xa0a0a0), gui->colormode);

    tsgl_gui* plate_up = tsgl_gui_addObject(scene);
    plate_up->x = 0;
    plate_up->y = 0;
    plate_up->width = framebuffer.width;
    plate_up->height = 50;
    plate_up->color = tsgl_color_raw(tsgl_color_fromHex(0x757575), plate_up->colormode);

    tsgl_gui* back = tsgl_gui_addButton(plate_up);
    back->width = 80;
    back->height = 40;
    back->user_callback = callback_openDesktop;
    tsgl_gui_setOffsetFromBorder(back, tsgl_gui_offsetFromBorder_center_left, 5, 0);
    tsgl_gui_button_setStyle(back, tsgl_color_fromHex(0xcacaca), tsgl_color_fromHex(0xa0a0a0), tsgl_gui_button_fill);
    tsgl_gui_button_setText(back, TSGL_WHITE, 8, "< back", false);

    tsgl_gui* text = tsgl_gui_addText(plate_up);
    tsgl_gui_setOffsetFromBorder(text, tsgl_gui_offsetFromBorder_center, 0, 0);
    tsgl_gui_text_setText(text, "settings", false);
    tsgl_gui_text_setParams(text, (tsgl_print_settings) {
        .fill = TSGL_INVALID_RAWCOLOR,
        .bg = TSGL_INVALID_RAWCOLOR,
        .fg = tsgl_color_raw(TSGL_WHITE, text->colormode),
        .font = tsgl_font_defaultFont,
        .locationMode = tsgl_print_start_top,
        .multiline = true,
        .globalCentering = true,
        .targetWidth = 16
    });
}

void app_settings_open() {
    system_playSoundFromList(system_sound_connect, false);
    tsgl_gui_select(scene);
}

void app_settings_save() {
    FILE* file = tsgl_filesystem_open(SETTINGS_PATH, "wb");
    if (file) {
        fwrite(&currentVersion, 1, 1, file);
        fwrite(&currentSettings, 1, sizeof(app_settings_struct), file);
        fclose(file);
    }
}