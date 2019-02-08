#include "ote/src/h/base.h"

#include <stdlib.h>
#include <string.h>
#define OTE_DEBUG 1

u32 num_spaces_to_insert_on_tab=4;
char ote_log_buffer[1000];
u32 const LOG_TYPE_ACTION=0;
u32 const LOG_TYPE_STRINGS=1;
u32 const LOG_TYPE_OTE=2;
bool log_type_enabled[3]={false,false,false};

s32 const EXIT_CODE_ALL_WINDOWS_CLOSED=-1;

bool const AUTO_CURLY_BRACE=true;
bool const AUTO_INDENT=true;

s32 const LINES_ON_PAGE_DOWNUP=10;

s32 const REQUEST_SPAWN_EDITOR=1;

void ote_log(char *text,u32 type)
{
    #if OTE_DEBUG
    if(log_type_enabled[type])
    {
        printf("\nLOG TYPE: %d\n<BEGIN_LOG>",type);
        printf(text);
        printf("<END_LOG>\n");
    }
    #endif
}

s32 indentation_level_spaces(char *line)
{   
    s32 retval=0;
    s32 i=0;
    s32 strlen_line=strlen(line);
    for(;i<strlen_line; i++)
    {
        if(line[i]!=' ')
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

char const *global_font_url="../res/umr.ttf";
u32 global_font_size=20;
s32 global_text_margin=1;/*if 0 line numbers wont render, can only increase, not decrease*/

s32 standard_button_width=200;
s32 standard_button_height=25;

r32 standard_button_alpha=.5;
r32 standard_button_hover_alpha=.625;
r32 standard_button_selected_alpha=.75;