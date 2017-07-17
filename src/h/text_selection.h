#ifndef d_text_selection_h
#define d_text_selection_h

#include "opl/src/h/util.h"

typedef struct editor editor;

typedef struct text_selection text_selection;

text_selection *ctor_text_selection(vec2 start_position, vec2 end_position, editor *focused_editor);
//frees from the editor that owns it
void dtor_text_selection(editor *e);

#endif
