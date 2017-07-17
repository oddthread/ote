#include "osg/src/h/osg.h"

#include "opl/src/h/util.h"
#include "opl/src/h/system.h"

#include "ode/src/h/editor.h"

#include <stdlib.h>
#include <string.h>

typedef struct text_selection
{
    image_renderer *ir;
    entity **text_selection_entities;
    u32 text_selection_entities_size;
} text_selection;

text_selection *ctor_text_selection(vec2 start_position, vec2 end_position, editor *focused_editor)
{

    text_selection *t=malloc(sizeof(text_selection));
    
    t->text_selection_entities=NULL;
    t->text_selection_entities_size=0;
 
    if(start_position.x==end_position.x && start_position.y == end_position.y)
    {
        return t;
    }
    
    char *base_path=get_base_path();
    char *adjpath=str_cat(base_path,"res/blue.png");
    sdl_free(base_path);
    texture *tex=ctor_texture(editor_get_window(focused_editor), adjpath);
    free(adjpath);
    t->ir=ctor_image_renderer(editor_get_window(focused_editor),tex);
    texture_set_alpha(tex,100);
    
    end_position.x-=1;//we actually dont want to include the character that the cursor is "on"
    /*
    @bug when deleting text lines it seems end_position can be from last frame after the deletion actually happens and then we try to create a text selection ?.
    so you can be accessing an invalid index in the lines array (which happens to be NULL)
    
    so we also have the condition that its less than lines_size.
    */
    for(u32 y=start_position.y; y<=end_position.y && y<editor_get_lines_size(focused_editor); y++)
    {
        if(!editor_get_line(focused_editor,y))continue;//this is redundant and you're accessing an invalid index anyway if you get NULL
        
        char *curline=editor_get_line(focused_editor,y);
        u32 strlen_curline=strlen(curline);
    
        if(y==start_position.y && y==end_position.y)
        {
            /*single line selection*/
            
            u32 w;
            u32 h;
            char *splice=malloc_str_slice(curline, start_position.x, end_position.x);
            size_ttf_font(editor_get_font(focused_editor), splice, &w, &h);
            
            //printf("curline: %s,start: %d,end: %d, splice: %s, size_x: %d, size_y: %d, \n",curline,(int)start_position.x,(int)end_position.x,splice,w,h);
            free(splice);

            u32 w2;
            u32 h2;
            char *splice2=malloc_str_slice(curline, 0, start_position.x-1);
            size_ttf_font(editor_get_font(focused_editor), splice2, &w2, &h2);
            free(splice2);

            t->text_selection_entities_size+=1;
            t->text_selection_entities=realloc(t->text_selection_entities, t->text_selection_entities_size*sizeof(entity*));
            
            entity *e=ctor_entity(editor_get_text_holder(focused_editor));
            entity_set_order(e,0);
            t->text_selection_entities[t->text_selection_entities_size-1]=e;
            
            entity_add_renderer(e,(renderer*)t->ir);
            
            entity_set_size(e,value_vec2(w,h));
            entity_set_position(e,value_vec2(w2,h*y));
        }
        else if(y==start_position.y)
        {
            u32 w;
            u32 h;
            char *splice=malloc_str_slice(curline, start_position.x, strlen_curline-1);
            size_ttf_font(editor_get_font(focused_editor), splice, &w, &h);
            free(splice);
            
            u32 w2;
            u32 h2;
            char *splice2=malloc_str_slice(curline, 0, start_position.x-1);
            size_ttf_font(editor_get_font(focused_editor), splice2, &w2, &h2);
            free(splice2);

            t->text_selection_entities_size+=1;
            t->text_selection_entities=realloc(t->text_selection_entities, t->text_selection_entities_size*sizeof(entity*));
            entity *e=ctor_entity(editor_get_text_holder(focused_editor));
            entity_set_order(e,0);
            t->text_selection_entities[t->text_selection_entities_size-1]=e;
            entity_add_renderer(e,(renderer*)t->ir);
            entity_set_size(e,value_vec2(w,h));

            entity_set_position(e,value_vec2(w2,h*y));
        }
        else if(y==end_position.y)
        {
            u32 w;
            u32 h;
            char *splice=malloc_str_slice(curline, 0, end_position.x);
            size_ttf_font(editor_get_font(focused_editor), splice, &w, &h);
            free(splice);

            t->text_selection_entities_size+=1;
            t->text_selection_entities=realloc(t->text_selection_entities, t->text_selection_entities_size*sizeof(entity*));
            entity *e=ctor_entity(editor_get_text_holder(focused_editor));
            entity_set_order(e,0);
            t->text_selection_entities[t->text_selection_entities_size-1]=e;
            entity_add_renderer(e,(renderer*)t->ir);
            entity_set_size(e,value_vec2(w,h));

            entity_set_position(e,value_vec2(0,h*y));
        }
        else
        {
            u32 w;
            u32 h;
            char *splice=malloc_str_slice(curline, 0, strlen(curline));
            size_ttf_font(editor_get_font(focused_editor), splice, &w, &h);
            free(splice);

            t->text_selection_entities_size+=1;
            t->text_selection_entities=realloc(t->text_selection_entities, t->text_selection_entities_size*sizeof(entity*));
            entity *e=ctor_entity(editor_get_text_holder(focused_editor));
            entity_set_order(e,0);
            t->text_selection_entities[t->text_selection_entities_size-1]=e;
            entity_add_renderer(e,(renderer*)t->ir);
            entity_set_size(e,value_vec2(w,h));

            entity_set_position(e,value_vec2(0,h*y));
        }
    }
    return t;
}
void dtor_text_selection(editor *e)
{    
    text_selection *ts=editor_get_current_text_selection(e);
    if(ts)
    {
        if(ts->text_selection_entities)
        {
            //@leak texture free in ir?
            dtor_image_renderer(ts->ir);
            for(u32 i=0; i<ts->text_selection_entities_size; i++)
            {
                dtor_entity(ts->text_selection_entities[i]);
            }
            
            free(ts->text_selection_entities);
        }
        
        free(ts);
        editor_set_current_text_selection(e, NULL);
    }
}
