/*This file was automatically generated.*/
#ifndef d_text_selection_h
#define d_text_selection_h

void editor_set_current_text_selection(editor *e,text_selection *ts);
text_selection *editor_get_current_text_selection(editor *e);
void dtor_text_selection(editor *e);
entity *editor_get_text_holder(editor *e);
ttf_font *editor_get_font(editor *e);
char *editor_get_line(editor *e,s32 line_y);
s32 editor_get_lines_size(editor *e);
window *editor_get_window(editor *e);
text_selection *ctor_text_selection(vec2 start_position,vec2 end_position,editor *focused_editor);

#endif
