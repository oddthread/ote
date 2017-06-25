#include "../../../OSAL/src/h/graphics.h"

#include "../h/event_handler.h"
#include "../h/base.h"

#define GSF gs->focused_editor

global_state *ctor_global_state()
{
    init_graphics();
    init_input();
    
    global_state *gs=malloc(sizeof(global_state));
    gs->e.id=0;
    gs->e.pressed=0;

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

s32 indentation_level_spaces(editor *e, s32 line)
{
    if(line<0)
    {
        return 0;
    }
    
    s32 retval=0;
    s32 i=0;
    s32 strlen_line=strlen(e->lines[line]);
    for(;i<strlen_line; i++)
    {
        if(e->lines[line][i]!=' ')
        {
            break;
        }
        else
        {
            retval++;
        }
    }
    return retval;
}

s32 handle_event_window(global_state *gs)
{
    if(gs->e.id)
    {/*do window specific operations*/
        if(gs->e.type==MOUSE_MOTION)
        {
            s32 i;
            bool hit=false;
            set_cursor(CURSOR_TEXT);
            entity *highest=hit_test_recursive(gs->mouse_position,gs->focused_editor->root,gs->focused_editor->root);
            for(i=0; i<gs->focused_editor->page_tabs_size; i++)
            {
                if(highest==gs->focused_editor->page_tabs[i]->ent)
                {
                    texture_set_alpha(gs->focused_editor->page_tabs[i]->tex,255);
                    set_cursor(CURSOR_NORMAL);
                    hit=true;
                }
                else
                {
                    texture_set_alpha(gs->focused_editor->page_tabs[i]->tex,150);
                }
            }
            gs->focused_editor->highest=highest;
    
            gs->mouse_position.x=gs->e.mouse_info.x;
            gs->mouse_position.y=gs->e.mouse_info.y;
            
            if(gs->focused_editor->page_tabs_size)
            {
                if(gs->mouse_position.x<=global_text_margin)
                {
                    for(i=0; i<gs->focused_editor->page_tabs_size; i++)
                    {
                        entity_set_visible(gs->focused_editor->page_tabs[i]->ent,true);
                    }                
                }
                if(gs->mouse_position.x>entity_get_render_size(gs->focused_editor->page_tabs[0]->ent).x)
                {
                    for(i=0; i<gs->focused_editor->page_tabs_size; i++)
                    {
                        entity_set_visible(gs->focused_editor->page_tabs[i]->ent,false);
                    }                
                }
            }
                        
            if(gs->focused_editor->start_selection_mouse)
            {
                vec2 new_selection=position_to_cursor(vec2_sub(gs->mouse_position,gs->focused_editor->offset), gs->focused_editor);
                editor_set_cursor_position(gs->focused_editor, new_selection.x, new_selection.y);
            }
        }
        
        if(gs->focused_editor->highest!=gs->focused_editor->root)
        {
            return 0;
        }
        
        if(gs->e.type==MOUSE_WHEEL)
        {
            gs->focused_editor->offset.y-=gs->e.mouse_info.y*(entity_get_render_size(gs->focused_editor->cursor).y)*5;
            gs->focused_editor->wheel_override=true;
        }
        /*@bug sometimes the cursor changes row but doesnt change column*/
        if(gs->e.type==LEFT_MOUSE && gs->e.pressed)
        {
            if(gs->focused_editor->current_text_selection)
            {
                dtor_text_selection(gs->focused_editor);
            }
            
            /*
                MUST RESET TEXT_SELECTION_ORIGIN AND END BEFORE CALL TO SET_CURSOR_POSITION BECAUSE:
            IT WILL USE THAT TO CTOR THE NEXT TEXT_SELECTION ZZZ
            */
            vec2 cursor_position=position_to_cursor(vec2_sub(gs->mouse_position,gs->focused_editor->offset),gs->focused_editor);
            gs->focused_editor->text_selection_origin=cursor_position;
            gs->focused_editor->text_selection_end=cursor_position;
            
            gs->focused_editor->current_text_selection=ctor_text_selection(gs->mouse_position,gs->mouse_position,gs->focused_editor);
            
            editor_set_cursor_position(gs->focused_editor, cursor_position.x, cursor_position.y);


            gs->focused_editor->start_selection_mouse=true;
            gs->focused_editor->start_selection_key=false;
        }
        if(gs->e.type==LEFT_MOUSE && !gs->e.pressed)
        {
            gs->focused_editor->start_selection_mouse=false;
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
    return 0;
}
bool handle_keys(global_state *gs)
{
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
        
        return true;
    }
    
    return false;
}

#define M_DELETE_SELECTION delete_text(gs->focused_editor->text_selection_origin,gs->focused_editor->text_selection_end, gs->focused_editor, true, ACTION_DELETE);\
dtor_text_selection(gs->focused_editor);\
gs->focused_editor->current_text_selection=NULL;\
gs->focused_editor->start_selection_mouse=false;

s32 handle_event(global_state *gs)
{
    s32 exit_code=handle_event_window(gs);
    
    /*@BUG IF RANDOMLY DONT GET KEYS ITS PROBABLY BECAUSE THIS E.TYPE CHECK*/
    if(gs->focused_editor && gs->e.type)
    {//keystroke to text interpreter
        
        
        if(handle_keys(gs))
        {
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
                        M_DELETE_SELECTION
                    }
                }
            }
            else/*NON CONTROL KEY HANDLING, TEXT SELECTION DELETING KEYS*/
            {
                if(gs->e.type==KEY_DELETE)
                {
                    if(gs->focused_editor->current_text_selection)
                    {
                        M_DELETE_SELECTION
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
                    }
                }
                else if(gs->e.type==KEY_ENTER)
                {
                    if(gs->focused_editor->current_text_selection)
                    {
                        M_DELETE_SELECTION
                    }
                    
                    char *indent_str=strcpy(malloc(2),"\n");
                    if(AUTO_INDENT)
                    {
                        s32 indent_str_size=2;
                        u32 i;
                        s32 x=indentation_level_spaces(gs->focused_editor,gs->focused_editor->cursor_y);
                        for(i=0;i<x;i++)
                        {                            
                            indent_str_size+=1;
                            indent_str=realloc(indent_str,indent_str_size);
                            indent_str[indent_str_size-2]=' ';
                        }
                        
                        indent_str[indent_str_size-1]=0;
                    }                    
                    add_text(gs->focused_editor, indent_str, true);                            
                }
                else if(gs->e.type==KEY_BACKSPACE)
                {
                    if(gs->focused_editor->current_text_selection)
                    {
                        M_DELETE_SELECTION
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
                        
                    }
                }
                else if(gs->e.type >=KEY_SPACE && gs->e.type <= KEY_Z)
                {
                    if(gs->focused_editor->current_text_selection)
                    {
                        M_DELETE_SELECTION
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
                    else if(get_mod_state() & KEY_MOD_CAPS)
                    {
                        char_to_insert=apply_shift(char_to_insert, false);
                    }
                    
                    char *str=malloc(2);
                    str[0]=char_to_insert;
                    str[1]=0;
                    add_text(gs->focused_editor,str,true);
                    
                    if(AUTO_CURLY_BRACE)
                    {
                        if(char_to_insert=='{')
                        {
                            
                            if(AUTO_INDENT)
                            {
                                s32 indent_str_size=1;
                                char *indent_str=malloc(indent_str_size);
                                indent_str[0]='\n';
                                
                                
                                u32 i;
                                s32 indent_level=indentation_level_spaces(gs->focused_editor,gs->focused_editor->cursor_y);
                                s32 x=num_spaces_to_insert_on_tab+indent_level;
                                for(i=0;i<x;i++)
                                {                            
                                    indent_str_size+=1;
                                    indent_str=realloc(indent_str,indent_str_size);
                                    indent_str[indent_str_size-1]=' ';
                                }
                                
                                indent_str_size+=1;
                                indent_str=realloc(indent_str,indent_str_size);                            
                                indent_str[indent_str_size-1]='\n';
                                
                                for(i=0;i<indent_level;i++)
                                {                            
                                    indent_str_size+=1;
                                    indent_str=realloc(indent_str,indent_str_size);
                                    indent_str[indent_str_size-1]=' ';
                                }
                                
                                indent_str_size+=2;
                                indent_str=realloc(indent_str,indent_str_size);                            
                                
                                indent_str[indent_str_size-2]='}';
                                indent_str[indent_str_size-1]=0;
                                
                                add_text(gs->focused_editor,indent_str,true);
                                editor_set_cursor_position(GSF, strlen(GSF->lines[GSF->cursor_y-1]), GSF->cursor_y-1);
                            }
                            else
                            {
                                char *str=strcpy(malloc(2),"}");
                                add_text(gs->focused_editor,str,true);
                                editor_set_cursor_position(GSF, GSF->cursor_x-1, GSF->cursor_y);
                            }
                        }
                    }
                }
                else /*TEXT_SELECTION CLOSING KEYS IF SHIFT ISNT PRESSED*/
                {                    
                    #define M_SHIFT_CHECK if(gs->focused_editor->current_text_selection && !(get_mod_state() & KEY_MOD_SHIFT))\
                    {\
                        dtor_text_selection(gs->focused_editor);\
                        gs->focused_editor->current_text_selection=NULL;\
                        gs->focused_editor->start_selection_mouse=false;\
                        gs->focused_editor->start_selection_key=false;\
                    }
                    
                    if(!gs->focused_editor->current_text_selection && (get_mod_state() & KEY_MOD_SHIFT))
                    {
                        if(gs->focused_editor->current_text_selection)
                        {
                            dtor_text_selection(gs->focused_editor);
                        }
                                   
                        vec2 cpos=value_vec2(gs->focused_editor->cursor_x,gs->focused_editor->cursor_y);
                        gs->focused_editor->text_selection_origin=cpos;
                        gs->focused_editor->text_selection_end=cpos;             
                        gs->focused_editor->current_text_selection=ctor_text_selection(cpos,cpos,gs->focused_editor);
                        
                        gs->focused_editor->start_selection_mouse=false;
                        gs->focused_editor->start_selection_key=true;
                    }
                    
                    if(gs->e.type==KEY_END)
                    {
                        M_SHIFT_CHECK
                        
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
                    else if(gs->e.type==KEY_HOME)
                    {
                        M_SHIFT_CHECK
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
                    else if(gs->e.type==KEY_PAGE_DOWN)
                    {      
                        M_SHIFT_CHECK                  
                        editor_set_cursor_position(gs->focused_editor, gs->focused_editor->cursor_x, gs->focused_editor->cursor_y+LINES_ON_PAGE_DOWNUP);
                    }
                    else if(gs->e.type==KEY_PAGE_UP)
                    {
                        M_SHIFT_CHECK
                        editor_set_cursor_position(gs->focused_editor, gs->focused_editor->cursor_x, gs->focused_editor->cursor_y-LINES_ON_PAGE_DOWNUP);
                    }                
                    else if(gs->e.type==KEY_TAB)
                    {
                        M_SHIFT_CHECK
                        if(gs->focused_editor->current_text_selection)
                        {
                            //if you want to keep the text selected, will have to create a new text selection
                            //basically just increase all values by 4 or whatever

                            //@TODO
                        }
                        else
                        {
                            char *str=strcpy(malloc(5),"    ");
                            add_text(gs->focused_editor, str,true);
                        }
                    }
                    else if(gs->e.type==KEY_F11)
                    {
                        M_SHIFT_CHECK
                        gs->focused_editor->is_fullscreen=!gs->focused_editor->is_fullscreen;
                        window_toggle_fullscreen(gs->focused_editor->win,gs->focused_editor->is_fullscreen);
                    }
                    else if(gs->e.type==KEY_LEFT)
                    {
                        M_SHIFT_CHECK
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
                    else if(gs->e.type==KEY_RIGHT)
                    {
                        M_SHIFT_CHECK
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
                    else if(gs->e.type==KEY_UP)
                    {
                        M_SHIFT_CHECK
                        if(gs->focused_editor->cursor_y>0)
                        {
                            if(gs->focused_editor->cursor_x>=strlen(gs->focused_editor->lines[gs->focused_editor->cursor_y-1]))
                            {
                                gs->focused_editor->cursor_x=strlen(gs->focused_editor->lines[gs->focused_editor->cursor_y-1]);
                            }
                            editor_set_cursor_position(gs->focused_editor,gs->focused_editor->cursor_x, gs->focused_editor->cursor_y - 1);
                        }
                        
                         
                    }
                    else if(gs->e.type==KEY_DOWN)
                    {
                        M_SHIFT_CHECK
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
            }
            
            gs->focused_editor->cursor_blink_state=true;
            entity_set_visible(gs->focused_editor->cursor,gs->focused_editor->cursor_blink_state);
            gs->focused_editor->cursor_blink_timer=0;
        }
    }         
    return exit_code;  
}
