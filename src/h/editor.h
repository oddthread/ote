#ifndef EDITOR_H
#define EDITOR_H

#include "base.h"
#include "action.h"
#include "text_selection.h"
#include "../../../OTG/src/h/OTG.h"

typedef struct editor
{
    keystate_interpreter_info keystate;
    
    bool start_selection_mouse;
    bool start_selection_key;
    bool wheel_override;

    action **action_list;
    s32 action_list_size;
    s32 action_list_index;

    text_selection *current_text_selection;
    vec2 text_selection_origin;
    vec2 text_selection_end;

    color font_color;
    bool is_fullscreen;
    char **lines;
    s32 lines_size;
    text_block_renderer *tbr;
    window *win;
    entity *root;
    entity *text_holder;
    entity *text_entity;
    ttf_font *font;

    entity *focused_entity;
    
    s32 cursor_x;
    s32 cursor_y;
    entity *cursor;
    
    bool cursor_blink_state;
    s64 cursor_blink_timer;
    s64 last_time;
    s64 held_key;
    s64 held_key_time_stamp;
    vec2 offset;
    
} editor;

void editor_set_cursor_position(editor *e, s32 x, s32 y);
editor *ctor_editor();
void dtor_editor(editor *e);

void update_editor(editor *e);
void render_editor(editor *e);

/*
if you pass the end of one line and the beginning of the next line it concats them

*/
void delete_text(vec2 begin, vec2 end, editor *focused_editor, bool do_add_action, u32 type);
/*
@todo change to pass position explicitly
dont free clip, it is passed to an action which frees it
*/
void add_text(editor *focused_editor, char *clip, bool do_add_action);

vec2 position_to_cursor(vec2 mouse_position, editor *focused_editor);
char *get_text(vec2 begin, vec2 end, editor *focused_editor);
/*
if you pass the end of one line and the beginning of the next line it concats them

*/
void delete_text(vec2 begin, vec2 end, editor *focused_editor, bool do_add_action, u32 type);
/*
@todo change to pass position explicitly
dont free clip, it is passed to an action which frees it
*/
void add_text(editor *focused_editor, char *clip, bool do_add_action);


vec2 position_to_cursor(vec2 mouse_position, editor *focused_editor);
char *get_text(vec2 begin, vec2 end, editor *focused_editor);

void add_action(editor *e, action *a);
void do_action(editor *e, bool un);
#endif
