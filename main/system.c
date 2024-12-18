// -------------------------------- display settings

#include "system.h"
#include "apps.h"
#include <TSGL_drivers/st77XX.h>

#define SPI    TSGL_HOST1
#define FREQ   80000000
#define BUFFER TSGL_SPIRAM
#define DC     21
#define CS     22
#define RST    33
#define BL     4

#define WIDTH 320
#define HEIGHT 480
#define ROTATION 1

static tsgl_display_settings settings = {
    .driver = &st77XX_rgb565,
    .swapRGB = true,
    .invert = true,
    .flipX = true,
    .width = WIDTH,
    .height = HEIGHT
};

// -------------------------------- touchscreen settings

#define TS_SDA   5
#define TS_SCL   27
#define TS_HOST  I2C_NUM_0
#define TS_ADDR  0x38
#define TS_RST   23

// -------------------------------- other

#define POWERLOCK 32

#define RED  18
#define BLUE 19

#define FIRST 34
#define LAST  35

#define LEFT_SPEAKER  25
#define RIGHT_SPEAKER 26
#define LEFT_SPEAKER_DAC  0
#define RIGHT_SPEAKER_DAC 1

// --------------------------------

tsgl_gui* gui;

tsgl_display display;
tsgl_framebuffer framebuffer;
tsgl_framebuffer framebuffer2;
tsgl_benchmark benchmark;
tsgl_touchscreen touchscreen = {
    .width = WIDTH,
    .height = HEIGHT
};
tsgl_keyboard keyboard;
tsgl_sound_output* left_speaker;
tsgl_sound_output* right_speaker;
tsgl_sound_output* speakers[2];

static tsgl_ledc led_red;
static tsgl_ledc led_blue;

void system_bigText(tsgl_framebuffer* framebuffer, const char* text) {
    tsgl_print_settings print = {
        .font = tsgl_font_defaultFont,
        .fill = TSGL_INVALID_RAWCOLOR,
        .bg = TSGL_INVALID_RAWCOLOR,
        .fg = tsgl_color_raw(tsgl_color_fromHex(0xda02c1), settings.driver->colormode),
        .multiline = true,
        .globalCentering = true,
        .locationMode = tsgl_print_start_top
    };
    print.width = framebuffer->width;
    print.height = framebuffer->height;
    print.targetHeight = framebuffer->width / 6;
    tsgl_framebuffer_clear(framebuffer, tsgl_color_raw(tsgl_color_fromHex(0x091078), settings.driver->colormode));
    tsgl_framebuffer_text(framebuffer, 0, 0, print, text);
}

static void _loop(void* parameter) {
    uint8_t oldTouchCount = 0;
    while (true) {
        tsgl_keyboard_readAll(&keyboard);
        tsgl_gui_processTouchscreen(gui, &touchscreen);
        uint8_t touchCount = tsgl_touchscreen_touchCount(&touchscreen);
        if (touchCount > 0 && oldTouchCount == 0) {
            system_playSoundFromList(system_sound_click, false);
        }
        oldTouchCount = touchCount;
        tsgl_gui_processGui(gui, NULL, &benchmark, 0);
    }
}

void system_init() {
    ESP_ERROR_CHECK(tsgl_framebuffer_init(&framebuffer, settings.driver->colormode, settings.width, settings.height, BUFFER));
    tsgl_framebuffer_rotate(&framebuffer, ROTATION);
    system_bigText(&framebuffer, "SMART\nSPEAKER");
    tsgl_framebuffer_rotate(&framebuffer, 0);

    settings.init_state = tsgl_display_init_framebuffer;
    settings.init_framebuffer_ptr = framebuffer.buffer;
    settings.init_framebuffer_size = framebuffer.buffersize;

    settings.backlight_init = true;
    settings.backlight_pin = BL;
    settings.backlight_value = 0;

    ESP_ERROR_CHECK(tsgl_spi_init(tsgl_math_maxSendSize(settings), SPI));
    ESP_ERROR_CHECK(tsgl_display_spi(&display, settings, SPI, FREQ, DC, CS, RST));
    tsgl_display_incompleteSending(&display, false, NULL);
    tsgl_display_setBacklight(&display, 255);
    ESP_ERROR_CHECK(tsgl_i2c_init(TS_HOST, TS_SDA, TS_SCL));
    ESP_ERROR_CHECK(tsgl_touchscreen_ft6336u(&touchscreen, TS_HOST, TS_ADDR, TS_RST));
    ESP_ERROR_CHECK(tsgl_framebuffer_init(&framebuffer2, display.colormode, settings.width, settings.height, BUFFER));
    ESP_ERROR_CHECK(tsgl_filesystem_mount_fatfs("/storage", "storage"));

    tsgl_framebuffer_hardwareRotate(&framebuffer, ROTATION);
    tsgl_display_rotate(&display, ROTATION);
    touchscreen.rotation = ROTATION;

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask |= 1ULL << POWERLOCK;
    io_conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&io_conf);
    gpio_set_level(POWERLOCK, false);

    tsgl_ledc_new(&led_red, RED, false, 0);
    tsgl_ledc_new(&led_blue, BLUE, false, 0);

    tsgl_keyboard_init(&keyboard);
    tsgl_keyboard_bindButton(&keyboard, 'A', false, false, FIRST);
    tsgl_keyboard_bindButton(&keyboard, 'B', false, false, LAST);
    tsgl_keyboard_setDebounce(&keyboard, 'A', 20, 50);
    tsgl_keyboard_setDebounce(&keyboard, 'B', 20, 50);

    left_speaker = tsgl_sound_newDacOutput(LEFT_SPEAKER_DAC);
    right_speaker = tsgl_sound_newDacOutput(RIGHT_SPEAKER_DAC);
    speakers[0] = left_speaker;
    speakers[1] = right_speaker;
    system_playSoundFromList(system_sound_load, false);

    gui = tsgl_gui_createRoot_buffer(&display, &framebuffer);
    apps_init();
    app_desktop_open();
    xTaskCreate(_loop, NULL, 4096, NULL, 24, NULL);
}

void system_powerOff() {
    system_bigText(&framebuffer, "goodbye!");
    tsgl_display_send(&display, &framebuffer);
    system_playSoundFromList(system_sound_shutdown, true);

    tsgl_display_setBacklight(&display, 0);
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask |= 1ULL << POWERLOCK;
    io_conf.mode = GPIO_MODE_DISABLE;
    gpio_config(&io_conf);
    while (true) vTaskDelay(1);
}

void system_setRed(uint8_t value) {
    tsgl_ledc_set(&led_red, value);
}

void system_setBlue(uint8_t value) {
    tsgl_ledc_set(&led_blue, value);
}


#define MAX_SOUND_PLAYS 4

typedef struct {
    tsgl_sound sound;
    char* path;
} PlaySound;

static PlaySound playSounds[MAX_SOUND_PLAYS];
static uint8_t currentPlaySound = 0;

void system_playSound(const char* path, bool wait, float volume) {
    PlaySound* playSound = NULL;
    for (size_t i = 0; i < MAX_SOUND_PLAYS; i++) {
        PlaySound* lPlaySound = &playSounds[i];
        if (lPlaySound->path && strcmp(path, lPlaySound->path) == 0) {
            playSound = lPlaySound;
            break;
        }
    }

    if (playSound == NULL) {
        playSound = &playSounds[currentPlaySound];
        currentPlaySound++;
        if (currentPlaySound >= MAX_SOUND_PLAYS)
            currentPlaySound = 0;
    }

    if (playSound->path) {
        if (strcmp(path, playSound->path) != 0) {
            tsgl_sound_free(&playSound->sound);
            free(playSound->path);
            playSound->path = NULL;
        }
    }

    if (!playSound->path) {
        tsgl_sound_load_pcm(&playSound->sound, TSGL_SOUND_FULLBUFFER, TSGL_SPIRAM, path, 8000, 1, 1, tsgl_sound_pcm_unsigned);
        playSound->path = malloc(strlen(path) + 1);
        strcpy(playSound->path, path);
    }
    
    if (playSound->sound.playing) {
        tsgl_sound_stop(&playSound->sound);
        tsgl_sound_setPosition(&playSound->sound, 0);
    }

    tsgl_sound_setOutputs(&playSound->sound, speakers, 2, false);
    tsgl_sound_setVolume(&playSound->sound, volume);
    tsgl_sound_play(&playSound->sound);

    while (wait && playSound->sound.playing) vTaskDelay(1);
}

void system_playSoundFromList(system_sound sound, bool wait) {
    bool enable = false;
    switch (sound) {
        case system_sound_click:
            if (currentSettings.sound_enable_click) {
                system_playSound("/storage/click.wav", wait, currentSettings.sound_volume_click);
                enable = true;
            }
            break;

        case system_sound_connect:
            if (currentSettings.sound_enable_connect) {
                system_playSound("/storage/cnct.wav", wait, currentSettings.sound_volume_connect);
                enable = true;
            }
            break;

        case system_sound_disconnect:
            if (currentSettings.sound_enable_disconnect) {
                system_playSound("/storage/discnct.wav", wait, currentSettings.sound_volume_disconnect);
                enable = true;
            }
            break;

        case system_sound_load:
            if (currentSettings.sound_enable_load) {
                system_playSound("/storage/load.wav", wait, currentSettings.sound_volume_load);
                enable = true;
            }
            break;

        case system_sound_shutdown:
            if (currentSettings.sound_enable_shutdown) {
                system_playSound("/storage/shutdown.wav", wait, currentSettings.sound_volume_shutdown);
                enable = true;
            }
            break;
    }
    if (!enable && wait) tsgl_delay(1000);
}