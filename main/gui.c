#include "system.h"
#include "apps.h"
#include <TSGL_gui.h>

tsgl_gui* gui;

static void gui_loop(void* parameter) {
    while (true) {
        tsgl_keyboard_readAll(&keyboard);
        tsgl_gui_processTouchscreen(gui, &touchscreen);
        tsgl_gui_processGui(gui, NULL, &benchmark, 0);
    }
}

void gui_init() {
    gui = tsgl_gui_createRoot_buffer(&display, &framebuffer);
    apps_init();
    app_desktop_open();
    xTaskCreate(gui_loop, NULL, 4096, NULL, 24, NULL);
}