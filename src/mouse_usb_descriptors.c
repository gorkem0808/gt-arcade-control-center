#include <string.h>
#include "tusb.h"
#include "mouse_usb_descriptors.h"

#ifndef PLAYER_ID
#define PLAYER_ID 1
#endif

#define USB_VID 0xCafe
#define USB_PID (0x4020 + PLAYER_ID)
#define USB_BCD 0x0200

// Eski çalışan çekirdeğe yakın Absolute HID Mouse: buttons + X/Y 0..32767.
uint8_t const desc_hid_report[] = {
  0x05, 0x01,
  0x09, 0x02,
  0xA1, 0x01,
  0x85, REPORT_ID_MOUSE,
  0x09, 0x01,
  0xA1, 0x00,

  0x05, 0x09,
  0x19, 0x01,
  0x29, 0x03,
  0x15, 0x00,
  0x25, 0x01,
  0x95, 0x03,
  0x75, 0x01,
  0x81, 0x02,
  0x95, 0x01,
  0x75, 0x05,
  0x81, 0x03,

  0x05, 0x01,
  0x09, 0x30,
  0x09, 0x31,
  0x16, 0x00, 0x00,
  0x26, 0xFF, 0x7F,
  0x36, 0x00, 0x00,
  0x46, 0xFF, 0x7F,
  0x75, 0x10,
  0x95, 0x02,
  0x81, 0x02,

  0xC0,
  0xC0
};

uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
  (void) instance;
  return desc_hid_report;
}

tusb_desc_device_t const desc_device = {
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = USB_BCD,
  .bDeviceClass       = TUSB_CLASS_MISC,
  .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
  .bDeviceProtocol    = MISC_PROTOCOL_IAD,
  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor           = USB_VID,
  .idProduct          = USB_PID,
  .bcdDevice          = 0x0100,
  .iManufacturer      = 0x01,
  .iProduct           = 0x02,
  .iSerialNumber      = 0x03,
  .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void) {
  return (uint8_t const*) &desc_device;
}

enum {
  ITF_NUM_CDC = 0,
  ITF_NUM_CDC_DATA,
  ITF_NUM_HID,
  ITF_NUM_TOTAL
};

#define EPNUM_CDC_NOTIF 0x81
#define EPNUM_CDC_OUT   0x02
#define EPNUM_CDC_IN    0x82
#define EPNUM_HID       0x83
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN)

uint8_t const desc_configuration[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
  TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_MOUSE, sizeof(desc_hid_report), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 5)
};

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
  (void) index;
  return desc_configuration;
}

char const* string_desc_arr[] = {
  (const char[]) { 0x09, 0x04 },
  "GT Elektronik Yazilim",
#if PLAYER_ID == 1
  "GT ARCADE P1 ABSOLUTE MOUSE",
  "GT-P1-MOUSE",
#else
  "GT ARCADE P2 ABSOLUTE MOUSE",
  "GT-P2-MOUSE",
#endif
  "GT Arcade CDC"
};

static uint16_t _desc_str[64];
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void) langid;
  uint8_t chr_count;

  if (index == 0) {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  } else {
    if (index >= sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) return NULL;
    const char* str = string_desc_arr[index];
    chr_count = (uint8_t) strlen(str);
    if (chr_count > 31) chr_count = 31;
    for (uint8_t i = 0; i < chr_count; i++) _desc_str[1+i] = str[i];
  }

  _desc_str[0] = (TUSB_DESC_STRING << 8) | (2*chr_count + 2);
  return _desc_str;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t* buffer, uint16_t reqlen) {
  (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) reqlen;
  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const* buffer, uint16_t bufsize) {
  (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) bufsize;
}
