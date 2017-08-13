/*This file was automatically generated.*/
#ifndef d_editor_h
#define d_editor_h

extern u32 const LOG_TYPE_ACTION;
void ote_log(char *text,u32 type);
extern char ote_log_buffer[1000];
action *ctor_action(vec2 cursor_position,vec2 optional_end_position,char *text,u32 type);
void editor_add_action(editor *e,action *a);
extern s32 const LINES_ON_PAGE_DOWNUP;
extern u32 num_spaces_to_insert_on_tab;
extern bool const AUTO_CURLY_BRACE;
s32 indentation_level_spaces(char *line);
extern bool const AUTO_INDENT;
void editor_do_action(editor *e,bool un);
char *editor_get_text(editor *focused_editor,vec2 begin,vec2 end);
extern s32 const REQUEST_SPAWN_EDITOR;
s32 editor_handle_keys(editor *e,event ev);
void editor_delete_text(editor *focused_editor,vec2 begin,vec2 end,bool do_add_action,u32 type);
bool editor_process_keys(editor *e,event ev);
bool editor_handle_left_mouse_down(editor *e,vec2 mouse_position);
void editor_handle_left_mouse_up(editor *e);
void editor_handle_mouse_wheel(editor *e,r32 mousewheel);
vec2 editor_position_to_cursor(editor *focused_editor,vec2 mouse_position);
extern r32 standard_button_hover_alpha;
void editor_insert_line(editor *focused_editor,s32 y,char *str);
void editor_remove_line(editor *focused_editor,s32 y);
entity *editor_get_root(editor *e);
entity *editor_get_highest(editor *e);
void editor_set_line(editor *e,s32 y,char *str);
void editor_set_current_text_selection(editor *e,text_selection *ts);
text_selection *editor_get_current_text_selection(editor *e);
entity *editor_get_text_holder(editor *e);
ttf_font *editor_get_font(editor *e);
window *editor_get_window(editor *e);
void editor_draw(editor *e_instance);
void editor_handle_mouse_motion(editor *e,vec2 mouse_position);
void editor_update(editor *e_instance);
void editor_set_mouse_dirty(editor *e);
page_tab *editor_get_current_page_tab(editor *e);
extern r32 standard_button_selected_alpha;
extern r32 standard_button_alpha;
void dtor_editor(editor *e);
editor *ctor_editor();
extern s32 standard_button_height;
extern s32 standard_button_width;
void dtor_action(action *a);
void dtor_page_tab(page_tab *p);
void editor_add_text(editor *focused_editor,char *clip,bool do_add_action);
void editor_set_current_page_tab(editor *e,page_tab *p);
extern s32 global_text_margin;
extern char const *global_font_url;
extern u32 global_font_size;
page_tab *ctor_page_tab(editor *e,char *filepath);
text_selection *ctor_text_selection(vec2 start_position,vec2 end_position,editor *focused_editor);
void dtor_text_selection(editor *e);
char *editor_get_line(editor *e,s32 line_y);
s32 editor_get_lines_size(editor *e);
void editor_set_cursor_position(editor *e,s32 x,s32 y);

#endif
