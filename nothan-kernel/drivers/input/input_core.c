/*
 * drivers/input/input_core.c — Input Event Router
 *
 * Receives raw input events from hardware drivers, translates them
 * to ASCII, and routes them to the terminal.
 */

#include "types.h"
#include "nothan/input.h"
#include "nothan/printk.h"
#include "nothan/init.h"

static const char keymap[128] = {
    [KEY_A] = 'a',
    [KEY_B] = 'b',
    [KEY_C] = 'c',
    [KEY_D] = 'd',
    [KEY_E] = 'e',
    [KEY_F] = 'f',
    [KEY_G] = 'g',
    [KEY_H] = 'h',
    [KEY_I] = 'i',
    [KEY_J] = 'j',
    [KEY_K] = 'k',
    [KEY_L] = 'l',
    [KEY_M] = 'm',
    [KEY_N] = 'n',
    [KEY_O] = 'o',
    [KEY_P] = 'p',
    [KEY_Q] = 'q',
    [KEY_R] = 'r',
    [KEY_S] = 's',
    [KEY_T] = 't',
    [KEY_U] = 'u',
    [KEY_V] = 'v',
    [KEY_W] = 'w',
    [KEY_X] = 'x',
    [KEY_Y] = 'y',
    [KEY_Z] = 'z',
    [KEY_1] = '1',
    [KEY_2] = '2',
    [KEY_3] = '3',
    [KEY_4] = '4',
    [KEY_5] = '5',
    [KEY_6] = '6',
    [KEY_7] = '7',
    [KEY_8] = '8',
    [KEY_9] = '9',
    [KEY_0] = '0',
    [KEY_MINUS] = '-',
    [KEY_EQUAL] = '=',
    [KEY_LEFTBRACE] = '[',
    [KEY_RIGHTBRACE] = ']',
    [KEY_SEMICOLON] = ';',
    [KEY_APOSTROPHE] = '\'',
    [KEY_GRAVE] = '`',
    [KEY_BACKSLASH] = '\\',
    [KEY_COMMA] = ',',
    [KEY_DOT] = '.',
    [KEY_SLASH] = '/',
    [KEY_SPACE] = ' '
};

static const char keymap_shift[128] = {
    [KEY_A] = 'A',
    [KEY_B] = 'B',
    [KEY_C] = 'C',
    [KEY_D] = 'D',
    [KEY_E] = 'E',
    [KEY_F] = 'F',
    [KEY_G] = 'G',
    [KEY_H] = 'H',
    [KEY_I] = 'I',
    [KEY_J] = 'J',
    [KEY_K] = 'K',
    [KEY_L] = 'L',
    [KEY_M] = 'M',
    [KEY_N] = 'N',
    [KEY_O] = 'O',
    [KEY_P] = 'P',
    [KEY_Q] = 'Q',
    [KEY_R] = 'R',
    [KEY_S] = 'S',
    [KEY_T] = 'T',
    [KEY_U] = 'U',
    [KEY_V] = 'V',
    [KEY_W] = 'W',
    [KEY_X] = 'X',
    [KEY_Y] = 'Y',
    [KEY_Z] = 'Z',
    [KEY_1] = '!',
    [KEY_2] = '@',
    [KEY_3] = '#',
    [KEY_4] = '$',
    [KEY_5] = '%',
    [KEY_6] = '^',
    [KEY_7] = '&',
    [KEY_8] = '*',
    [KEY_9] = '(',
    [KEY_0] = ')',
    [KEY_MINUS] = '_',
    [KEY_EQUAL] = '+',
    [KEY_LEFTBRACE] = '{',
    [KEY_RIGHTBRACE] = '}',
    [KEY_SEMICOLON] = ':',
    [KEY_APOSTROPHE] = '"',
    [KEY_GRAVE] = '~',
    [KEY_BACKSLASH] = '|',
    [KEY_COMMA] = '<',
    [KEY_DOT] = '>',
    [KEY_SLASH] = '?',
    [KEY_SPACE] = ' '
};

static int shift_pressed = 0;
static int capslock_active = 0;

void input_report_key(uint16_t code, int32_t value)
{
    char c;
    int use_shift;

    if (code == KEY_LEFTSHIFT || code == KEY_RIGHTSHIFT) {
        shift_pressed = value;
        return;
    }

    if (code == KEY_CAPSLOCK && value == 1) {
        capslock_active = !capslock_active;
        return;
    }

    if (value == 0 || code >= 128)
        return;

    if (code == KEY_ENTER) {
        pr_info("\n");
        return;
    }

    if (code == KEY_BACKSPACE) {
        pr_info("\b \b");
        return;
    }

    use_shift = shift_pressed;
    if (keymap[code] >= 'a' && keymap[code] <= 'z' && capslock_active)
        use_shift = !use_shift;

    c = use_shift ? keymap_shift[code] : keymap[code];

    if (c != 0)
        pr_info("%c", c);
}

void input_sync(void)
{
}

static int __init input_core_init(void)
{
    return 0;
}
subsys_initcall(input_core_init);
