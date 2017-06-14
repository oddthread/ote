#include "../h/base.h"
#include "../h/event_handler.h"

int main()
{
    global_state *gs=ctor_global_state();
    
    u32 frame_time_stamp=milli_current_time();
    s32 exit_code=0;
    
    while(true)
    {
        u32 i;

        while(poll_input(&gs->e))
        {
            if(exit_code=handle_event(gs))
            {
                goto EXIT_LABEL;
            }
        }

        for(i=0; i<gs->editors_size; i++)
        {
            update_editor(gs->editors[i]);
        }
        for(i=0; i<gs->editors_size; i++)
        {
            render_editor(gs->editors[i]);
        }

        if(milli_current_time()-frame_time_stamp<1000.0f/60.0f)
        {
            sleep_milli(1000.0f/60.0f-(milli_current_time()-frame_time_stamp));
        }
    }
    
    EXIT_LABEL:
    
    sprintf(ote_log_buffer,"exit_code: %d\n",exit_code);
    ote_log(ote_log_buffer,LOG_TYPE_OTE);
    
    return exit_code;
}
