#include "../h/base.h"

#define OTE_DEBUG 1

u32 num_spaces_to_insert_on_tab=4;
char ote_log_buffer[100];
u32 const LOG_TYPE_ACTION=0;
u32 const LOG_TYPE_STRINGS=1;
u32 const LOG_TYPE_OTE=2;
bool log_type_enabled[3]={false,false,false};

s32 const EXIT_CODE_ALL_WINDOWS_CLOSED=-1;

bool const AUTO_CURLY_BRACE=true;
bool const AUTO_INDENT=true;

s32 const LINES_ON_PAGE_DOWNUP=10;


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

char const *global_font_url="res/umr.ttf";
u32 global_font_size=54;
s32 global_text_margin=0;/*if 0 line numbers wont render*/
