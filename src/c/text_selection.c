#include "../../../OTG/src/h/OTG.h"
#include "../h/editor.h"

typedef struct text_selection
{
    image_renderer *ir;
    entity **text_selection_entities;
    u32 text_selection_entities_size;
} text_selection;

text_selection *ctor_text_selection(vec2 start_position, vec2 end_position, editor *focused_editor)
{

    text_selection *t=malloc(sizeof(text_selection));
    texture *tex=ctor_texture(focused_editor->win, "res/blue.png");
    t->ir=ctor_image_renderer(focused_editor->win,tex);
    texture_set_alpha(tex,100);
    t->text_selection_entities=NULL;
    t->text_selection_entities_size=0;

    /*
    if(start_position.x==end_position.x && start_position.y == end_position.y)
    {
        return;
    }
    */

    for(u32 y=start_position.y; y<=end_position.y; y++)
    {
        char *curline=focused_editor->lines[y];
        u32 strlen_curline=strlen(curline);

        if(y==start_position.y && y==end_position.y)
        {
            /*single line selection*/
            
            u32 w;
            u32 h;
            char *splice=malloc_str_slice(curline, start_position.x, end_position.x);
            size_ttf_font(focused_editor->font, splice, &w, &h);
            free(splice);

            u32 w2;
            u32 h2;
            char *splice2=malloc_str_slice(curline, 0, start_position.x-1);
            size_ttf_font(focused_editor->font, splice2, &w2, &h2);
            free(splice2);

            t->text_selection_entities_size+=1;
            t->text_selection_entities=realloc(t->text_selection_entities, t->text_selection_entities_size*sizeof(entity*));
            entity *e=ctor_entity(focused_editor->text_holder);
            entity_set_order(e,0);
            t->text_selection_entities[t->text_selection_entities_size-1]=e;
            entity_add_renderer(e,(renderer*)t->ir);
            
            entity_set_size(e,value_vec2(w,h));

            entity_set_position(e,value_vec2(w2+global_text_margin,h*y));
        }
        else if(y==start_position.y)
        {
            u32 w;
            u32 h;
            char *splice=malloc_str_slice(curline, start_position.x, strlen_curline-1);
            size_ttf_font(focused_editor->font, splice, &w, &h);
            free(splice);
            
            u32 w2;
            u32 h2;
            char *splice2=malloc_str_slice(curline, 0, start_position.x-1);
            size_ttf_font(focused_editor->font, splice2, &w2, &h2);
            free(splice2);

            t->text_selection_entities_size+=1;
            t->text_selection_entities=realloc(t->text_selection_entities, t->text_selection_entities_size*sizeof(entity*));
            entity *e=ctor_entity(focused_editor->text_holder);
            entity_set_order(e,0);
            t->text_selection_entities[t->text_selection_entities_size-1]=e;
            entity_add_renderer(e,(renderer*)t->ir);
            entity_set_size(e,value_vec2(w,h));

            entity_set_position(e,value_vec2(w2+global_text_margin,h*y));
        }
        else if(y==end_position.y)
        {
            u32 w;
            u32 h;
            char *splice=malloc_str_slice(curline, 0, end_position.x);
            size_ttf_font(focused_editor->font, splice, &w, &h);
            free(splice);

            t->text_selection_entities_size+=1;
            t->text_selection_entities=realloc(t->text_selection_entities, t->text_selection_entities_size*sizeof(entity*));
            entity *e=ctor_entity(focused_editor->text_holder);
            entity_set_order(e,0);
            t->text_selection_entities[t->text_selection_entities_size-1]=e;
            entity_add_renderer(e,(renderer*)t->ir);
            entity_set_size(e,value_vec2(w,h));

            entity_set_position(e,value_vec2(0+global_text_margin,h*y));
        }
        else
        {
            u32 w;
            u32 h;
            char *splice=malloc_str_slice(curline, 0, strlen(curline));
            size_ttf_font(focused_editor->font, splice, &w, &h);
            free(splice);

            t->text_selection_entities_size+=1;
            t->text_selection_entities=realloc(t->text_selection_entities, t->text_selection_entities_size*sizeof(entity*));
            entity *e=ctor_entity(focused_editor->text_holder);
            entity_set_order(e,0);
            t->text_selection_entities[t->text_selection_entities_size-1]=e;
            entity_add_renderer(e,(renderer*)t->ir);
            entity_set_size(e,value_vec2(w,h));

            entity_set_position(e,value_vec2(0+global_text_margin,h*y));
        }
    }
    return t;
}
void dtor_text_selection(text_selection *s)
{
    dtor_image_renderer(s->ir);
    for(u32 i=0; i<s->text_selection_entities_size; i++)
    {
        dtor_entity(s->text_selection_entities[i]);
    }
    free(s->text_selection_entities);
}
