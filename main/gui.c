#include "system.h"
#include <TSGL_gui.h>
#include <TSGL_gui/button.h>
#include <TSGL_gui/sprite.h>
#include <TSGL_gui/colorpicker.h>

static tsgl_gui* gui;
static tsgl_gui* scene_main;
static tsgl_gui* scene_settings;

// --------------------------------------------- main scene

static tsgl_gui* plate_up;
static tsgl_gui* button_powerOff;
static tsgl_gui* button_settings;

static void* callback_powerOff(tsgl_gui* self, int arg0, void* arg1, void* userArg) {
    if (arg0 == 0) {
        system_powerOff();
    }
    return NULL;
}

static void* callback_settings(tsgl_gui* self, int arg0, void* arg1, void* userArg) {
    if (arg0 == 0) {
        tsgl_gui_select(scene_settings);
    }
    return NULL;
}

static void gui_scene_main() {
    scene_main = tsgl_gui_addObject(gui);
    scene_main->color = tsgl_color_raw(tsgl_color_fromHex(0x018db4), gui->colormode);

    plate_up = tsgl_gui_addObject(scene_main);
    plate_up->x = 0;
    plate_up->y = 0;
    plate_up->width = framebuffer.width;
    plate_up->height = 50;
    plate_up->color = tsgl_color_raw(tsgl_color_fromHex(0x757575), plate_up->colormode);

    button_powerOff = tsgl_gui_addButton(plate_up);
    button_powerOff->width = 150;
    button_powerOff->height = 30;
    button_powerOff->user_callback = callback_powerOff;
    tsgl_gui_setOffsetFromBorder(button_powerOff, tsgl_gui_offsetFromBorder_center_left, 10, 0);
    tsgl_gui_button_setStyle(button_powerOff, TSGL_RED, tsgl_color_fromHex(0x9c6800), tsgl_gui_button_fill);
    tsgl_gui_button_setText(button_powerOff, TSGL_WHITE, 8, "power off", false);

    button_settings = tsgl_gui_addButton(plate_up);
    button_settings->width = 150;
    button_settings->height = 30;
    button_settings->user_callback = callback_settings;
    tsgl_gui_setOffsetFromBorder(button_settings, tsgl_gui_offsetFromBorder_center_right, 10, 0);
    tsgl_gui_button_setStyle(button_settings, tsgl_color_fromHex(0xcacaca), tsgl_color_fromHex(0xa0a0a0), tsgl_gui_button_fill);
    tsgl_gui_button_setText(button_settings, TSGL_WHITE, 8, "settings", false);
}

// --------------------------------------------- main scene

static void gui_scene_settings() {
    scene_settings = tsgl_gui_addObject(gui);
    scene_settings->color = tsgl_color_raw(tsgl_color_fromHex(0xa0a0a0), gui->colormode);
}

// ---------------------------------------------

static void gui_loop(void* parameter) {
    while (true) {
        tsgl_keyboard_readAll(&keyboard);
        tsgl_gui_processTouchscreen(gui, &touchscreen);
        tsgl_gui_processGui(gui, NULL, &benchmark, 0);
    }
}

void gui_init() {
    gui = tsgl_gui_createRoot_buffer(&display, &framebuffer);

    gui_scene_main();
    gui_scene_settings();

    tsgl_gui_select(scene_main);
    xTaskCreate(gui_loop, NULL, 4096, NULL, 24, NULL);
}