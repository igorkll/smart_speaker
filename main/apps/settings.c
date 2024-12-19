#include "settings.h"
#include "../system.h"
#include "../apps.h"

#include <TSGL_gui.h>
#include <TSGL_gui/button.h>
#include <TSGL_gui/sprite.h>
#include <TSGL_gui/colorpicker.h>
#include <TSGL_gui/text.h>
#include <TSGL_gui/lever.h>

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

const char* settingsPath = "/storage/settings.cfg";
static uint8_t currentVersion = 0;
app_settings_struct currentSettings;

#define TAB_COUNT 4
static uint8_t currentTab = 0;
static uint8_t oldTab = 0;
static tsgl_gui* tabButtons[TAB_COUNT];
static tsgl_gui* tabs[TAB_COUNT];
static uint32_t tabButtonColorEnable = 0xcacaca;
static uint32_t tabButtonColorDisable = 0x666666;
static const tsgl_pos tab_host_size = 90;
static bool needSave = false;

static void updateTab(uint8_t i) {
    bool enabled = currentTab == i;

    tsgl_gui* button = tabButtons[i];
    tsgl_gui* tab = tabs[i];

    button->color = tsgl_color_raw(enabled ? tsgl_color_fromHex(tabButtonColorEnable) : tsgl_color_fromHex(tabButtonColorDisable), gui->colormode);
    button->needDraw = true;

    tab->interactive = enabled;
    tab->displayable = enabled;
    tab->needDraw = enabled;
}

static void updateTabs() {
    updateTab(currentTab);
    updateTab(oldTab);
    oldTab = currentTab;
}

static void* callback_onTabChange(tsgl_gui* self, int arg0, void* arg1, void* userArg) {
    if (arg0 == 1) {
        currentTab = *((uint8_t*)userArg);
        updateTabs();
    }
    return NULL;
}

static void* callback_openDesktop(tsgl_gui* self, int arg0, void* arg1, void* userArg) {
    if (arg0 == 0) {
        if (needSave) {
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

static tsgl_pos lastTabY = 5;
static size_t lastIndex = 0;
static void addTab(tsgl_gui* tab_host, tsgl_gui* tab, const char* title) {
    tsgl_gui* tab_button = tsgl_gui_addButton(tab_host);
    tab_button->x = 5;
    tab_button->y = lastTabY;
    tab_button->width = tab_host->width - 10;
    tab_button->height = 40;
    tab_button->animationStopEnable = true;
    tab_button->animationStop = 0;
    tab_button->user_callback = callback_onTabChange;
    uint8_t* tabIndex = malloc(sizeof(uint8_t));
    *tabIndex = lastIndex;
    tab_button->userArg = tabIndex;
    tsgl_gui_button_setStyle(tab_button, tsgl_color_fromHex(tabButtonColorDisable), tsgl_color_fromHex(tabButtonColorDisable), tsgl_gui_button_fill);
    tsgl_gui_button_setText(tab_button, TSGL_WHITE, 8, title, false);

    tabButtons[lastIndex] = tab_button;
    tabs[lastIndex] = tab;

    tab->interactive = lastIndex == 0;
    tab->displayable = lastIndex == 0;

    lastTabY += 45;
    lastIndex++;
}

static tsgl_pos leverPos;
static const tsgl_pos leverPadding = 14;
static tsgl_pos lastLeverY = 14;
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

tsgl_gui* newTab() {
    tsgl_gui* tab = tsgl_gui_addObject(scene);
    tab->x = tab_host_size;
    tab->y = 50;
    tab->width = gui->width - tab_host_size;
    tab->height = gui->height - 50;
    tab->color = tsgl_color_raw(tsgl_color_fromHex(0xa0a0a0), gui->colormode);;
    return tab;
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
    tsgl_gui_button_setStyle(back, tsgl_color_fromHex(tabButtonColorEnable), tsgl_color_fromHex(0xa0a0a0), tsgl_gui_button_fill);
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

    tsgl_gui* tab_host = tsgl_gui_addObject(scene);
    tsgl_gui_setAllFormat(tab_host, tsgl_gui_absolute);
    tab_host->color = tsgl_color_raw(tsgl_color_fromHex(0x2c2c2c), gui->colormode);
    tab_host->x = 0;
    tab_host->y = 50;
    tab_host->width = tab_host_size;
    tab_host->height = scene->height - 50;

    // --------------------------------------- sound tab

    tsgl_gui* tab = newTab();
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
    addTitleLever(tab, "load", &currentSettings.sound_enable_load);
    addTitleLever(tab, "shutdown", &currentSettings.sound_enable_shutdown);
    addTitleLever(tab, "click", &currentSettings.sound_enable_click);
    addTitleLever(tab, "connect", &currentSettings.sound_enable_connect);
    addTitleLever(tab, longName, &currentSettings.sound_enable_disconnect);
    addTab(tab_host, tab, "sound");

    // --------------------------------------- gui tab

    tab = newTab();
    addTab(tab_host, tab, "gui");

    // --------------------------------------- connect tab

    tab = newTab();
    addTab(tab_host, tab, "connect");

    // --------------------------------------- power tab

    tab = newTab();
    addTab(tab_host, tab, "power");

    // ---------------------------------------

    updateTabs();
}

void app_settings_open() {
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