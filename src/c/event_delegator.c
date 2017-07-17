#include "../../../OSAL/src/h/graphics.h"
#include "../../../OSAL/src/h/system.h"

#include "../h/event_delegator.h"
#include "../h/base.h"

#include <stdlib.h>
#include <string.h>
global_state *ctor_global_state(int argc, char **argv)
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

    for(s32 i=1; i<argc; i++)
    {
        ctor_page_tab(gs->focused_editor,argv[i]);
    }

    return gs;
}

s32 delegate_event_window(global_state *gs)
{
    if(gs->e.id)
    {/*do window specific operations*/
        if(editor_get_current_page_tab(gs->focused_editor))
        {
            if(gs->e.type==MOUSE_MOTION)
            {
                gs->mouse_position.x=gs->e.mouse_info.x;
                gs->mouse_position.y=gs->e.mouse_info.y;
                editor_handle_mouse_motion(gs->focused_editor, gs->mouse_position);
            }
            
            if(gs->e.type==MOUSE_WHEEL)
            {
                editor_handle_mouse_wheel(gs->focused_editor,gs->e.mouse_info.y);
            }
            /*@bug sometimes the cursor changes row but doesnt change column*/
            if(gs->e.type==LEFT_MOUSE && gs->e.pressed)
            {
                if(editor_handle_left_mouse_down(gs->focused_editor, gs->mouse_position))
                {
                    return 0;
                }
            }
            if(gs->e.type==LEFT_MOUSE && !gs->e.pressed)
            {
                editor_handle_left_mouse_up(gs->focused_editor);
            }
        }
        else
        {
            set_cursor(CURSOR_NORMAL);
        }         

        if(gs->e.type==WINDOW_CLOSE)
        {
            u32 i;
            for(i=0; i<gs->editors_size; i++)
            {
                if(window_get_id(editor_get_window(gs->editors[i]))==gs->e.id)
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
                if(window_get_id(editor_get_window(gs->editors[i]))==gs->e.id)
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
        if(gs->e.type==DROP_FILE)
        {
            ctor_page_tab(gs->focused_editor,gs->e.str);
            sdl_free(gs->e.str);
        } 
    }
    return 0;
}

s32 delegate_event(global_state *gs)
{
    s32 exit_code;
    
    exit_code=delegate_event_window(gs);
    
    if(!exit_code&&editor_get_current_page_tab(gs->focused_editor))
    {
        /*@BUG IF RANDOMLY DONT GET KEYS ITS PROBABLY BECAUSE THIS E.TYPE CHECK*/
        if(gs->focused_editor && gs->e.type)
        {//keystroke to text interpreter
            if(editor_process_keys(gs->focused_editor, gs->e))
            {
                s32 handle_keys_code=editor_handle_keys(gs->focused_editor, gs->e);
                if(handle_keys_code==REQUEST_SPAWN_EDITOR)
                {
                    M_APPEND(gs->editors,gs->editors_size,ctor_editor());
                }
            }
        }
    }
    return exit_code;  
}
