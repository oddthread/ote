#include "../h/base.h"
#include "../h/event_delegator.h"

#include "opl/src/h/system.h"

#include "oul/src/h/oul.h"

#include "ovp/src/h/ovp.h"

#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    create_empty_file_if_not_exists("colors.var");
    ovp *config=ctor_ovp(alloc_file_to_str("colors.var"));

    global_state *gs=ctor_global_state(argc, argv, config);

    u32 frame_time_stamp=milli_current_time();
    s32 exit_code=0;

    //flush_events(ALL_EVENTS);

    while(true)
    {
        u32 i;
        while(poll_input(&gs->e))
        {
            exit_code=delegate_event(gs);
            if(exit_code)
            {
                goto EXIT_LABEL;
            }
        }
        
        for(i=0; i<gs->editors_size; i++)
        {
            editor_update(gs->editors[i]);
        }
        for(i=0; i<gs->editors_size; i++)
        {
            editor_draw(gs->editors[i]);
        }

        if(milli_current_time()-frame_time_stamp<1000.0f/60.0f)
        {
            s32 sleep_time=1000.0f/60.0f-(milli_current_time()-frame_time_stamp);
            if(sleep_time>0)
            {
                sleep_milli(sleep_time);
            }
        }
    }
    
    EXIT_LABEL:
    
    sprintf(ote_log_buffer,"exit_code: %d\n",exit_code);
    ote_log(ote_log_buffer,LOG_TYPE_OTE);
    
    return exit_code;
}
