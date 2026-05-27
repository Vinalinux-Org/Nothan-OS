/*
 * drivers/input/keyboard/hid_keyboard.c — HID to Input Translator
 *
 * Translates USB HID boot protocol scancodes into generic
 * input event keycodes.
 */

#include "types.h"
#include "nothan/input.h"
#include "nothan/hid.h"

static const uint16_t hid_to_keymap[128] = {
    [0x00] = KEY_RESERVED,
    [0x04] = KEY_A,
    [0x05] = KEY_B,
    [0x06] = KEY_C,
    [0x07] = KEY_D,
    [0x08] = KEY_E,
    [0x09] = KEY_F,
    [0x0A] = KEY_G,
    [0x0B] = KEY_H,
    [0x0C] = KEY_I,
    [0x0D] = KEY_J,
    [0x0E] = KEY_K,
    [0x0F] = KEY_L,
    [0x10] = KEY_M,
    [0x11] = KEY_N,
    [0x12] = KEY_O,
    [0x13] = KEY_P,
    [0x14] = KEY_Q,
    [0x15] = KEY_R,
    [0x16] = KEY_S,
    [0x17] = KEY_T,
    [0x18] = KEY_U,
    [0x19] = KEY_V,
    [0x1A] = KEY_W,
    [0x1B] = KEY_X,
    [0x1C] = KEY_Y,
    [0x1D] = KEY_Z,
    [0x1E] = KEY_1,
    [0x1F] = KEY_2,
    [0x20] = KEY_3,
    [0x21] = KEY_4,
    [0x22] = KEY_5,
    [0x23] = KEY_6,
    [0x24] = KEY_7,
    [0x25] = KEY_8,
    [0x26] = KEY_9,
    [0x27] = KEY_0,
    [0x28] = KEY_ENTER,
    [0x29] = KEY_ESC,
    [0x2A] = KEY_BACKSPACE,
    [0x2B] = KEY_TAB,
    [0x2C] = KEY_SPACE,
    [0x2D] = KEY_MINUS,
    [0x2E] = KEY_EQUAL,
    [0x2F] = KEY_LEFTBRACE,
    [0x30] = KEY_RIGHTBRACE,
    [0x31] = KEY_BACKSLASH,
    [0x33] = KEY_SEMICOLON,
    [0x34] = KEY_APOSTROPHE,
    [0x35] = KEY_GRAVE,
    [0x36] = KEY_COMMA,
    [0x37] = KEY_DOT,
    [0x38] = KEY_SLASH,
    [0x39] = KEY_CAPSLOCK,
};

static uint8_t prev_keys[6];
static uint8_t prev_modifiers = 0;

void hid_keyboard_process_report(const struct hid_keyboard_report *report)
{
    int i;
    int j;
    int found;
    uint8_t key;
    uint16_t code;
    uint8_t mod_diff;

    mod_diff = prev_modifiers ^ report->modifiers;
    if (mod_diff) {
        if (mod_diff & (1 << 1)) /* Left Shift */
            input_report_key(KEY_LEFTSHIFT, (report->modifiers & (1 << 1)) ? 1 : 0);
        if (mod_diff & (1 << 5)) /* Right Shift */
            input_report_key(KEY_RIGHTSHIFT, (report->modifiers & (1 << 5)) ? 1 : 0);
        prev_modifiers = report->modifiers;
    }

    for (i = 0; i < 6; i++) {
        key = prev_keys[i];
        if (key == 0)
            continue;

        found = 0;
        for (j = 0; j < 6; j++) {
            if (report->keys[j] == key) {
                found = 1;
                break;
            }
        }

        if (!found && key < 128) {
            code = hid_to_keymap[key];
            if (code != KEY_RESERVED) {
                input_report_key(code, 0);
            }
        }
    }

    for (i = 0; i < 6; i++) {
        key = report->keys[i];
        if (key == 0)
            continue;

        found = 0;
        for (j = 0; j < 6; j++) {
            if (prev_keys[j] == key) {
                found = 1;
                break;
            }
        }

        if (key < 128) {
            code = hid_to_keymap[key];
            if (code != KEY_RESERVED) {
                if (!found) {
                    input_report_key(code, 1);
                }
            }
        }
    }

    for (i = 0; i < 6; i++)
        prev_keys[i] = report->keys[i];

    input_sync();
}
