#include "desktop.h"
#include "../system.h"
#include "../apps.h"

#include <TSGL_gui.h>
#include <TSGL_gui/button.h>
#include <TSGL_gui/sprite.h>
#include <TSGL_gui/colorpicker.h>
#include <TSGL_gui/text.h>

static tsgl_gui* scene;

static void* callback_powerOff(tsgl_gui* self, int arg0, void* arg1, void* userArg) {
    if (arg0 == 0) {
        system_powerOff();
    }
    return NULL;
}

static void* callback_settings(tsgl_gui* self, int arg0, void* arg1, void* userArg) {
    if (arg0 == 0) {
        app_settings_open();
    }
    return NULL;
}

void app_desktop_init() {
    scene = tsgl_gui_addObject(gui);
    scene->color = tsgl_color_raw(tsgl_color_fromHex(0x018db4), gui->colormode);

    tsgl_gui* plate_up = tsgl_gui_addObject(scene);
    plate_up->x = 0;
    plate_up->y = 0;
    plate_up->width = framebuffer.width;
    plate_up->height = 50;
    plate_up->color = tsgl_color_raw(tsgl_color_fromHex(0x757575), plate_up->colormode);

    tsgl_gui* powerOff = tsgl_gui_addButton(plate_up);
    powerOff->width = 90;
    powerOff->height = 40;
    powerOff->user_callback = callback_powerOff;
    tsgl_gui_setOffsetFromBorder(powerOff, tsgl_gui_offsetFromBorder_center_left, 5, 0);
    tsgl_gui_button_setStyle(powerOff, TSGL_RED, tsgl_color_fromHex(0x9c6800), tsgl_gui_button_fill);
    tsgl_gui_button_setText(powerOff, TSGL_WHITE, 8, "shutdown", false);

    tsgl_gui* settings = tsgl_gui_addButton(plate_up);
    settings->width = 90;
    settings->height = 40;
    settings->user_callback = callback_settings;
    tsgl_gui_setOffsetFromBorder(settings, tsgl_gui_offsetFromBorder_center_right, 5, 0);
    tsgl_gui_button_setStyle(settings, tsgl_color_fromHex(0xcacaca), tsgl_color_fromHex(0xa0a0a0), tsgl_gui_button_fill);
    tsgl_gui_button_setText(settings, TSGL_WHITE, 8, "settings", false);

    tsgl_gui* text = tsgl_gui_addText(plate_up);
    tsgl_gui_setOffsetFromBorder(text, tsgl_gui_offsetFromBorder_center, 0, 0);
    tsgl_gui_text_setText(text, "smart speaker", false);
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

void app_desktop_open() {
    tsgl_gui_select(scene);
}