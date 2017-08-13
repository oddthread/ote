/*This file was automatically generated.*/
#ifndef d_base_h
#define d_base_h

extern r32 standard_button_selected_alpha;
extern r32 standard_button_hover_alpha;
extern r32 standard_button_alpha;
extern s32 standard_button_height;
extern s32 standard_button_width;
extern s32 global_text_margin;
extern u32 global_font_size;
extern char const *global_font_url;
s32 indentation_level_spaces(char *line);
void ote_log(char *text,u32 type);
extern s32 const REQUEST_SPAWN_EDITOR;
extern s32 const LINES_ON_PAGE_DOWNUP;
extern bool const AUTO_INDENT;
extern bool const AUTO_CURLY_BRACE;
extern s32 const EXIT_CODE_ALL_WINDOWS_CLOSED;
extern bool log_type_enabled[3];
extern u32 const LOG_TYPE_OTE;
extern u32 const LOG_TYPE_STRINGS;
extern u32 const LOG_TYPE_ACTION;
extern char ote_log_buffer[1000];
extern u32 num_spaces_to_insert_on_tab;

#endif
