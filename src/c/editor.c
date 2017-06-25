#include "../h/base.h"
#include "../h/action.h"
#include "../h/editor.h"
#include "../../../OSAL/src/h/util.h"
#include "../../../OSAL/src/h/input.h"

/*
    for find and replace:
    can spawn a top-borderless(OS border) EDITOR small window with modified handle_event with a red outline or something where typing the name searches in its associated real editor
    and hitting enter navigates to the next instance
    hitting escape closes it
    
    for go to line:
    same as previous except hitting enter just navigates to the line in the associated real editor and closes the box and it does nothing while typing.
*/

//@TODO move checks in here, rather than doing them outside also
void editor_set_cursor_position(editor *e, s32 x, s32 y)
{

    //@todo modify current text selection if it exists
    //printf("%d,%d,\n",x,y);
    if(y>=e->lines_size)
    {
        y=e->lines_size-1;
    }
    else if(y<0)
    {
        y=0;
    }
    else if(x>strlen(e->lines[y]))
    {
        x=strlen(e->lines[y]);
    }
    else if(x<0)
    {
        x=0;
    }
    if(e->current_text_selection)
    {
        dtor_text_selection(e);        
        
        vec2 new_selection=value_vec2(x,y);
        vec2 pos=e->text_selection_origin;
        //printf("ENTRY: new_selection: (%d,%d), pos: (%d,%d)\n",(int)new_selection.x,(int)new_selection.y,(int)pos.x,(int)pos.y);
        if((new_selection.y<e->text_selection_origin.y) || (e->text_selection_origin.y==new_selection.y && e->text_selection_origin.x > new_selection.x))
        {
            vec2 temp=new_selection;
            new_selection=pos;
            pos=temp;
        }

        //printf("AFTER: new_selection: (%d,%d), pos: (%d,%d)\n\n",(int)new_selection.x,(int)new_selection.y,(int)pos.x,(int)pos.y);
        e->current_text_selection=ctor_text_selection(pos, new_selection, e);
        
        e->text_selection_end=value_vec2(x,y);
    }            
    
    s32 i;
    e->cursor_x=x;
    e->cursor_y=y;

    u32 width;
    u32 height;
    e->wheel_override=false;
    char *spliced_str=NULL;
    if(x==0)
    {//str_slice returns null or something if the end and begin are the same index, and ttf_size_text has undefined behavior if string is null
        spliced_str=strcpy(malloc(1),"");
    }
    else
    {
        spliced_str=malloc_str_slice(e->lines[y],0,x-1);
    }
    
    size_ttf_font(e->font,spliced_str,&width,&height);
    
    entity_set_position(e->cursor,value_vec2(width+global_text_margin,height*y));

    if(spliced_str)
    {
        free(spliced_str);
    }
}
page_tab *ctor_page_tab(editor *e)
{    
    page_tab *p=malloc(sizeof(page_tab));
    u32 i;
    entity *button_holder=ctor_entity(e->root);
    p->ent=button_holder;
    
    p->file_path=NULL;//might be parameter
    
    entity_set_relsize(button_holder,value_vec2(.2,.05));
    
    entity_set_relposme(button_holder,value_vec2(0,e->page_tabs_size));
    entity_set_position(button_holder,value_vec2(0,5*e->page_tabs_size));
    
    entity_set_order(button_holder,999);
    texture *top_white_texture=ctor_texture(e->win,"res/cursor.png");
    p->tex=top_white_texture;
    texture_set_alpha(top_white_texture,150);
    entity_add_renderer(button_holder,(renderer*)ctor_image_renderer(e->win,top_white_texture));
    
    M_APPEND(e->page_tabs,e->page_tabs_size,p);
    
    return p;
}
void dtor_page_tab(page_tab *p)
{
//@todo
}

editor *ctor_editor()
{
    editor *e=(editor*)malloc(sizeof(editor));
    e->win=ctor_window("Odd Thread Editor", 1000, 1000);
    e->root=ctor_entity(NULL);
    e->text_holder=ctor_entity(e->root);
    entity_set_solid(e->text_holder,false);
    entity_set_relsize(e->text_holder, value_vec2(1,1));
    //entity_set_relpos(e->text_holder, value_vec2(0,0));
    
    window_set_position(e->win,100,100);
    
    e->highest=NULL;
    
    memset(&e->keystate,0,sizeof(e->keystate));

    e->cursor_blink_state=true;
    e->cursor_blink_timer=0;
    e->last_time=milli_current_time();

    e->held_key=0;
    e->held_key_time_stamp=0;

    e->offset=value_vec2(0,0);
    
    e->wheel_override=false;
    e->current_text_selection=NULL;
    e->text_selection_origin.x=0;
    e->text_selection_origin.y=0;
    e->text_selection_end.x=0;
    e->text_selection_end.y=0;

    e->text_entity=ctor_entity(e->text_holder);
    entity_set_solid(e->text_entity,false);
    entity_set_relsize(e->text_entity, value_vec2(1,1));
    entity_set_order(e->text_entity,100);

    e->page_tabs=NULL;
    e->page_tabs_size=0;    
    e->font_color=value_color(0,255,0,255);

    char **c=malloc(sizeof(char*));
    c[0]=strcpy(malloc(strlen("")+1),"");

    e->action_list=NULL;
    e->action_list_size=0;
    e->action_list_index=0;

    e->is_fullscreen=false;//assumes windows start in windowed mode

    e->lines=c;
    e->lines_size=1;

    e->cursor=ctor_entity(e->text_entity);
    entity_set_solid(e->cursor,false);
    
    entity_set_size(e->cursor,value_vec2(2,global_font_size));//@todo size text and set to actual render size, in addition to rendering cursor in code multicolor support etc, have to size text width to move cursor anyway
    entity_add_renderer(e->cursor,(renderer*)ctor_image_renderer(e->win,ctor_texture(e->win,"res/cursor.png")));
    e->cursor_x=0;
    e->cursor_y=0;
    
    e->font=ctor_ttf_font(global_font_url,global_font_size);
    //@todo also change this so i construct a ttf font before hand instead of loading it from url
    text_block_renderer *r=ctor_text_block_renderer(e->win,e->font,false,global_text_margin?&global_text_margin:NULL);
    text_block_renderer_set_text(r,c,e->lines_size,e->font_color,NULL);
    e->tbr=r;
    entity_add_renderer(e->text_entity,(renderer*)r);

    e->start_selection_mouse=false;
    e->start_selection_key=false;
    
    flush_events(MOUSE_EVENTS);
    return e;
}
void dtor_editor(editor *e)
{
    dtor_window(e->win);
    dtor_entity(e->root);/*@todo implement this function and it should free all children*/
    for(u32 i=0; i<e->action_list_size; i++)
    {
        //@bug @leak
        dtor_action(e->action_list[i]);
    }
    free(e->action_list);
    /*@todo @bug @leak how to handle freeing of lines? does closing an editor free the lines? do they have their own copy?*/
    dtor_ttf_font(e->font);
    dtor_text_block_renderer(e->tbr);
    free(e);
}

void update_editor(editor *e_instance)
{
    if(e_instance->cursor_blink_timer>500)
    {
        e_instance->cursor_blink_state=!e_instance->cursor_blink_state;
        entity_set_visible(e_instance->cursor,e_instance->cursor_blink_state);
        e_instance->cursor_blink_timer=0;
    }
    e_instance->cursor_blink_timer+=milli_current_time()-e_instance->last_time;
    e_instance->last_time=milli_current_time();

    int w;
    int h;
    window_get_size(e_instance->win,&w,&h);
    entity_set_size(e_instance->root,value_vec2(w,h));
        
    if(!e_instance->wheel_override)
    {
        vec2 cursorpos=entity_get_render_position(e_instance->cursor);
        vec2 cursorsize=entity_get_render_size(e_instance->cursor);

        if(cursorpos.x+cursorsize.x>w)
        {
            e_instance->offset.x+=w-(cursorpos.x+cursorsize.x);
        }
        if(cursorpos.y+cursorsize.y>h)
        {
            e_instance->offset.y+=h-(cursorpos.y+cursorsize.y);
        }
        if(cursorpos.x<0)
        {
            e_instance->offset.x+=-(cursorpos.x);
        }
        if(cursorpos.y<0)
        {
            e_instance->offset.y+=-(cursorpos.y);
        }
    }
    
    if(e_instance->offset.x > 0)
    {
        e_instance->offset.x=0;
    }
    if(e_instance->offset.y > 0)
    {
        e_instance->offset.y=0;
    }

    entity_set_position(e_instance->text_holder,e_instance->offset);
    
    update_entity_recursive(e_instance->root);
}
void render_editor(editor *e_instance)
{
    clear(e_instance->win);
    render_entity_recursive(e_instance->root);
    flip(e_instance->win);
}

/*
if you pass the end of one line and the beginning of the next line it concats them

*/
void delete_text(vec2 begin, vec2 end, editor *focused_editor, bool do_add_action, u32 type)
{
    if((end.y<begin.y) || (begin.y==end.y && begin.x > end.x))
    {
        vec2 temp=end;
        end=begin;
        begin=temp;
    }
    /*@old bug reallocd array and was using old pointer, because i was holding two pointers to it derp*/
    char *built_string=NULL;

    char *first_line=NULL;
    char *last_line=NULL;
    for(u32 y=begin.y; y<=end.y; y++)
    {
        char *curline=focused_editor->lines[y];
        if(begin.y==end.y)
        {
            char *first=malloc_str_slice(curline, 0, begin.x-1);
            
            char *second=malloc_str_slice(curline, end.x, strlen(curline)-1);

            if(do_add_action)
            {
                built_string=malloc_str_slice(curline,begin.x,end.x-1);
            }

            focused_editor->lines[y]=str_cat(first,second);

            free(first);
            free(second);
            free(curline);
            break;
        }
        else if(y==begin.y)
        {
            first_line=malloc_str_slice(curline,0,begin.x-1);
            if(do_add_action)
            {
                built_string=malloc_str_slice(curline,begin.x,strlen(curline)-1);
            }
        }
        else if(y==end.y)
        {
            u32 i;
            last_line=malloc_str_slice(curline,end.x,strlen(curline)-1);

            if(do_add_action)
            {
                char *temp=built_string;
                built_string=str_cat(built_string,"\n");
                free(temp);
                temp=built_string;
                built_string=str_cat(built_string,malloc_str_slice(curline,0,end.x-1));
                free(temp);
            }

            focused_editor->lines[y]=NULL;
            //free(curline);
        }
        else
        {
            if(do_add_action)
            {
                char *temp=built_string;
                built_string=str_cat(built_string,"\n");
                free(temp);
                temp=built_string;
                built_string=str_cat(built_string,focused_editor->lines[y]);
                free(temp);
            }
            focused_editor->lines[y]=NULL;
            //free(curline);
        }
        
    }
    for(s32 i=0; i<focused_editor->lines_size; i++)
    {
        if(!focused_editor->lines[i])
        {
            for(s32 i2=i; i2<focused_editor->lines_size-1; i2++)
            {
                focused_editor->lines[i2]=focused_editor->lines[i2+1];
            }
            
            i--;

            focused_editor->lines_size-=1;
            focused_editor->lines=realloc(focused_editor->lines,focused_editor->lines_size*sizeof(char*));
        }
    }
    if(first_line && last_line)
    {
        free(focused_editor->lines[(int)begin.y]);
        focused_editor->lines[(int)begin.y]=str_cat(first_line,last_line);
        free(first_line);
        free(last_line);
    }
    
    if(do_add_action)
    {
        add_action(focused_editor,ctor_action(begin, end, built_string, type));
    }
    editor_set_cursor_position(focused_editor, begin.x, begin.y);
    text_block_renderer_set_text(focused_editor->tbr,focused_editor->lines,focused_editor->lines_size,focused_editor->font_color,NULL);
}
/*
@todo change to pass position explicitly
it takes ownership of clip, by passing to action
dont free clip, it is passed to an action which frees it
dont reference clip after passing it
*/
void add_text(editor *focused_editor, char *clip, bool do_add_action)
{
    if(focused_editor->current_text_selection)
    {
        delete_text(focused_editor->text_selection_origin, focused_editor->text_selection_end, focused_editor,false,ACTION_DELETE);
        dtor_text_selection(focused_editor);
        focused_editor->current_text_selection=NULL;
        focused_editor->start_selection_mouse=false;
    }

    u32 clip_index=0;
    u32 strlen_clip=strlen(clip);

    vec2 cursor_position_start;
    cursor_position_start.x=focused_editor->cursor_x;
    cursor_position_start.y=focused_editor->cursor_y;

    while(clip_index<strlen_clip)
    {     
        if(clip[clip_index]=='\n')
        {                                 
            focused_editor->lines_size+=1;
            focused_editor->lines=realloc(focused_editor->lines,focused_editor->lines_size*sizeof(char*));
            for(u32 i=focused_editor->lines_size-1; i>focused_editor->cursor_y+1; i--)
            {
                focused_editor->lines[i]=focused_editor->lines[i-1];
            }
            
            u32 next_line_splice_start=focused_editor->cursor_x;
            u32 next_line_splice_end=strlen(focused_editor->lines[focused_editor->cursor_y])-1;
            focused_editor->lines[focused_editor->cursor_y+1]=malloc_str_slice(focused_editor->lines[focused_editor->cursor_y],next_line_splice_start,next_line_splice_end);
            
            char *temp=focused_editor->lines[focused_editor->cursor_y];
            focused_editor->lines[focused_editor->cursor_y]=malloc_str_slice(temp,0,focused_editor->cursor_x-1);
            free(temp);

            editor_set_cursor_position(focused_editor,0,focused_editor->cursor_y+1);
        }
        else
        {
            char *curline=focused_editor->lines[focused_editor->cursor_y];
            char *modstring=str_insert(curline,clip[clip_index],focused_editor->cursor_x);//@leak?
            focused_editor->lines[focused_editor->cursor_y]=modstring;
            editor_set_cursor_position(focused_editor,focused_editor->cursor_x+1,focused_editor->cursor_y);
        }
        clip_index++;
    }
    if(do_add_action)
    {
        add_action(focused_editor,ctor_action(cursor_position_start, value_vec2(0,0), clip, ACTION_TEXT));
    }
    
    text_block_renderer_set_text(focused_editor->tbr,focused_editor->lines,focused_editor->lines_size,focused_editor->font_color,&focused_editor->cursor_y);
}


vec2 position_to_cursor(vec2 mouse_position, editor *focused_editor)
{
    s32 i;
    s32 w;
    s32 h;
    
    size_ttf_font(focused_editor->font,focused_editor->lines[0],&w,&h);

    s32 cursor_x=0;
    s32 cursor_y=mouse_position.y;
    
    s32 mpos_x=mouse_position.x-global_text_margin;

    cursor_y/=h;

    if(mpos_x<0)
    {
        cursor_x=0;
    }
    else if(cursor_y<focused_editor->lines_size)
    {
        char *curline=focused_editor->lines[cursor_y];

        u32 row_strlen=strlen(curline);
        for(i=0; i<row_strlen; i++)
        {
            char *slice=malloc_str_slice(curline, 0, i);
            size_ttf_font(focused_editor->font,slice,&w,&h);
            free(slice);
            
            if(w>mpos_x)
            {
                if(i)
                {
                    s32 dist2=0;/*distance to the character that passes the mouse*/
                    s32 dist=0;/*distance to the previous character to dist2*/

                    char *slice2=malloc_str_slice(curline,0,i-1);
                    size_ttf_font(focused_editor->font,slice2,&dist,0);
                    free(slice2);

                    dist=dist-mpos_x;
                    dist2=w-mpos_x;
                    
                    if(dist2<0)dist2*=-1;
                    
                    if(dist<0)dist*=-1;

                    if(dist2<dist)
                    {
                        i++;
                    }
                }
                else
                {
                    s32 dist=0-mpos_x;
                    s32 dist2=w-mpos_x;

                    if(dist2<0)dist2*=-1;
                    
                    if(dist<0)dist*=-1;

                    if(dist2<dist)
                    {
                        i++;
                    }
                }       
                break;
            }     
            
        }
        cursor_x=i;
    }
    
    if(cursor_y>=focused_editor->lines_size)
    {
        cursor_y=focused_editor->lines_size-1;
        cursor_x=strlen(focused_editor->lines[cursor_y]);
    }

    return value_vec2(cursor_x,cursor_y);
}
char *get_text(vec2 begin, vec2 end, editor *focused_editor)
{
    if((end.y<begin.y) || (begin.y==end.y && begin.x > end.x))
    {
        vec2 temp=end;
        end=begin;
        begin=temp;
    }
    /*@old bug reallocd array and was using old pointer, because i was holding two pointers to it derp*/

    char *retval=NULL;
    char *temp=NULL;
    for(u32 y=begin.y; y<=end.y; y++)
    {
        char *curline=focused_editor->lines[y];
        u32 strlen_curline=strlen(curline);

        if(begin.y==end.y)
        {
            retval=malloc_str_slice(curline,begin.x,end.x);
            break;
        }
        else if(y==begin.y)
        {
            retval=malloc_str_slice(curline,begin.x,strlen_curline-1);
        }
        else if(y==end.y)
        {
            temp=retval;
            retval=str_cat(retval,"\n");
            free(temp);
            temp=retval;
            retval=str_cat(retval,malloc_str_slice(curline,0,end.x));            
            free(temp);
        }
        else
        {
            temp=retval;
            retval=str_cat(retval,"\n");
            free(temp);
            temp=retval;
            retval=str_cat(retval,curline);
            free(temp);
        }
        
    }
    return retval;
}


void add_action(editor *e, action *a)
{   
    sprintf(ote_log_buffer,"(ENTRY) add_action - e->action_list_size: %d\n",e->action_list_size);
    ote_log(ote_log_buffer,LOG_TYPE_ACTION);
    sprintf(ote_log_buffer,"(ENTRY) add_action - action_list_index: %d\n",e->action_list_index);
    ote_log(ote_log_buffer,LOG_TYPE_ACTION);
    /*@todo delete all actions after the index*/
    if(e->action_list_index < e->action_list_size-1)
    {
        for(u32 i=e->action_list_index+1; i<e->action_list_size; i++)
        {
            if(e->action_list[i])
            {
                dtor_action(e->action_list[i]);
            }
            e->action_list[i]=NULL;
        }
        ote_log("CUTTING OFF HEAD\n",LOG_TYPE_ACTION);

        e->action_list_size=e->action_list_index+1;
        e->action_list_size++;
        e->action_list=realloc(e->action_list, e->action_list_size*sizeof(action*));
        
        e->action_list_index=e->action_list_size-1;
        e->action_list[e->action_list_index]=a;
    }
    else
    {
        e->action_list_size++;
        e->action_list=realloc(e->action_list, e->action_list_size*sizeof(action*));
        e->action_list[e->action_list_size-1]=a;    
        e->action_list_index=e->action_list_size-1;    
    }
    sprintf(ote_log_buffer,"(EXIT) add_action - e->action_list_size: %d\n",e->action_list_size);
    ote_log(ote_log_buffer,LOG_TYPE_ACTION);
    sprintf(ote_log_buffer,"(EXIT) add_action - action_list_index: %d\n",e->action_list_index);
    ote_log(ote_log_buffer,LOG_TYPE_ACTION);
}
void do_action(editor *e, bool un)
{
    sprintf(ote_log_buffer,"do_action - (action_list_index,action_list_size): %d,%d\n",e->action_list_index,e->action_list_size);
    ote_log(ote_log_buffer,LOG_TYPE_ACTION);
    action *a=NULL;
    
    if(un)
    {
        if(!e->action_list_size || e->action_list_index < 0)
        {
            return;
        }
        a = e->action_list[e->action_list_index];
        
        vec2 cursor_pos=e->action_list[e->action_list_index]->cursor_position;
        editor_set_cursor_position(e, cursor_pos.x, cursor_pos.y);

        if(a->type==ACTION_TEXT)
        {
            if(e->action_list_index >=0)
            {
                sprintf(ote_log_buffer,"do_action - undo text: \"%s\"\n",a->text);
                ote_log(ote_log_buffer,LOG_TYPE_ACTION);
                vec2 end;
                u32 newline_count=0;
                u32 i=0;
                u32 strlen_text=strlen(a->text);
                bool found_newline=false;
                u32 last_newline_index=0;

                for(i=0; i<strlen_text; i++)
                {
                    if(a->text[i]=='\n')
                    {
                        found_newline=true;
                        newline_count++;
                        last_newline_index=i;
                    }
                }

                end.x=cursor_pos.x;
                if(found_newline)
                {
                    u32 strlen_lastline=strlen(&a->text[last_newline_index+1]);
                    end.x = strlen_lastline;
                    sprintf(ote_log_buffer,"STRLEN_LASTLINE: %d\n",strlen_lastline);
                    ote_log(ote_log_buffer,LOG_TYPE_ACTION);
                }
                else
                {
                    end.x += strlen(a->text);
                }
                end.y = a->cursor_position.y + newline_count;
                
                sprintf(ote_log_buffer,"do_action - remove text: (%d,%d), (%d,%d)\n",(u32)cursor_pos.x,(u32)cursor_pos.y,(u32)end.x,(u32)end.y);
                ote_log(ote_log_buffer,LOG_TYPE_ACTION);
                
                delete_text(cursor_pos, end, e, false, ACTION_DELETE);

                e->action_list_index--;
            }
        }
        else if(a->type==ACTION_BACKSPACE)
        {
            /*
            for these that use a->text copy the action string - dont want it to get freed in the add_text function
            */
            add_text(e,strcpy(malloc(strlen(a->text)+1),a->text),false);
            e->action_list_index--;
        }
        else if(a->type==ACTION_DELETE)
        {
            add_text(e,strcpy(malloc(strlen(a->text)+1),a->text),false);
            e->action_list_index--;
            editor_set_cursor_position(e, cursor_pos.x, cursor_pos.y);
        }
    }
    else
    {/*redo*/
        if(e->action_list_index+1 > e->action_list_size-1)
        {
            return;
        }

        e->action_list_index++;
        a = e->action_list[e->action_list_index];
        vec2 cursor_pos=e->action_list[e->action_list_index]->cursor_position;
        editor_set_cursor_position(e, cursor_pos.x, cursor_pos.y);
        
        if(a->type==ACTION_TEXT)
        {
            add_text(e, strcpy(malloc(strlen(a->text)+1),a->text), false);
        }
        else if(a->type==ACTION_DELETE)
        {
            delete_text(a->cursor_position, a->optional_end_position, e, false, ACTION_DELETE);
            editor_set_cursor_position(e, cursor_pos.x, cursor_pos.y);
        }
        else if(a->type==ACTION_BACKSPACE)
        {
            delete_text(a->cursor_position, a->optional_end_position, e, false, ACTION_DELETE);
        }
    }
}
