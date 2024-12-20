#include "settings.h"
#include "../system.h"
#include "../apps.h"

#include <TSGL_gui.h>
#include <TSGL_gui/button.h>
#include <TSGL_gui/sprite.h>
#include <TSGL_gui/colorpicker.h>
#include <TSGL_gui/text.h>
#include <TSGL_gui/lever.h>
#include <TSGL_gui/tabbar.h>

#define TAB_COUNT 4
#define TAB_COLOR tsgl_color_fromHex(0x666666)
#define TAB_COLOR_ENABLE tsgl_color_fromHex(0xcacaca)

const char* settingsPath = "/storage/settings.cfg";
static const tsgl_pos tabbar_size = 90;
static const tsgl_pos leverPadding = 14;

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

static bool needSave = false;
static uint8_t currentTab;
static uint8_t oldTab;
static tsgl_pos lastTabY;
static size_t lastIndex;
static tsgl_pos lastLeverY;

static void resetParams() {
    currentTab = 0;
    oldTab = 0;
    lastTabY = 5;
    lastIndex = 0;
    lastLeverY = 14;
}

static TaskHandle_t infoUpdateTask_handle;
static void* callback_openDesktop(tsgl_gui* self, int arg0, void* arg1, void* userArg) {
    if (arg0 == 0) {
        if (needSave) {
            vTaskDelete(infoUpdateTask_handle);
            tsgl_gui_free(scene);
            app_settings_save();
            needSave = false;
        }
        app_desktop_open();
    }
    return NULL;
}

static void* callback_onBoolChange(tsgl_gui* self, int arg0, void* arg1, void* userArg) {
    bool* val = userArg;
    *val = (bool)arg0;
    needSave = true;
    return NULL;
}

static tsgl_pos leverPos;
static void addTitleLever(tsgl_gui* tab, const char* title, bool* parameter) {
    tsgl_gui* text = tsgl_gui_addText(tab);
    tsgl_gui_setOffsetFromBorder(text, tsgl_gui_offsetFromBorder_up_left, 10, lastLeverY);
    tsgl_gui_text_setText(text, title, false);
    tsgl_gui_text_setParams(text, (tsgl_print_settings) {
        .fill = TSGL_INVALID_RAWCOLOR,
        .bg = TSGL_INVALID_RAWCOLOR,
        .fg = tsgl_color_raw(TSGL_WHITE, text->colormode),
        .font = tsgl_font_defaultFont,
        .locationMode = tsgl_print_start_top,
        .multiline = false,
        .globalCentering = false,
        .targetWidth = 12
    });

    tsgl_gui* lever = tsgl_gui_addLever(tab, *parameter);
    tsgl_rawcolor bodyColor = tsgl_color_raw(tsgl_color_fromHex(0x656565), lever->colormode);
    tsgl_gui_lever_setParams(lever, bodyColor, tsgl_color_raw(TSGL_WHITE, lever->colormode), bodyColor, tsgl_color_raw(TSGL_GREEN, lever->colormode));
    lever->x = leverPos;
    lever->y = text->y - 8;
    lever->width = 80;
    lever->height = 32;
    lever->user_callback = callback_onBoolChange;
    lever->userArg = parameter;

    lastLeverY += 32 + leverPadding;
}

static void _tab_sound(tsgl_gui* host) {
    const char* longName = "disconnect";
    tsgl_print_textArea textArea = tsgl_font_getTextArea(0, 0, (tsgl_print_settings) {
        .fill = TSGL_INVALID_RAWCOLOR,
        .bg = TSGL_INVALID_RAWCOLOR,
        .fg = TSGL_INVALID_RAWCOLOR,
        .font = tsgl_font_defaultFont,
        .locationMode = tsgl_print_start_top,
        .multiline = false,
        .globalCentering = false,
        .targetWidth = 12
    }, longName);
    leverPos = textArea.right + 10 + 10;
    addTitleLever(host, "load", &currentSettings.sound_enable_load);
    addTitleLever(host, "shutdown", &currentSettings.sound_enable_shutdown);
    addTitleLever(host, "click", &currentSettings.sound_enable_click);
    addTitleLever(host, "connect", &currentSettings.sound_enable_connect);
    addTitleLever(host, longName, &currentSettings.sound_enable_disconnect);
}

static void _tab_connect(tsgl_gui* host) {
    tsgl_gui* tabbar = tsgl_gui_addTabbar(host, true, 5, 5, 80);
    tsgl_gui_setAllFormat(tabbar, tsgl_gui_absolute);
    tabbar->color = tsgl_color_raw(tsgl_color_fromHex(0x2c2c2c), gui->colormode);
    tabbar->height = 50;

    // ----------------- BT host

    tsgl_gui* inlineTab = tsgl_gui_tabbar_addTabObject(tabbar, false);
    inlineTab->color = scene->color;



    tsgl_gui* tabButton = tsgl_gui_tabbar_addTabButton(tabbar, TAB_COLOR, TAB_COLOR_ENABLE, inlineTab);
    tsgl_gui_button_setStyle(tabButton, TSGL_BLACK, TSGL_BLACK, tsgl_gui_button_fill);
    tsgl_gui_button_setText(tabButton, TSGL_WHITE, 8, "BT host", false);

    // ----------------- BT

    inlineTab = tsgl_gui_tabbar_addTabObject(tabbar, false);
    inlineTab->color = scene->color;

    tsgl_gui* back = tsgl_gui_addButton(inlineTab);
    back->width = 80;
    back->height = 40;
    back->user_callback = callback_openDesktop;
    tsgl_gui_setOffsetFromBorder(back, tsgl_gui_offsetFromBorder_center_left, 5, 0);
    tsgl_gui_button_setStyle(back, TAB_COLOR_ENABLE, tsgl_color_fromHex(0xa0a0a0), tsgl_gui_button_fill);
    tsgl_gui_button_setText(back, TSGL_RED, 8, "< back", false);

    tabButton = tsgl_gui_tabbar_addTabButton(tabbar, TAB_COLOR, TAB_COLOR_ENABLE, inlineTab);
    tsgl_gui_button_setStyle(tabButton, TSGL_BLACK, TSGL_BLACK, tsgl_gui_button_fill);
    tsgl_gui_button_setText(tabButton, TSGL_WHITE, 8, "BT", false);

    // ----------------- wifi host

    inlineTab = tsgl_gui_tabbar_addTabObject(tabbar, false);
    inlineTab->color = scene->color;



    tabButton = tsgl_gui_tabbar_addTabButton(tabbar, TAB_COLOR, TAB_COLOR_ENABLE, inlineTab);
    tsgl_gui_button_setStyle(tabButton, TSGL_BLACK, TSGL_BLACK, tsgl_gui_button_fill);
    tsgl_gui_button_setText(tabButton, TSGL_WHITE, 8, "wifi host", false);

    // ----------------- wifi

    inlineTab = tsgl_gui_tabbar_addTabObject(tabbar, false);
    inlineTab->color = scene->color;

    back = tsgl_gui_addButton(inlineTab);
    back->width = 80;
    back->height = 40;
    back->user_callback = callback_openDesktop;
    tsgl_gui_setOffsetFromBorder(back, tsgl_gui_offsetFromBorder_center_left, 5, 0);
    tsgl_gui_button_setStyle(back, TAB_COLOR_ENABLE, tsgl_color_fromHex(0xa0a0a0), tsgl_gui_button_fill);
    tsgl_gui_button_setText(back, TSGL_WHITE, 8, "< back", false);

    tabButton = tsgl_gui_tabbar_addTabButton(tabbar, TAB_COLOR, TAB_COLOR_ENABLE, inlineTab);
    tsgl_gui_button_setStyle(tabButton, TSGL_BLACK, TSGL_BLACK, tsgl_gui_button_fill);
    tsgl_gui_button_setText(tabButton, TSGL_WHITE, 8, "wifi", false);
}

static tsgl_gui* infoTab;
static tsgl_gui* ramInfo;
static tsgl_gui* romInfo;
static void _tab_info(tsgl_gui* host) {
    ramInfo = tsgl_gui_addText(host);
    tsgl_gui_setOffsetFromBorder(ramInfo, tsgl_gui_offsetFromBorder_up_left, 5, 5);
    tsgl_gui_text_setParams(ramInfo, (tsgl_print_settings) {
        .fill = TSGL_INVALID_RAWCOLOR,
        .bg = TSGL_INVALID_RAWCOLOR,
        .fg = tsgl_color_raw(TSGL_WHITE, ramInfo->colormode),
        .font = tsgl_font_defaultFont,
        .locationMode = tsgl_print_start_top,
        .multiline = false,
        .globalCentering = false,
        .targetWidth = 12
    });

    romInfo = tsgl_gui_addText(host);
    tsgl_gui_setOffsetFromBorder(romInfo, tsgl_gui_offsetFromBorder_up_left, 5, 5 + 5 + 12);
    tsgl_gui_text_setParams(romInfo, (tsgl_print_settings) {
        .fill = TSGL_INVALID_RAWCOLOR,
        .bg = TSGL_INVALID_RAWCOLOR,
        .fg = tsgl_color_raw(TSGL_WHITE, romInfo->colormode),
        .font = tsgl_font_defaultFont,
        .locationMode = tsgl_print_start_top,
        .multiline = false,
        .globalCentering = false,
        .targetWidth = 12
    });
}

static void _infoUpdateTask(void *pvParameters) {
    char ramTitle[64];
    char romTitle[64];

    while (true) {
        multi_heap_info_t info;
        heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);

        int totalRam = info.total_allocated_bytes + info.total_free_bytes;
        sprintf(ramTitle, "RAM: %iKB / %iKB = %i%%", totalRam / 1024, info.total_allocated_bytes / 1024, (int)((((((float)info.total_allocated_bytes) / (float)totalRam)) * 100.0) + 0.5));
        tsgl_gui_text_setText(ramInfo, ramTitle, false);

        sprintf(romTitle, "ROM: %iKB / %iKB", 1, 1);
        tsgl_gui_text_setText(romInfo, romTitle, false);

        infoTab->needDraw = true;
        while (true) {
            tsgl_delay(100);
        }
    }
}

static void _initGui() {
    resetParams();

    // --------------------------------------- create scene

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
    tsgl_gui_button_setStyle(back, TAB_COLOR_ENABLE, tsgl_color_fromHex(0xa0a0a0), tsgl_gui_button_fill);
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

    // --------------------------------------- tab host

    tsgl_gui* tabbar = tsgl_gui_addTabbar(scene, false, 5, 5, 40);
    tsgl_gui_setAllFormat(tabbar, tsgl_gui_absolute);
    tabbar->color = tsgl_color_raw(tsgl_color_fromHex(0x2c2c2c), gui->colormode);
    tabbar->x = 0;
    tabbar->y = 50;
    tabbar->width = tabbar_size;
    tabbar->height = scene->height - 50;

    // --------------------------------------- sound tab

    tsgl_gui* tab = tsgl_gui_tabbar_addTabObject(tabbar, false);
    tab->color = scene->color;

    _tab_sound(tab);

    tsgl_gui* tabButton = tsgl_gui_tabbar_addTabButton(tabbar, TAB_COLOR, TAB_COLOR_ENABLE, tab);
    tsgl_gui_button_setStyle(tabButton, TSGL_BLACK, TSGL_BLACK, tsgl_gui_button_fill);
    tsgl_gui_button_setText(tabButton, TSGL_WHITE, 8, "sound", false);

    // --------------------------------------- gui tab

    tab = tsgl_gui_tabbar_addTabObject(tabbar, false);
    tab->color = scene->color;

    tabButton = tsgl_gui_tabbar_addTabButton(tabbar, TAB_COLOR, TAB_COLOR_ENABLE, tab);
    tsgl_gui_button_setStyle(tabButton, TSGL_BLACK, TSGL_BLACK, tsgl_gui_button_fill);
    tsgl_gui_button_setText(tabButton, TSGL_WHITE, 8, "gui", false);

    // --------------------------------------- connect tab

    tab = tsgl_gui_tabbar_addTabObject(tabbar, false);
    tab->color = scene->color;

    _tab_connect(tab);

    tabButton = tsgl_gui_tabbar_addTabButton(tabbar, TAB_COLOR, TAB_COLOR_ENABLE, tab);
    tsgl_gui_button_setStyle(tabButton, TSGL_BLACK, TSGL_BLACK, tsgl_gui_button_fill);
    tsgl_gui_button_setText(tabButton, TSGL_WHITE, 8, "connect", false);

    // --------------------------------------- power tab

    tab = tsgl_gui_tabbar_addTabObject(tabbar, false);
    tab->color = scene->color;

    tabButton = tsgl_gui_tabbar_addTabButton(tabbar, TAB_COLOR, TAB_COLOR_ENABLE, tab);
    tsgl_gui_button_setStyle(tabButton, TSGL_BLACK, TSGL_BLACK, tsgl_gui_button_fill);
    tsgl_gui_button_setText(tabButton, TSGL_WHITE, 8, "power", false);

    // --------------------------------------- info tab

    infoTab = tsgl_gui_tabbar_addTabObject(tabbar, false);
    infoTab->color = scene->color;

    _tab_info(infoTab);

    tabButton = tsgl_gui_tabbar_addTabButton(tabbar, TAB_COLOR, TAB_COLOR_ENABLE, infoTab);
    tsgl_gui_button_setStyle(tabButton, TSGL_BLACK, TSGL_BLACK, tsgl_gui_button_fill);
    tsgl_gui_button_setText(tabButton, TSGL_WHITE, 8, "info", false);
}

void app_settings_init() {
    FILE* file = tsgl_filesystem_open(settingsPath, "rb");
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
}

void app_settings_open() {
    _initGui();
    xTaskCreate(_infoUpdateTask, NULL, 4096, NULL, 1, &infoUpdateTask_handle);
    tsgl_gui_select(scene);
}

void app_settings_save() {
    FILE* file = tsgl_filesystem_open(settingsPath, "wb");
    if (file) {
        fwrite(&currentVersion, 1, 1, file);
        fwrite(&currentSettings, 1, sizeof(app_settings_struct), file);
        fclose(file);
    }
}