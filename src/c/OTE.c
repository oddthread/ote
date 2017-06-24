#include "../h/base.h"
#include "../h/event_handler.h"

void log_ints(int *x,int sz)
{
    printf("(");
    for(int i=0; i<sz; i++)
    {   
        printf("%d,",x[i]);
    }
    printf(")\n");
}
void test_macros()
{
    u32 i;
    int *test=malloc(6*sizeof(int));
    test[0]=0;
    test[1]=1;
    test[2]=2;
    test[3]=3;
    test[4]=4;
    test[5]=5;

    int sz=6;
    log_ints(test,sz);
    int somenum=5;
    M_INSERT(test,sz,2,somenum);
    log_ints(test,sz);
    M_REMOVE(test,sz,0,,int);
    log_ints(test,sz);
    
    M_APPEND(test,sz,(int)25);
    log_ints(test,sz);
}

int main()
{
    global_state *gs=ctor_global_state();
    
    u32 frame_time_stamp=milli_current_time();
    s32 exit_code=0;

    //flush_events(ALL_EVENTS);

    while(true)
    {
        u32 i;
        while(poll_input(&gs->e))
        {
            exit_code=handle_event(gs);
            if(exit_code)
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
