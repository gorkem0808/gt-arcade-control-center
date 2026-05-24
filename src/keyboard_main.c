#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "keyboard_usb_descriptors.h"

typedef struct {
    uint pin;
    uint8_t key;
    const char *name;
} key_pin_t;

// HID key codes: 1-0 and A-K
#define KEY_1 0x1E
#define KEY_2 0x1F
#define KEY_3 0x20
#define KEY_4 0x21
#define KEY_5 0x22
#define KEY_6 0x23
#define KEY_7 0x24
#define KEY_8 0x25
#define KEY_9 0x26
#define KEY_0 0x27
#define KEY_A 0x04
#define KEY_B 0x05
#define KEY_C 0x06
#define KEY_D 0x07
#define KEY_E 0x08
#define KEY_F 0x09
#define KEY_G 0x0A
#define KEY_H 0x0B
#define KEY_I 0x0C
#define KEY_J 0x0D
#define KEY_K 0x0E

static const key_pin_t pins[] = {
    {2,  KEY_1, "GP2=1"},
    {3,  KEY_2, "GP3=2"},
    {4,  KEY_3, "GP4=3"},
    {5,  KEY_4, "GP5=4"},
    {6,  KEY_5, "GP6=5"},
    {7,  KEY_6, "GP7=6"},
    {8,  KEY_7, "GP8=7"},
    {17, KEY_8, "GP17=8"},
    {18, KEY_9, "GP18=9"},
    {19, KEY_0, "GP19=0"},
    {9,  KEY_A, "GP9=A"},
    {10, KEY_B, "GP10=B"},
    {11, KEY_C, "GP11=C"},
    {12, KEY_D, "GP12=D"},
    {13, KEY_E, "GP13=E"},
    {14, KEY_F, "GP14=F"},
    {15, KEY_G, "GP15=G"},
    {16, KEY_H, "GP16=H"},
    {21, KEY_I, "GP21=I"},
    {22, KEY_J, "GP22=J"},
    {28, KEY_K, "GP28=K"},
};
#define PIN_COUNT (sizeof(pins)/sizeof(pins[0]))

static uint32_t ms_now(void) {
    return to_ms_since_boot(get_absolute_time());
}

static bool pressed_pin(uint pin) {
    return gpio_get(pin) == 0;
}

static void cdc_write(const char *s) {
    if (!tud_cdc_connected()) return;
    tud_cdc_write_str(s);
    tud_cdc_write_flush();
}

static void send_status(void) {
    if (!tud_cdc_connected()) return;
    char buf[256];
    int n = snprintf(buf, sizeof(buf), "STATUS,KEYBOARD,BTN");
    for (uint i = 0; i < PIN_COUNT && n < (int)sizeof(buf)-4; i++) {
        n += snprintf(buf+n, sizeof(buf)-n, ",%u:%d", pins[i].pin, pressed_pin(pins[i].pin) ? 1 : 0);
    }
    snprintf(buf+n, sizeof(buf)-n, "\n");
    cdc_write(buf);
}

static void handle_line(char *line) {
    if (strcmp(line, "PING") == 0) {
        cdc_write("HELLO,KEYBOARD,BUTTON_BOARD\n");
    } else if (strcmp(line, "GET") == 0) {
        send_status();
    } else if (strcmp(line, "MAP") == 0) {
        cdc_write("INFO,FIXED_MAP,GP2-8=1-7,GP17=8,GP18=9,GP19=0,GP9-16=A-H,GP21=I,GP22=J,GP28=K\n");
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

static void send_keyboard_report(void) {
    if (!tud_hid_ready()) return;

    uint8_t keycode[6] = {0};
    uint8_t count = 0;

    for (uint i = 0; i < PIN_COUNT && count < 6; i++) {
        if (pressed_pin(pins[i].pin)) {
            keycode[count++] = pins[i].key;
        }
    }

    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
}

int main(void) {
    tusb_init();
    for (uint i = 0; i < PIN_COUNT; i++) {
        gpio_init(pins[i].pin);
        gpio_set_dir(pins[i].pin, GPIO_IN);
        gpio_pull_up(pins[i].pin);
    }

    uint32_t last_hid = 0;
    uint32_t last_status = 0;
    while (1) {
        tud_task();
        poll_cdc();
        uint32_t now = ms_now();
        if (now - last_hid >= 10) {
            last_hid = now;
            send_keyboard_report();
        }
        if (now - last_status >= 80) {
            last_status = now;
            send_status();
        }
    }
}
