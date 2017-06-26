#ifndef EDITOR_H
#define EDITOR_H

#include "base.h"
#include "action.h"
#include "text_selection.h"
#include "../../../OTG/src/h/OTG.h"
#include "../../../OSAL/src/h/input.h"

typedef struct page_tab page_tab;
page_tab *ctor_page_tab(editor *e, char *filepath);

typedef struct editor editor;
editor *ctor_editor();
void dtor_editor(editor *e);

void editor_set_current_page_tab(editor *e, page_tab *p);
void editor_set_cursor_position(editor *e, s32 x, s32 y);
s32 editor_get_lines_size(editor *e);
char *editor_get_line(editor *e, s32 line_y);
window *editor_get_window(editor *e);
ttf_font *editor_get_font(editor *e);
entity *editor_get_text_holder(editor *e);
text_selection *editor_get_text_selection(editor *e);
void editor_set_text_selection(editor *e, text_selection *ts);
void editor_set_line(editor *e,s32 y,char *str);
entity *editor_get_highest(editor *e);
entity *editor_get_root(editor *e);
void editor_remove_line(editor *e,s32 y);
void editor_insert_line(editor *e, s32 y, char *str);
char *editor_get_text(editor *focused_editor,vec2 begin, vec2 end);
/*
if you pass the end of one line and the beginning of the next line it concats them
*/
void editor_delete_text(editor *focused_editor, vec2 begin, vec2 end, bool do_add_action, u32 type);
/*
@todo change to pass position explicitly
dont free clip, it is passed to an action which frees it
@leak if false is passed to do add action clipn needs to be freed doesnt it?
*/
void editor_add_text(editor *focused_editor, char *clip, bool do_add_action);
vec2 editor_position_to_cursor(editor *focused_editor, vec2 mouse_position);
void editor_add_action(editor *e, action *a);
void editor_do_action(editor *e, bool un);

bool editor_process_keys(editor *e, event ev);
s32 editor_handle_keys(editor *e, event ev);
void editor_handle_mouse_motion(editor *e, vec2 mouse_position);
void editor_handle_mouse_wheel(editor *e, f32 mousewheel);
void editor_handle_left_mouse_up(editor *e);
void editor_handle_left_mouse_down(editor *e, vec2 mouse_position);

void editor_update(editor *e);
void editor_draw(editor *e);

#endif
