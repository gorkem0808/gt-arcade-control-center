#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "tusb.h"
#include "mouse_usb_descriptors.h"

#ifndef PLAYER_ID
#define PLAYER_ID 1
#endif

#define PIN_X_ADC       26
#define PIN_Y_ADC       27
#define PIN_ENABLE      20
#define ADC_MIN_DEFAULT 120
#define ADC_MAX_DEFAULT 3975
#define HID_MAX         32767
#define SEND_INTERVAL_MS 5
#define STATUS_INTERVAL_MS 50

#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define CONFIG_MAGIC 0x474D4331u // GMC1

typedef struct __attribute__((packed)) {
    uint8_t buttons;
    uint16_t x;
    uint16_t y;
} abs_mouse_report_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t x_min;
    uint16_t x_max;
    uint16_t y_min;
    uint16_t y_max;
    uint8_t filter_shift;
    uint8_t invert_x;
    uint8_t invert_y;
    uint8_t reserved[5];
} config_t;

static config_t cfg;
static uint32_t filt_x = 0;
static uint32_t filt_y = 0;
static bool filter_ready = false;
static uint16_t last_raw_x = 0;
static uint16_t last_raw_y = 0;
static uint16_t last_hid_x = 0;
static uint16_t last_hid_y = 0;

static uint32_t ms_now(void) {
    return to_ms_since_boot(get_absolute_time());
}

static void default_config(void) {
    cfg.magic = CONFIG_MAGIC;
    cfg.x_min = ADC_MIN_DEFAULT;
    cfg.x_max = ADC_MAX_DEFAULT;
    cfg.y_min = ADC_MIN_DEFAULT;
    cfg.y_max = ADC_MAX_DEFAULT;
    cfg.filter_shift = 2;
    cfg.invert_x = 0;
    cfg.invert_y = 0;
}

static void load_config(void) {
    const config_t *stored = (const config_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
    if (stored->magic == CONFIG_MAGIC && stored->x_max > stored->x_min + 10 && stored->y_max > stored->y_min + 10) {
        memcpy(&cfg, stored, sizeof(config_t));
        if (cfg.filter_shift > 6) cfg.filter_shift = 2;
    } else {
        default_config();
    }
}

static void save_config(void) {
    uint8_t sector[FLASH_SECTOR_SIZE];
    memset(sector, 0xFF, sizeof(sector));
    memcpy(sector, &cfg, sizeof(config_t));
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, sector, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
}

static uint16_t clamp_u16_i32(int32_t v, int32_t lo, int32_t hi) {
    if (v < lo) return (uint16_t)lo;
    if (v > hi) return (uint16_t)hi;
    return (uint16_t)v;
}

static uint16_t read_adc_channel(uint channel) {
    adc_select_input(channel);
    sleep_us(10);
    uint32_t sum = 0;
    for (int i = 0; i < 8; i++) {
        sum += adc_read();
        sleep_us(40);
    }
    return (uint16_t)(sum / 8);
}

static uint16_t map_adc_to_hid(uint16_t raw, uint16_t minv, uint16_t maxv, bool invert) {
    if (maxv <= minv + 10) return HID_MAX / 2;
    int32_t v = raw;
    if (v < minv) v = minv;
    if (v > maxv) v = maxv;
    int32_t out = (v - minv) * HID_MAX / (maxv - minv);
    if (invert) out = HID_MAX - out;
    return clamp_u16_i32(out, 0, HID_MAX);
}

static void update_values(void) {
    last_raw_x = read_adc_channel(0);
    last_raw_y = read_adc_channel(1);

    if (!filter_ready) {
        filt_x = last_raw_x;
        filt_y = last_raw_y;
        filter_ready = true;
    } else {
        uint8_t sh = cfg.filter_shift;
        if (sh > 6) sh = 2;
        filt_x = filt_x + (((int32_t)last_raw_x - (int32_t)filt_x) >> sh);
        filt_y = filt_y + (((int32_t)last_raw_y - (int32_t)filt_y) >> sh);
    }

    last_hid_x = map_adc_to_hid((uint16_t)filt_x, cfg.x_min, cfg.x_max, cfg.invert_x);
    last_hid_y = map_adc_to_hid((uint16_t)filt_y, cfg.y_min, cfg.y_max, cfg.invert_y);
}

static void send_abs_mouse(void) {
    if (!tud_hid_ready()) return;
    if (gpio_get(PIN_ENABLE) != 0) return; // GP20 GND = aktif. Pasifken normal PC mouse serbest.

    abs_mouse_report_t rpt = {
        .buttons = 0,
        .x = last_hid_x,
        .y = last_hid_y
    };
    tud_hid_report(REPORT_ID_MOUSE, &rpt, sizeof(rpt));
}

static void cdc_write(const char *s) {
    if (!tud_cdc_connected()) return;
    tud_cdc_write_str(s);
    tud_cdc_write_flush();
}

static void send_status(void) {
    if (!tud_cdc_connected()) return;
    char buf[192];
    snprintf(buf, sizeof(buf),
        "STATUS,MOUSE,P%d,RAW,%u,%u,HID,%u,%u,ACTIVE,%d,CAL,%u,%u,%u,%u,FILTER,%u\n",
        PLAYER_ID,
        last_raw_x, last_raw_y, last_hid_x, last_hid_y,
        gpio_get(PIN_ENABLE) == 0,
        cfg.x_min, cfg.x_max, cfg.y_min, cfg.y_max, cfg.filter_shift);
    cdc_write(buf);
}

static void handle_line(char *line) {
    if (strcmp(line, "PING") == 0) {
        char b[64];
        snprintf(b, sizeof(b), "HELLO,MOUSE,P%d\n", PLAYER_ID);
        cdc_write(b);
        return;
    }
    if (strcmp(line, "GET") == 0) {
        send_status();
        return;
    }
    if (strncmp(line, "SETCAL,", 7) == 0) {
        unsigned xmn, xmx, ymn, ymx;
        if (sscanf(line + 7, "%u,%u,%u,%u", &xmn, &xmx, &ymn, &ymx) == 4) {
            if (xmx > xmn + 10 && ymx > ymn + 10) {
                cfg.x_min = (uint16_t)xmn;
                cfg.x_max = (uint16_t)xmx;
                cfg.y_min = (uint16_t)ymn;
                cfg.y_max = (uint16_t)ymx;
                save_config();
                cdc_write("OK,SETCAL\n");
            } else {
                cdc_write("ERR,SETCAL_RANGE\n");
            }
        }
        return;
    }
    if (strncmp(line, "FILTER,", 7) == 0) {
        int f = atoi(line + 7);
        if (f < 0) f = 0;
        if (f > 6) f = 6;
        cfg.filter_shift = (uint8_t)f;
        save_config();
        cdc_write("OK,FILTER\n");
        return;
    }
    if (strcmp(line, "RESETCAL") == 0) {
        default_config();
        save_config();
        cdc_write("OK,RESETCAL\n");
        return;
    }
}

static void poll_cdc(void) {
    static char line[128];
    static uint8_t pos = 0;
    while (tud_cdc_available()) {
        char c;
        tud_cdc_read(&c, 1);
        if (c == '\n' || c == '\r') {
            if (pos) {
                line[pos] = 0;
                handle_line(line);
                pos = 0;
            }
        } else if (pos < sizeof(line) - 1) {
            line[pos++] = c;
        }
    }
}

int main(void) {
    tusb_init();
    adc_init();
    adc_gpio_init(PIN_X_ADC);
    adc_gpio_init(PIN_Y_ADC);

    gpio_init(PIN_ENABLE);
    gpio_set_dir(PIN_ENABLE, GPIO_IN);
    gpio_pull_up(PIN_ENABLE);

    load_config();

    uint32_t last_mouse = 0;
    uint32_t last_status = 0;

    while (1) {
        tud_task();
        poll_cdc();
        update_values();

        uint32_t now = ms_now();
        if (now - last_mouse >= SEND_INTERVAL_MS) {
            last_mouse = now;
            send_abs_mouse();
        }
        if (now - last_status >= STATUS_INTERVAL_MS) {
            last_status = now;
            send_status();
        }
    }
}
