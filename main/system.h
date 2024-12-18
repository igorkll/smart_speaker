#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_random.h>
#include <esp_timer.h>
#include <math.h>
#include <string.h>

#include <TSGL.h>
#include <TSGL_benchmark.h>
#include <TSGL_framebuffer.h>
#include <TSGL_touchscreen.h>
#include <TSGL_filesystem.h>
#include <TSGL_sound.h>
#include <TSGL_keyboard.h>
#include <TSGL_display.h>
#include <TSGL_color.h>
#include <TSGL_font.h>
#include <TSGL_ledc.h>
#include <TSGL_spi.h>
#include <TSGL_i2c.h>
#include <TSGL_math.h>
#include <TSGL_gui.h>

#include <TSGL_fonts/default.h>

extern tsgl_gui* gui;

extern tsgl_display display;
extern tsgl_framebuffer framebuffer;
extern tsgl_framebuffer framebuffer2;
extern tsgl_benchmark benchmark;
extern tsgl_touchscreen touchscreen;
extern tsgl_keyboard keyboard;
extern tsgl_sound_output* left_speaker;
extern tsgl_sound_output* right_speaker;
extern tsgl_sound_output* speakers[];

void system_init();
void system_powerOff();
void system_setRed(uint8_t value);
void system_setBlue(uint8_t value);
void system_playSound(const char* path, bool wait, float volume);