/*This file was automatically generated.*/
#ifndef d_event_delegator_h
#define d_event_delegator_h

extern s32 const REQUEST_SPAWN_EDITOR;
s32 editor_handle_keys(editor *e,event ev);
bool editor_process_keys(editor *e,event ev);
s32 delegate_event(global_state *gs);
extern s32 const EXIT_CODE_ALL_WINDOWS_CLOSED;
void dtor_editor(editor *e);
window *editor_get_window(editor *e);
void editor_handle_left_mouse_up(editor *e);
bool editor_handle_left_mouse_down(editor *e,vec2 mouse_position);
void editor_handle_mouse_wheel(editor *e,r32 mousewheel);
void editor_handle_mouse_motion(editor *e,vec2 mouse_position);
page_tab *editor_get_current_page_tab(editor *e);
s32 delegate_event_window(global_state *gs);
page_tab *ctor_page_tab(editor *e,char *filepath);
editor *ctor_editor();
global_state *ctor_global_state(int argc,char **argv);

#endif
