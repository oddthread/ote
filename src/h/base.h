#ifndef BASE_H
#define BASE_H

#include "../../../OSAL/src/h/util.h"
#include <stdio.h>

extern u32 num_spaces_to_insert_on_tab;
extern char ote_log_buffer[100];
extern u32 const LOG_TYPE_ACTION;
extern u32 const LOG_TYPE_STRINGS;
extern u32 const LOG_TYPE_OTE;
extern bool log_type_enabled[3];

extern s32 const EXIT_CODE_ALL_WINDOWS_CLOSED;

extern bool const AUTO_CURLY_BRACE;
extern bool const AUTO_INDENT;

extern s32 const LINES_ON_PAGE_DOWNUP;

void ote_log(char *text,u32 type);

extern char const *global_font_url;
extern u32 global_font_size;
extern s32 global_text_margin;/*if 0 line numbers wont render*/


typedef struct keystate_interpreter_info
{
    bool shift_pressed;
    bool alt_pressed;//?
    bool ctrl_pressed;
    s64 *pressed_keys;
    u32 pressed_keys_size;
} keystate_interpreter_info;

#endif
