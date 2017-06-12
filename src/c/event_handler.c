#include "../../../OSAL/src/h/graphics.h"

#include "../h/event_handler.h"
#include "../h/base.h"

global_state *ctor_global_state()
{
    init_graphics();
    init_input();
    
    global_state *gs=malloc(sizeof(global_state));
    gs->e.id=0;

    gs->editors_size=1;
    gs->editors=(editor**)malloc(sizeof(editor*)*gs->editors_size);
    gs->editors[0]=ctor_editor();
    

    gs->focused_editor=gs->editors[0];
    
    gs->mouse_position;
    gs->mouse_position.x=0;
    gs->mouse_position.y=0;    

    
    set_cursor(CURSOR_TEXT);
    return gs;
}

s32 handle_event(global_state *gs)
{
    s32 caps_lock_toggled=get_mod_state() & KEY_MOD_CAPS; 

    if(gs->e.id)
    {/*do window specific operations*/
        if(gs->e.type==MOUSE_MOTION)
        {
            gs->mouse_position.x=gs->e.mouse_info.x;
            gs->mouse_position.y=gs->e.mouse_info.y;
            if(gs->focused_editor->start_selection)
            {
                if(gs->focused_editor->current_text_selection)
                {
                    dtor_text_selection(gs->focused_editor->current_text_selection);        
                }
            
                vec2 new_selection=position_to_cursor(vec2_sub(gs->mouse_position,gs->focused_editor->offset), gs->focused_editor);
                vec2 pos=gs->focused_editor->text_selection_origin;

                vec2 actual_new_selection=new_selection;

                if((new_selection.y<gs->focused_editor->text_selection_origin.y) || (gs->focused_editor->text_selection_origin.y==new_selection.y && gs->focused_editor->text_selection_origin.x > new_selection.x))
                {
                    vec2 temp=new_selection;
                    new_selection=pos;
                    pos=temp;
                    actual_new_selection=temp;
                }

                if(new_selection.x)
                {
                    new_selection.x-=1;
                }
                gs->focused_editor->current_text_selection=ctor_text_selection(pos, new_selection, gs->focused_editor);
                editor_set_cursor_position(gs->focused_editor, actual_new_selection.x, actual_new_selection.y);
            }
        }
        if(gs->e.type==MOUSE_WHEEL)
        {
            gs->focused_editor->offset.y-=gs->e.mouse_info.y*(entity_get_render_size(gs->focused_editor->cursor).y);
            gs->focused_editor->wheel_override=true;
        }
        /*@bug sometimes the cursor changes row but doesnt change column*/
        if(gs->e.type==LEFT_MOUSE && gs->e.pressed)
        {
            if(gs->focused_editor->current_text_selection)
            {
                dtor_text_selection(gs->focused_editor->current_text_selection);
                gs->focused_editor->current_text_selection=NULL;
            }
            vec2 cursor_position=position_to_cursor(vec2_sub(gs->mouse_position,gs->focused_editor->offset),gs->focused_editor);

            editor_set_cursor_position(gs->focused_editor, cursor_position.x, cursor_position.y);

            gs->focused_editor->text_selection_origin=cursor_position;

            gs->focused_editor->start_selection=true;
        }
        if(gs->e.type==LEFT_MOUSE && !gs->e.pressed)
        {
            /*
            if(current_text_selection)
            {
                dtor_text_selection(current_text_selection);
                current_text_selection=NULL;
            }
            */
            gs->focused_editor->start_selection=false;
            gs->focused_editor->text_selection_end=position_to_cursor(vec2_sub(gs->mouse_position,gs->focused_editor->offset),gs->focused_editor);
        }

        if(gs->e.type==WINDOW_CLOSE)
        {
            u32 i;
            for(i=0; i<gs->editors_size; i++)
            {
                if(window_get_id(gs->editors[i]->win)==gs->e.id)
                {
                    editor *editor_to_free=gs->editors[i];
                    u32 i2;
                    for(i2=i; i2<gs->editors_size-1; i2++)
                    {
                        gs->editors[i2]=gs->editors[i2+1];
                    }
                    gs->editors_size-=1;
                    gs->editors=(editor**)realloc(gs->editors,sizeof(editor*)*gs->editors_size);
                    dtor_editor(editor_to_free);
                    break;
                }
            }
            if(gs->editors_size==0)
            {
                return EXIT_CODE_ALL_WINDOWS_CLOSED;
            }
        }       
        if(gs->e.type == FOCUS_GAINED)
        {
            u32 i;
            for(i=0; i<gs->editors_size; i++)
            {
                if(window_get_id(gs->editors[i]->win)==gs->e.id)
                {
                    gs->focused_editor=gs->editors[i];
                    /*@bug mouse doesnt properly register events ending if its not set,
                    but if it is set then it registers events you wouldnt expect...
                    set_mouse_capture_on_currently_focused_window(true);*/

                    break;
                }
            }
            //gs->focused_editor=editors[e.id-1];//@todo do lookup on editors, because id can change and wont just index into array cleanly
        }                
    }

    /*@BUG IF RANDOMLY DONT GET KEYS ITS PROBABLY BECAUSE THIS E.TYPE CHECK*/
    if(gs->focused_editor && gs->e.type)
    {//keystroke to text interpreter
        bool key_is_already_pressed=false;
        u32 i;
        for(i=0; i<gs->focused_editor->keystate.pressed_keys_size; i++)
        {
            if(gs->focused_editor->keystate.pressed_keys[i]==gs->e.type)
            {
                key_is_already_pressed=true;
                break;
            }
        }
        
        bool repeat_key=false;
        if(key_is_already_pressed && gs->e.pressed)
        {
            if(gs->focused_editor->held_key==gs->e.type)
            {
                if(milli_current_time() - gs->focused_editor->held_key_time_stamp > 500)
                {
                    repeat_key=true;
                }
            }
        }

        if(key_is_already_pressed && !gs->e.pressed)
        {
            /*start with index from previous for loop to remove unpressed key*/
            for(;i<gs->focused_editor->keystate.pressed_keys_size-1; i++)
            {
                gs->focused_editor->keystate.pressed_keys[i]=gs->focused_editor->keystate.pressed_keys[i+1];
            }
            gs->focused_editor->keystate.pressed_keys_size-=1;
            gs->focused_editor->keystate.pressed_keys=(s64*)realloc(gs->focused_editor->keystate.pressed_keys,sizeof(s64)*(gs->focused_editor->keystate.pressed_keys_size));
            gs->focused_editor->held_key=0;
        }

        if((!key_is_already_pressed && gs->e.pressed) ||  repeat_key)
        {
            if((!key_is_already_pressed && gs->e.pressed))
            {
                /*@todo add other keys that I don't want to be held down to this condition*/
                if(gs->e.type != KEY_CAPS_LOCK && gs->e.type != KEY_F11)
                {
                    gs->focused_editor->held_key_time_stamp=milli_current_time();
                    gs->focused_editor->held_key=gs->e.type;
                }
                
                gs->focused_editor->keystate.pressed_keys_size+=1;
                gs->focused_editor->keystate.pressed_keys=realloc(gs->focused_editor->keystate.pressed_keys,sizeof(s64)*(gs->focused_editor->keystate.pressed_keys_size));
                gs->focused_editor->keystate.pressed_keys[gs->focused_editor->keystate.pressed_keys_size-1]=gs->e.type;
            }
            
            if(gs->e.type != gs->focused_editor->held_key)
            {
                gs->focused_editor->held_key=0;
            }
                
            gs->focused_editor->keystate.ctrl_pressed=false;
            for(u32 i=0; i<gs->focused_editor->keystate.pressed_keys_size; i++)
            {
                if(gs->focused_editor->keystate.pressed_keys[i]==KEY_LEFT_CONTROL || gs->focused_editor->keystate.pressed_keys[i]==KEY_RIGHT_CONTROL)
                {
                    gs->focused_editor->keystate.ctrl_pressed=true;
                    break;
                }
            }
            /*ACTUAL KEY HANDLING STARTS HERE*/
            if(gs->focused_editor->keystate.ctrl_pressed)
            {
                /*@todo make these rebindable*/
                
                /*open*/
                if(gs->e.type==KEY_O)
                {

                }
                /*save*/
                if(gs->e.type==KEY_S)
                {

                }
                /*find, one for just window if pressing alt for all open windows*/
                if(gs->e.type==KEY_F)
                {

                }
                /*replace, one for just window if pressing alt for all open windows*/
                if(gs->e.type==KEY_H)
                {

                }
                /*move to token*/
                if(gs->e.type==KEY_LEFT)
                {

                }
                /*move to token*/
                if(gs->e.type==KEY_RIGHT)
                {

                }
                /*select all*/
                if(gs->e.type==KEY_A)
                {
                    vec2 begin=value_vec2(0,0);
                    vec2 end=value_vec2(strlen(gs->focused_editor->lines[gs->focused_editor->lines_size-1]),gs->focused_editor->lines_size-1);

                    gs->focused_editor->current_text_selection=ctor_text_selection(begin, end, gs->focused_editor);
                    gs->focused_editor->text_selection_origin=begin;
                    gs->focused_editor->text_selection_end=end;
                }
                /*open new window*/
                if(gs->e.type==KEY_N)
                {
                    gs->editors_size+=1;
                    gs->editors=(editor**)realloc(gs->editors, sizeof(editor*)*gs->editors_size);
                    gs->editors[gs->editors_size-1]=ctor_editor();
                }
                if(gs->e.type==KEY_C)
                {
                    if(gs->focused_editor->current_text_selection)
                    {
                        set_clipboard_text(get_text(gs->focused_editor->text_selection_origin,gs->focused_editor->text_selection_end,gs->focused_editor));
                    }
                }
                if(gs->e.type==KEY_Z)
                {
                    do_action(gs->focused_editor,true);
                }
                if(gs->e.type==KEY_Y)
                {
                    do_action(gs->focused_editor,false);
                }
                if(gs->e.type==KEY_V)
                {
                    char *sdl_clip=get_clipboard_text();
                    char *clip=strcpy(malloc(strlen(sdl_clip)+1),sdl_clip);
                    sdl_free(sdl_clip);
                    clip=str_remove_characters(clip,'\r');
                    if(clip)
                    {
                        add_text(gs->focused_editor,clip,true);
                    }

                }
                if(gs->e.type==KEY_X)
                {
                    if(gs->focused_editor->current_text_selection)
                    {
                        set_clipboard_text(get_text(gs->focused_editor->text_selection_origin,gs->focused_editor->text_selection_end,gs->focused_editor));
                        delete_text(gs->focused_editor->text_selection_origin,gs->focused_editor->text_selection_end, gs->focused_editor, true, ACTION_DELETE);
                        dtor_text_selection(gs->focused_editor->current_text_selection);
                        gs->focused_editor->current_text_selection=NULL;
                        gs->focused_editor->start_selection=false;
                    }
                }
            }
            else
            {
                if(gs->e.type==KEY_DELETE)
                {
                    if(gs->focused_editor->current_text_selection)
                    {
                        delete_text(gs->focused_editor->text_selection_origin, gs->focused_editor->text_selection_end, gs->focused_editor, true, ACTION_DELETE);
                        dtor_text_selection(gs->focused_editor->current_text_selection);
                        gs->focused_editor->current_text_selection=NULL;
                        gs->focused_editor->start_selection=false;
                    }
                    else
                    {
                        vec2 begin;
                        begin.x=gs->focused_editor->cursor_x;
                        begin.y=gs->focused_editor->cursor_y;
                        vec2 end;
                        end.x=gs->focused_editor->cursor_x+1;
                        end.y=gs->focused_editor->cursor_y;

                        
                        if(end.x>strlen(gs->focused_editor->lines[gs->focused_editor->cursor_y]))
                        {                                    
                            if(gs->focused_editor->cursor_y<gs->focused_editor->lines_size-1)
                            {
                                end.x=0;
                                end.y++;    
                                delete_text(begin,end,gs->focused_editor,true, ACTION_DELETE);
                            }
                        }
                        else
                        {
                            delete_text(begin,end,gs->focused_editor,true,ACTION_DELETE);
                        }
                        /*
                        if(gs->focused_editor->cursor_x<strlen(gs->focused_editor->lines[gs->focused_editor->cursor_y]))
                        {
                            char *curline=gs->focused_editor->lines[gs->focused_editor->cursor_y];
                            gs->focused_editor->lines[gs->focused_editor->cursor_y]=str_remove(curline,gs->focused_editor->cursor_x);
                            
                            text_block_renderer_set_text(gs->focused_editor->tbr,gs->focused_editor->lines,gs->focused_editor->lines_size,gs->focused_editor->font_color,NULL);
                        }
                        else if(gs->focused_editor->cursor_y<gs->focused_editor->lines_size-1)
                        {
                            u32 i=0;
                            char *curline=gs->focused_editor->lines[gs->focused_editor->cursor_y];
                            char *nextline=gs->focused_editor->lines[gs->focused_editor->cursor_y+1];
                            u32 strlen_nextline=strlen(nextline);
                            gs->focused_editor->lines[gs->focused_editor->cursor_y]=str_cat(curline,nextline);
                            
                            for(i=gs->focused_editor->cursor_y+1; i<gs->focused_editor->lines_size-1; i++)
                            {
                                gs->focused_editor->lines[i]=gs->focused_editor->lines[i+1];
                            }

                            gs->focused_editor->lines_size-=1;

                            gs->focused_editor->lines=realloc(gs->focused_editor->lines,gs->focused_editor->lines_size*sizeof(char*));
                            
                            text_block_renderer_set_text(gs->focused_editor->tbr,gs->focused_editor->lines,gs->focused_editor->lines_size,gs->focused_editor->font_color,NULL);
                            
                            free(curline);
                            free(nextline);
                        }
                        */
                    }
                }
                if(gs->e.type==KEY_END)
                {
                    
                    if(gs->focused_editor->current_text_selection)
                    {
                        dtor_text_selection(gs->focused_editor->current_text_selection);
                        gs->focused_editor->current_text_selection=NULL;
                        gs->focused_editor->start_selection=false;
                    }
                    
                    char *curline=gs->focused_editor->lines[gs->focused_editor->cursor_y];
                    u32 last_character=0;
                    for(u32 i=gs->focused_editor->cursor_x; i<strlen(curline); i++)
                    {
                        if(curline[i]!=' ')
                        {
                            last_character=i+1;
                        }
                    }   

                    if(!last_character)
                    {
                        last_character=strlen(gs->focused_editor->lines[gs->focused_editor->cursor_y]);
                    }

                    editor_set_cursor_position(gs->focused_editor, last_character, gs->focused_editor->cursor_y);
                }
                if(gs->e.type==KEY_HOME)
                {
                    if(gs->focused_editor->current_text_selection)
                    {
                        dtor_text_selection(gs->focused_editor->current_text_selection);
                        gs->focused_editor->current_text_selection=NULL;
                        gs->focused_editor->start_selection=false;
                    }
                    char *curline=gs->focused_editor->lines[gs->focused_editor->cursor_y];
                    u32 last_character=0;

                    if(strlen(curline))
                    {
                        for(s32 i=gs->focused_editor->cursor_x-1; i>=0; i--)
                        {
                            if(curline[i] != ' ')
                            {
                                last_character=i;
                            }
                        }
                    }

                    editor_set_cursor_position(gs->focused_editor, last_character, gs->focused_editor->cursor_y);
                }
                if(gs->e.type==KEY_TAB)
                {
                    if(gs->focused_editor->current_text_selection)
                    {
                        delete_text(gs->focused_editor->text_selection_origin, gs->focused_editor->text_selection_end,gs->focused_editor,true,ACTION_DELETE);
                        dtor_text_selection(gs->focused_editor->current_text_selection);
                        gs->focused_editor->current_text_selection=NULL;
                        
                        gs->focused_editor->start_selection=false;
                    }
                    char *str=strcpy(malloc(5),"    ");
                    add_text(gs->focused_editor, str,true);
                }
                if(gs->e.type==KEY_F11)
                {
                    gs->focused_editor->is_fullscreen=!gs->focused_editor->is_fullscreen;
                    window_toggle_fullscreen(gs->focused_editor->win,gs->focused_editor->is_fullscreen);
                }
                if(gs->e.type >=KEY_SPACE && gs->e.type <= KEY_Z)
                {
                    if(gs->focused_editor->current_text_selection)
                    {
                        delete_text(gs->focused_editor->text_selection_origin, gs->focused_editor->text_selection_end,gs->focused_editor,true,ACTION_DELETE);
                        dtor_text_selection(gs->focused_editor->current_text_selection);
                        gs->focused_editor->current_text_selection=NULL;
                        gs->focused_editor->start_selection=false;
                    }

                    char *curline=gs->focused_editor->lines[gs->focused_editor->cursor_y];
                    char char_to_insert=gs->e.type;
                    
                    gs->focused_editor->keystate.shift_pressed=false;
                    for(u32 i=0; i<gs->focused_editor->keystate.pressed_keys_size; i++)
                    {
                        if(gs->focused_editor->keystate.pressed_keys[i]==KEY_LEFT_SHIFT || gs->focused_editor->keystate.pressed_keys[i]==KEY_RIGHT_SHIFT)
                        {
                            gs->focused_editor->keystate.shift_pressed=true;
                            break;
                        }
                    }
                    
                    if(gs->focused_editor->keystate.shift_pressed)
                    {
                        char_to_insert=apply_shift(char_to_insert, true);
                    }
                    else if(caps_lock_toggled)
                    {
                        char_to_insert=apply_shift(char_to_insert, false);
                    }

                    char *str=malloc(2);
                    str[0]=char_to_insert;
                    str[1]=0;
                    add_text(gs->focused_editor,str,true); 
                }
                if(gs->e.type==KEY_LEFT_SHIFT||gs->e.type==KEY_RIGHT_SHIFT)
                {
                    /*prolly dont do anything in here since itll get called on heldkey wot
                    also these keys get just checked in the array instead of changing state on press*/
                }
                if(gs->e.type==KEY_LEFT_CONTROL||gs->e.type==KEY_RIGHT_CONTROL)
                {
                    /*prolly dont do anything in here since itll get called on heldkey wot
                    also these keys get just checked in the array instead of changing state on press*/
                }
                if(gs->e.type==KEY_LEFT_ALT||gs->e.type==KEY_RIGHT_ALT)
                {
                    /*prolly dont do anything in here since itll get called on heldkey wot
                    also these keys get just checked in the array instead of changing state on press*/                            
                }
                if(gs->e.type==KEY_ENTER)
                {
                    if(gs->focused_editor->current_text_selection)
                    {
                        delete_text(gs->focused_editor->text_selection_origin, gs->focused_editor->text_selection_end,gs->focused_editor,true,ACTION_DELETE);
                        dtor_text_selection(gs->focused_editor->current_text_selection);
                        gs->focused_editor->current_text_selection=NULL;
                        gs->focused_editor->start_selection=false;
                    }
                    add_text(gs->focused_editor,strcpy(malloc(2),"\n"), true);
                    /*
                    u32 i;
                    char *curline=gs->focused_editor->lines[gs->focused_editor->cursor_y];
                    char *startsplice;
                    char *endsplice;
                    if(strlen(curline))
                    {
                        startsplice=malloc_str_slice(curline,0,gs->focused_editor->cursor_x-1);
                        endsplice=malloc_str_slice(curline,gs->focused_editor->cursor_x,strlen(curline)-1);
                        free(curline);
                    }
                    else
                    {
                        startsplice=malloc(1);
                        startsplice[0]=(char)NULL;
                        
                        endsplice=malloc(1);
                        endsplice[0]=(char)NULL;
                    }
                    
                    gs->focused_editor->lines[gs->focused_editor->cursor_y]=startsplice;
                    
                    gs->focused_editor->lines_size+=1;
                    gs->focused_editor->lines=realloc(gs->focused_editor->lines,gs->focused_editor->lines_size*sizeof(char*));
                    for(i=gs->focused_editor->lines_size-1; i>gs->focused_editor->cursor_y+1; i--)
                    {
                        gs->focused_editor->lines[i]=gs->focused_editor->lines[i-1];
                    }
                    gs->focused_editor->lines[gs->focused_editor->cursor_y+1]=endsplice;
                    text_block_renderer_set_text(gs->focused_editor->tbr,gs->focused_editor->lines,gs->focused_editor->lines_size,gs->focused_editor->font_color,NULL);
                    editor_set_cursor_position(gs->focused_editor,0,gs->focused_editor->cursor_y+1);
                    */                             
                }
                if(gs->e.type==KEY_BACKSPACE)
                {
                    if(gs->focused_editor->current_text_selection)
                    {
                        delete_text(gs->focused_editor->text_selection_origin, gs->focused_editor->text_selection_end,gs->focused_editor,true,ACTION_DELETE);
                        dtor_text_selection(gs->focused_editor->current_text_selection);
                        gs->focused_editor->current_text_selection=NULL;
                        gs->focused_editor->start_selection=false;
                    }
                    else
                    {
                        vec2 begin;
                        begin.x=((s32)gs->focused_editor->cursor_x)-1;
                        begin.y=gs->focused_editor->cursor_y;
                        vec2 end;
                        end.x=gs->focused_editor->cursor_x;
                        end.y=gs->focused_editor->cursor_y;

                          
                        if(begin.x<0)
                        {                           
                            if(gs->focused_editor->cursor_y>0)
                            {
                                begin.x=strlen(gs->focused_editor->lines[gs->focused_editor->cursor_y-1]);
                                begin.y--;    
                                delete_text(begin,end,gs->focused_editor,true,ACTION_DELETE);
                            }
                        }
                        else
                        {
                            delete_text(begin,end,gs->focused_editor,true,ACTION_BACKSPACE);
                        }
                        
                         /*
                        if(gs->focused_editor->cursor_x)
                        {
                            char *curline=gs->focused_editor->lines[gs->focused_editor->cursor_y];
                            gs->focused_editor->cursor_x-=1;
                            gs->focused_editor->lines[gs->focused_editor->cursor_y]=str_remove(curline,gs->focused_editor->cursor_x);
                            
                            text_block_renderer_set_text(gs->focused_editor->tbr,gs->focused_editor->lines,gs->focused_editor->lines_size,gs->focused_editor->font_color,NULL);
                            editor_set_cursor_position(gs->focused_editor,gs->focused_editor->cursor_x,gs->focused_editor->cursor_y);
                        }
                        else if(gs->focused_editor->cursor_y)
                        {
                            u32 i=0;
                            char *curline=gs->focused_editor->lines[gs->focused_editor->cursor_y];
                            char *prevline=gs->focused_editor->lines[gs->focused_editor->cursor_y-1];
                            u32 strlen_prevline=strlen(prevline);
                            gs->focused_editor->lines[gs->focused_editor->cursor_y-1]=str_cat(prevline,curline);
                            
                            for(i=gs->focused_editor->cursor_y; i<gs->focused_editor->lines_size-1; i++)
                            {
                                gs->focused_editor->lines[i]=gs->focused_editor->lines[i+1];
                            }

                            gs->focused_editor->lines_size-=1;

                            gs->focused_editor->lines=realloc(gs->focused_editor->lines,gs->focused_editor->lines_size*sizeof(char*));
                            
                            text_block_renderer_set_text(gs->focused_editor->tbr,gs->focused_editor->lines,gs->focused_editor->lines_size,gs->focused_editor->font_color,NULL);
                            editor_set_cursor_position(gs->focused_editor,strlen_prevline,gs->focused_editor->cursor_y-1);
                            
                            free(curline);
                            free(prevline);
                        }
                        */
                    }
                }
                if(gs->e.type==KEY_CAPS_LOCK)
                {
                    /*we handle this differently now, but might want to do something if they actually click the key someday ? ?*/
                }
                if(gs->e.type==KEY_LEFT)
                {
                    if(gs->focused_editor->current_text_selection)
                    {
                        dtor_text_selection(gs->focused_editor->current_text_selection);
                        gs->focused_editor->current_text_selection=NULL;
                        gs->focused_editor->start_selection=false;
                    }
                    if(gs->focused_editor->cursor_x)
                    {
                        editor_set_cursor_position(gs->focused_editor,gs->focused_editor->cursor_x - 1, gs->focused_editor->cursor_y);
                    }
                    else
                    {
                        if(gs->focused_editor->cursor_y>0)
                        {
                            u32 ypos=gs->focused_editor->cursor_y-1;
                            editor_set_cursor_position(gs->focused_editor,strlen(gs->focused_editor->lines[ypos]), gs->focused_editor->cursor_y-1);
                        }
                    }
                    
                     
                }
                if(gs->e.type==KEY_RIGHT)
                {
                    
                    if(gs->focused_editor->current_text_selection)
                    {
                        dtor_text_selection(gs->focused_editor->current_text_selection);
                        gs->focused_editor->current_text_selection=NULL;
                        gs->focused_editor->start_selection=false;
                    }
                    if(gs->focused_editor->cursor_x < strlen(gs->focused_editor->lines[gs->focused_editor->cursor_y]))
                    {
                        editor_set_cursor_position(gs->focused_editor,gs->focused_editor->cursor_x + 1, gs->focused_editor->cursor_y);
                    }
                    else
                    {
                        if(gs->focused_editor->cursor_y<gs->focused_editor->lines_size-1)
                        {
                            u32 ypos=gs->focused_editor->cursor_y+1;
                            editor_set_cursor_position(gs->focused_editor,0, gs->focused_editor->cursor_y+1);
                        }
                    }

                     
                }
                if(gs->e.type==KEY_UP)
                {
                    
                    if(gs->focused_editor->current_text_selection)
                    {
                        dtor_text_selection(gs->focused_editor->current_text_selection);
                        gs->focused_editor->current_text_selection=NULL;
                        gs->focused_editor->start_selection=false;
                    }
                    if(gs->focused_editor->cursor_y>0)
                    {
                        if(gs->focused_editor->cursor_x>=strlen(gs->focused_editor->lines[gs->focused_editor->cursor_y-1]))
                        {
                            gs->focused_editor->cursor_x=strlen(gs->focused_editor->lines[gs->focused_editor->cursor_y-1]);
                        }
                        editor_set_cursor_position(gs->focused_editor,gs->focused_editor->cursor_x, gs->focused_editor->cursor_y - 1);
                    }
                    
                     
                }
                if(gs->e.type==KEY_DOWN)
                {
                    
                    if(gs->focused_editor->current_text_selection)
                    {
                        dtor_text_selection(gs->focused_editor->current_text_selection);
                        gs->focused_editor->current_text_selection=NULL;
                        gs->focused_editor->start_selection=false;
                    }
                    if(gs->focused_editor->cursor_y<gs->focused_editor->lines_size-1)
                    {
                        
                        if(gs->focused_editor->cursor_x>=strlen(gs->focused_editor->lines[gs->focused_editor->cursor_y+1]))
                        {
                            gs->focused_editor->cursor_x=strlen(gs->focused_editor->lines[gs->focused_editor->cursor_y+1]);
                        }
                        editor_set_cursor_position(gs->focused_editor,gs->focused_editor->cursor_x, gs->focused_editor->cursor_y + 1);
                    }
                    
                     
                }
            }
            
            gs->focused_editor->cursor_blink_state=true;
            entity_set_visible(gs->focused_editor->cursor,gs->focused_editor->cursor_blink_state);
            gs->focused_editor->cursor_blink_timer=0;
        }
    }         
    return 0;  
}
