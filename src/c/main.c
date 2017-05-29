#include "../../../OTG/src/h/OTG.h"
#include "../../../OSAL/src/h/input.h"
#include "../../../OSAL/src/h/graphics.h"
#include "../../../OSAL/src/h/system.h"
#include "../../../OSAL/src/h/util.h"
#include <stdlib.h>
#include <string.h>

/*@bugs
cant select just a single character

sometimes cursor is unresponsive after opening app

sometimes enter will skip the first keypress

text selection stops working when mouse goes outside of window, doesnt get left key up event? maybe just do mouseup event when cursor leaves window

if you select a line from the 0th character and go upwards with the selection it highlights that space when it shouldnt?
*/

/*@todo actually check off this shit
sort out cursor positioning on undo/redo, especially for deleting selections

auto indent and braces completion, insert second newline and indent after enter on brace ({[]})

shift makes you highlight whenever you move the cursor in any way (shift down should assign to focused_editor->current_text_selection)

other ctrl shortcut - undo,redo - search and replace should (optionally?) be able to work on all windows
undo redo, whenever you insert text into the document push the action into some array and set index to top of array, when undo do
opposite action of array[index] and decrement index, when pressing redo do action at array[index] and increment index

ui and files (js port?) - if the ui is changed need to check the mouse position and change the system_cursor (hittest)

ctrl arrows, ctrl shift arrows for selection (shared code with mouse selection and parsing, it goes to tokens)
syntax highlighting & auto completion

tbr - if line numbers render at greater than margin size clip them

optimize paste and line insertion/removal (im sure rendering is the bottleneck)

update root entity on window resize for each window instead of every frame

inside draw function check if in bounds before drawing

convert empty parenthesis to (void)

turn on Wall, get compiling on linux, general testing

? use my own fullscreen instead of sdl so it doesnt minimize when I click away

? change to use sdl_textinputevent? it might be wonky, its a pretty big abstraction

*done* fix camera so if you are below the window you can still move the cursor up, same with left/right
*done* mouse scroll
*done* ctrl key disabled regular key entry because it is used for ctrl commands e.g. ctrl c (USE CLIPBOARD)
*done* move camera on cursor pose
*done* make camera move when selection and going beyond end of screen (so it functions like vscode)
*done* mouse place cursor and selection
*done* home, delete, end
*done* arbitrary number of windows, handle opening and closing and removing renderers to those windows
*done* end process once all windows are closed

@perf
dirty
sort renderers by renderer type
profile to find bottlenecks
optimize osal functions

@notes

move all editor-stateful variables from main to editor

designed to only have one font rendering in an editor

*/

/*??? future project after writing parser generator and C parser, .otc files that get compiled to .c (c code that is as portable as possible)

write tool to:

templates on structs and functions using 
"@template type
struct 
{
    type varname;
}"
notation (@ sign will be a signal to identify OT preprocessor code)

can mark public on structs (they will be in header file), they are private by default (defined in c file)

generate header files, also copies comments to header file, possibly can also autogenerate documentation

generate getters/setters

prefix all functions in system with system name (or namespaces)

methods on structs, always static (unless doing inheritance also then theres some crazy magic and shit)
can declare method for a struct even when its not defined (takes a pointer and it is defined elsewhere)

move variables declarations to top of scope and initialize them after (if they are initialized),
cast all void*,
other changes to make the code compatible with as many C/C++ compilers as possible

remove necessity to type typedef on structs (add the typdefs)
 
format code

eventually make it a general linter (detect buffer overflows, integer overflows, etc.)

possibly anonymous functions and structs (this would be tricky.. might not be worth it anyway, and then you need auto?)

possibly inheritance

possibly overloading

possibly custom operators
*/

typedef struct editor editor;
void delete_text(vec2 begin, vec2 end, editor *focused_editor, bool do_add_action, u32 type);
void add_text(editor *focused_editor, char *clip, bool do_add_action);

char const *global_font_url="res/UbuntuMono-R.ttf";
u32 global_font_size=24;
s32 global_text_margin=0;/*if 0 line numbers wont render*/

typedef struct text_selection
{
    image_renderer *ir;
    entity **text_selection_entities;
    u32 text_selection_entities_size;
} text_selection;

typedef struct keystate_interpreter_info
{
    bool shift_pressed;
    bool alt_pressed;//?
    bool ctrl_pressed;
    s64 *pressed_keys;
    u32 pressed_keys_size;
} keystate_interpreter_info;
typedef struct action
{
    vec2 cursor_position;
    vec2 optional_end_position;
    enum {ACTION_TEXT,ACTION_BACKSPACE,ACTION_DELETE} type;
    char *text;
} action;
action *ctor_action(vec2 cursor_position, vec2 optional_end_position, char *text, u32 type)
{
    action *a=malloc(sizeof(action));
    a->cursor_position=cursor_position;
    a->optional_end_position=optional_end_position;
    a->text=text;
    a->type=type;
    return a;
}
void dtor_action(action *a)
{
    free(a->text);
    free(a);
}

typedef struct editor
{
    bool start_selection;
    bool wheel_override;

    action **action_list;
    s32 action_list_size;
    s32 action_list_index;

    text_selection *current_text_selection;
    vec2 text_selection_origin;
    vec2 text_selection_end;

    color font_color;
    bool is_fullscreen;
    char **lines;
    u32 lines_size;
    text_block_renderer *tbr;
    window *win;
    entity *root;
    entity *text_holder;
    entity *text_entity;
    ttf_font *font;

    entity *focused_entity;
    
    s32 cursor_x;
    s32 cursor_y;
    entity *cursor;
} editor;
void editor_set_cursor_position(editor *e, u32 x, u32 y)
{
    u32 i;
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
editor *ctor_editor()
{
    editor *e=(editor*)malloc(sizeof(editor));
    e->win=ctor_window("Odd Thread Editor", 1000, 1000);
    e->root=ctor_entity(NULL);
    e->text_holder=ctor_entity(e->root);
    entity_set_relsize(e->text_holder, value_vec2(1,1));

    e->wheel_override=false;
    e->start_selection=false;
    e->current_text_selection=NULL;
    e->text_selection_origin.x=0;
    e->text_selection_origin.y=0;
    e->text_selection_end.x=0;
    e->text_selection_end.y=0;

    e->text_entity=ctor_entity(e->text_holder);
    entity_set_relsize(e->text_entity, value_vec2(1,1));
    entity_set_order(e->text_entity,100);

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

    flush_events(MOUSE_EVENTS);
    return e;
}
void dtor_editor(editor *e)
{
    dtor_window(e->win);
    dtor_entity(e->root);/*@todo implement this function and it should free all children*/
    for(u32 i=0; i<e->action_list_size; i++)
    {
        dtor_action(e->action_list[i]);
    }
    free(e->action_list);
    /*@todo @bug @leak how to handle freeing of lines? does closing an editor free the lines? do they have their own copy?*/
    dtor_ttf_font(e->font);
    dtor_text_block_renderer(e->tbr);
    free(e);
}

text_selection *ctor_text_selection(vec2 start_position, vec2 end_position, editor *focused_editor)
{

    text_selection *t=malloc(sizeof(text_selection));
    texture *tex=ctor_texture(focused_editor->win, "res/blue.png");
    t->ir=ctor_image_renderer(focused_editor->win,tex);
    texture_set_alpha(tex,100);
    t->text_selection_entities=NULL;
    t->text_selection_entities_size=0;

    /*
    if(start_position.x==end_position.x && start_position.y == end_position.y)
    {
        return;
    }
    */

    for(u32 y=start_position.y; y<=end_position.y; y++)
    {
        char *curline=focused_editor->lines[y];
        u32 strlen_curline=strlen(curline);

        if(y==start_position.y && y==end_position.y)
        {
            /*single line selection*/
            
            u32 w;
            u32 h;
            char *splice=malloc_str_slice(curline, start_position.x, end_position.x);
            size_ttf_font(focused_editor->font, splice, &w, &h);
            free(splice);

            u32 w2;
            u32 h2;
            char *splice2=malloc_str_slice(curline, 0, start_position.x-1);
            size_ttf_font(focused_editor->font, splice2, &w2, &h2);
            free(splice2);

            t->text_selection_entities_size+=1;
            t->text_selection_entities=realloc(t->text_selection_entities, t->text_selection_entities_size*sizeof(entity*));
            entity *e=ctor_entity(focused_editor->text_holder);
            entity_set_order(e,0);
            t->text_selection_entities[t->text_selection_entities_size-1]=e;
            entity_add_renderer(e,(renderer*)t->ir);
            
            entity_set_size(e,value_vec2(w,h));

            entity_set_position(e,value_vec2(w2+global_text_margin,h*y));
        }
        else if(y==start_position.y)
        {
            u32 w;
            u32 h;
            char *splice=malloc_str_slice(curline, start_position.x, strlen_curline-1);
            size_ttf_font(focused_editor->font, splice, &w, &h);
            free(splice);
            
            u32 w2;
            u32 h2;
            char *splice2=malloc_str_slice(curline, 0, start_position.x-1);
            size_ttf_font(focused_editor->font, splice2, &w2, &h2);
            free(splice2);

            t->text_selection_entities_size+=1;
            t->text_selection_entities=realloc(t->text_selection_entities, t->text_selection_entities_size*sizeof(entity*));
            entity *e=ctor_entity(focused_editor->text_holder);
            entity_set_order(e,0);
            t->text_selection_entities[t->text_selection_entities_size-1]=e;
            entity_add_renderer(e,(renderer*)t->ir);
            entity_set_size(e,value_vec2(w,h));

            entity_set_position(e,value_vec2(w2+global_text_margin,h*y));
        }
        else if(y==end_position.y)
        {
            u32 w;
            u32 h;
            char *splice=malloc_str_slice(curline, 0, end_position.x);
            size_ttf_font(focused_editor->font, splice, &w, &h);
            free(splice);

            t->text_selection_entities_size+=1;
            t->text_selection_entities=realloc(t->text_selection_entities, t->text_selection_entities_size*sizeof(entity*));
            entity *e=ctor_entity(focused_editor->text_holder);
            entity_set_order(e,0);
            t->text_selection_entities[t->text_selection_entities_size-1]=e;
            entity_add_renderer(e,(renderer*)t->ir);
            entity_set_size(e,value_vec2(w,h));

            entity_set_position(e,value_vec2(0+global_text_margin,h*y));
        }
        else
        {
            u32 w;
            u32 h;
            char *splice=malloc_str_slice(curline, 0, strlen(curline));
            size_ttf_font(focused_editor->font, splice, &w, &h);
            free(splice);

            t->text_selection_entities_size+=1;
            t->text_selection_entities=realloc(t->text_selection_entities, t->text_selection_entities_size*sizeof(entity*));
            entity *e=ctor_entity(focused_editor->text_holder);
            entity_set_order(e,0);
            t->text_selection_entities[t->text_selection_entities_size-1]=e;
            entity_add_renderer(e,(renderer*)t->ir);
            entity_set_size(e,value_vec2(w,h));

            entity_set_position(e,value_vec2(0+global_text_margin,h*y));
        }
    }
    return t;
}
void dtor_text_selection(text_selection *s)
{
    dtor_image_renderer(s->ir);
    for(u32 i=0; i<s->text_selection_entities_size; i++)
    {
        dtor_entity(s->text_selection_entities[i]);
    }
    free(s->text_selection_entities);
}

void add_action(editor *e, action *a)
{   
    printf("(ENTRY) add_action - e->action_list_size: %d\n",e->action_list_size);
    printf("(ENTRY) add_action - action_list_index: %d\n",e->action_list_index);
    /*@todo delete all actions after the index*/
    if(e->action_list_index < e->action_list_size-1)
    {
        for(u32 i=e->action_list_index+1; i<e->action_list_size; i++)
        {
            dtor_action(e->action_list[i]);
            e->action_list[i]=NULL;
        }
        printf("CUTTING OFF HEAD\n");

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
    printf("(EXIT) add_action - e->action_list_size: %d\n",e->action_list_size);
    printf("(EXIT) add_action - action_list_index: %d\n",e->action_list_index);
}
void do_action(editor *e, bool un)
{
    printf("do_action - (action_list_index,action_list_size): %d,%d\n",e->action_list_index,e->action_list_size);
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
                printf("do_action - undo text: \"%s\"\n",a->text);
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
                    printf("STRLEN_LASTLINE: %d\n",strlen_lastline);
                }
                else
                {
                    end.x += strlen(a->text);
                }
                end.y = a->cursor_position.y + newline_count;
                
                printf("do_action - remove text: (%d,%d), (%d,%d)\n",(u32)cursor_pos.x,(u32)cursor_pos.y,(u32)end.x,(u32)end.y);
                delete_text(cursor_pos, end, e, false, ACTION_DELETE);

                e->action_list_index--;
            }
        }
        else if(a->type==ACTION_BACKSPACE)
        {
            add_text(e,a->text,false);
            e->action_list_index--;
        }
        else if(a->type==ACTION_DELETE)
        {
            add_text(e,a->text,false);
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
            add_text(e, a->text, false);
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
        printf("%s\n",built_string);
        add_action(focused_editor,ctor_action(begin, end, built_string, type));
    }
    editor_set_cursor_position(focused_editor, begin.x, begin.y);
    text_block_renderer_set_text(focused_editor->tbr,focused_editor->lines,focused_editor->lines_size,focused_editor->font_color,NULL);
}
/*
@todo change to pass position explicitly
dont free clip, it is passed to an action which frees it
*/
void add_text(editor *focused_editor, char *clip, bool do_add_action)
{
    if(focused_editor->current_text_selection)
    {
        delete_text(focused_editor->text_selection_origin, focused_editor->text_selection_end, focused_editor,false,ACTION_DELETE);
        dtor_text_selection(focused_editor->current_text_selection);
        focused_editor->current_text_selection=NULL;
        focused_editor->start_selection=false;
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
            char *modstring=str_insert(curline,clip[clip_index],focused_editor->cursor_x);
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




int main()
{
    u32 num_spaces_to_insert_on_tab=4;
    event e;
    editor **editors;
    u32 editors_size;
    editor *focused_editor;

    e.id=0;

    init_graphics();
    init_input();

    editors_size=1;
    editors=(editor**)malloc(sizeof(editor*)*editors_size);
    editors[0]=ctor_editor();
    

    focused_editor=editors[0];
    keystate_interpreter_info keystate;
    memset(&keystate,0,sizeof(keystate));

    bool cursor_blink_state=true;
    s64 cursor_blink_timer=0;
    s64 last_time=milli_current_time();

    s64 held_key=0;
    s64 held_key_time_stamp=0;

    vec2 mouse_position;
    mouse_position.x=0;
    mouse_position.y=0;

    set_cursor(CURSOR_TEXT);
    

    vec2 offset=value_vec2(0,0);
    u32 frame_time_stamp=milli_current_time();

    while(true)
    {
        u32 i;

        while(poll_input(&e))//@todo change to while
        {
            s32 caps_lock_toggled=get_mod_state() & KEY_MOD_CAPS; 

            if(e.id)
            {/*do window specific operations*/
                if(e.type==MOUSE_MOTION)
                {
                    mouse_position.x=e.mouse_info.x;
                    mouse_position.y=e.mouse_info.y;
                    if(focused_editor->start_selection)
                    {
                        if(focused_editor->current_text_selection)
                        {
                            dtor_text_selection(focused_editor->current_text_selection);        
                        }
                    
                        vec2 new_selection=position_to_cursor(vec2_sub(mouse_position,offset), focused_editor);
                        vec2 pos=focused_editor->text_selection_origin;

                        vec2 actual_new_selection=new_selection;

                        if((new_selection.y<focused_editor->text_selection_origin.y) || (focused_editor->text_selection_origin.y==new_selection.y && focused_editor->text_selection_origin.x > new_selection.x))
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
                        focused_editor->current_text_selection=ctor_text_selection(pos, new_selection, focused_editor);
                        editor_set_cursor_position(focused_editor, actual_new_selection.x, actual_new_selection.y);
                    }
                }
                if(e.type==MOUSE_WHEEL)
                {
                    offset.y-=e.mouse_info.y*(entity_get_render_size(focused_editor->cursor).y);
                    focused_editor->wheel_override=true;
                }
                /*@bug sometimes the cursor changes row but doesnt change column*/
                if(e.type==LEFT_MOUSE && e.pressed)
                {
                    if(focused_editor->current_text_selection)
                    {
                        dtor_text_selection(focused_editor->current_text_selection);
                        focused_editor->current_text_selection=NULL;
                    }
                    vec2 cursor_position=position_to_cursor(vec2_sub(mouse_position,offset),focused_editor);

                    editor_set_cursor_position(focused_editor, cursor_position.x, cursor_position.y);

                    focused_editor->text_selection_origin=cursor_position;

                    focused_editor->start_selection=true;
                }
                if(e.type==LEFT_MOUSE && !e.pressed)
                {
                    /*
                    if(current_text_selection)
                    {
                        dtor_text_selection(current_text_selection);
                        current_text_selection=NULL;
                    }
                    */
                    focused_editor->start_selection=false;
                    focused_editor->text_selection_end=position_to_cursor(vec2_sub(mouse_position,offset),focused_editor);
                }

                if(e.type==WINDOW_CLOSE)
                {
                    u32 i;
                    for(i=0; i<editors_size; i++)
                    {
                        if(window_get_id(editors[i]->win)==e.id)
                        {
                            editor *editor_to_free=editors[i];
                            u32 i2;
                            for(i2=i; i2<editors_size-1; i2++)
                            {
                                editors[i2]=editors[i2+1];
                            }
                            editors_size-=1;
                            editors=(editor**)realloc(editors,sizeof(editor*)*editors_size);
                            dtor_editor(editor_to_free);
                            break;
                        }
                    }
                    if(editors_size==0)
                    {
                        exit(0);
                    }
                }       
                if(e.type == FOCUS_GAINED)
                {
                    u32 i;
                    for(i=0; i<editors_size; i++)
                    {
                        if(window_get_id(editors[i]->win)==e.id)
                        {
                            focused_editor=editors[i];
                            /*@bug mouse doesnt properly register events ending if its not set,
                            but if it is set then it registers events you wouldnt expect...
                            set_mouse_capture_on_currently_focused_window(true);*/

                            break;
                        }
                    }
                    //focused_editor=editors[e.id-1];//@todo do lookup on editors, because id can change and wont just index into array cleanly
                }                
            }

            /*@BUG IF RANDOMLY DONT GET KEYS ITS PROBABLY BECAUSE THIS E.TYPE CHECK*/
            if(focused_editor && e.type)
            {//keystroke to text interpreter
                bool key_is_already_pressed=false;
                u32 i;
                for(i=0; i<keystate.pressed_keys_size; i++)
                {
                    if(keystate.pressed_keys[i]==e.type)
                    {
                        key_is_already_pressed=true;
                        break;
                    }
                }
                
                bool repeat_key=false;
                if(key_is_already_pressed && e.pressed)
                {
                    if(held_key==e.type)
                    {
                        if(milli_current_time() - held_key_time_stamp > 500)
                        {
                            repeat_key=true;
                        }
                    }
                }

                if(key_is_already_pressed && !e.pressed)
                {
                    /*start with index from previous for loop to remove unpressed key*/
                    for(;i<keystate.pressed_keys_size-1; i++)
                    {
                        keystate.pressed_keys[i]=keystate.pressed_keys[i+1];
                    }
                    keystate.pressed_keys_size-=1;
                    keystate.pressed_keys=(s64*)realloc(keystate.pressed_keys,sizeof(s64)*(keystate.pressed_keys_size));
                    held_key=0;
                }

                if((!key_is_already_pressed && e.pressed) ||  repeat_key)
                {
                    if((!key_is_already_pressed && e.pressed))
                    {
                        /*@todo add other keys that I don't want to be held down to this condition*/
                        if(e.type != KEY_CAPS_LOCK && e.type != KEY_F11)
                        {
                            held_key_time_stamp=milli_current_time();
                            held_key=e.type;
                        }
                        
                        keystate.pressed_keys_size+=1;
                        keystate.pressed_keys=realloc(keystate.pressed_keys,sizeof(s64)*(keystate.pressed_keys_size));
                        keystate.pressed_keys[keystate.pressed_keys_size-1]=e.type;
                    }
                    
                    if(e.type != held_key)
                    {
                        held_key=0;
                    }
                        
                    keystate.ctrl_pressed=false;
                    for(u32 i=0; i<keystate.pressed_keys_size; i++)
                    {
                        if(keystate.pressed_keys[i]==KEY_LEFT_CONTROL || keystate.pressed_keys[i]==KEY_RIGHT_CONTROL)
                        {
                            keystate.ctrl_pressed=true;
                            break;
                        }
                    }
                    /*ACTUAL KEY HANDLING STARTS HERE*/
                    if(keystate.ctrl_pressed)
                    {
                        /*@todo make these rebindable*/
                        
                        /*open*/
                        if(e.type==KEY_O)
                        {

                        }
                        /*save*/
                        if(e.type==KEY_S)
                        {

                        }
                        /*find, one for just window if pressing alt for all open windows*/
                        if(e.type==KEY_F)
                        {

                        }
                        /*replace, one for just window if pressing alt for all open windows*/
                        if(e.type==KEY_H)
                        {

                        }
                        /*move to token*/
                        if(e.type==KEY_LEFT)
                        {

                        }
                        /*move to token*/
                        if(e.type==KEY_RIGHT)
                        {

                        }
                        /*select all*/
                        if(e.type==KEY_A)
                        {
                            vec2 begin=value_vec2(0,0);
                            vec2 end=value_vec2(strlen(focused_editor->lines[focused_editor->lines_size-1]),focused_editor->lines_size-1);

                            focused_editor->current_text_selection=ctor_text_selection(begin, end, focused_editor);
                            focused_editor->text_selection_origin=begin;
                            focused_editor->text_selection_end=end;
                        }
                        /*open new window*/
                        if(e.type==KEY_N)
                        {
                            editors_size+=1;
                            editors=(editor**)realloc(editors, sizeof(editor*)*editors_size);
                            editors[editors_size-1]=ctor_editor();
                        }
                        if(e.type==KEY_C)
                        {
                            if(focused_editor->current_text_selection)
                            {
                                set_clipboard_text(get_text(focused_editor->text_selection_origin,focused_editor->text_selection_end,focused_editor));
                            }
                        }
                        if(e.type==KEY_Z)
                        {
                            do_action(focused_editor,true);
                        }
                        if(e.type==KEY_Y)
                        {
                            do_action(focused_editor,false);
                        }
                        if(e.type==KEY_V)
                        {
                            char *sdl_clip=get_clipboard_text();
                            char *clip=strcpy(malloc(strlen(sdl_clip)+1),sdl_clip);
                            sdl_free(sdl_clip);
                            clip=str_remove_characters(clip,'\r');
                            if(clip)
                            {
                                add_text(focused_editor,clip,true);
                            }

                        }
                        if(e.type==KEY_X)
                        {
                            if(focused_editor->current_text_selection)
                            {
                                set_clipboard_text(get_text(focused_editor->text_selection_origin,focused_editor->text_selection_end,focused_editor));
                                delete_text(focused_editor->text_selection_origin,focused_editor->text_selection_end, focused_editor, true, ACTION_DELETE);
                                dtor_text_selection(focused_editor->current_text_selection);
                                focused_editor->current_text_selection=NULL;
                                focused_editor->start_selection=false;
                            }
                        }
                    }
                    else
                    {
                        if(e.type==KEY_DELETE)
                        {
                            if(focused_editor->current_text_selection)
                            {
                                delete_text(focused_editor->text_selection_origin, focused_editor->text_selection_end, focused_editor, true, ACTION_DELETE);
                                dtor_text_selection(focused_editor->current_text_selection);
                                focused_editor->current_text_selection=NULL;
                                focused_editor->start_selection=false;
                            }
                            else
                            {
                                vec2 begin;
                                begin.x=focused_editor->cursor_x;
                                begin.y=focused_editor->cursor_y;
                                vec2 end;
                                end.x=focused_editor->cursor_x+1;
                                end.y=focused_editor->cursor_y;

                                
                                if(end.x>strlen(focused_editor->lines[focused_editor->cursor_y]))
                                {                                    
                                    if(focused_editor->cursor_y<focused_editor->lines_size-1)
                                    {
                                        end.x=0;
                                        end.y++;    
                                        delete_text(begin,end,focused_editor,true, ACTION_DELETE);
                                    }
                                }
                                else
                                {
                                    delete_text(begin,end,focused_editor,true,ACTION_DELETE);
                                }
                                /*
                                if(focused_editor->cursor_x<strlen(focused_editor->lines[focused_editor->cursor_y]))
                                {
                                    char *curline=focused_editor->lines[focused_editor->cursor_y];
                                    focused_editor->lines[focused_editor->cursor_y]=str_remove(curline,focused_editor->cursor_x);
                                    
                                    text_block_renderer_set_text(focused_editor->tbr,focused_editor->lines,focused_editor->lines_size,focused_editor->font_color,NULL);
                                }
                                else if(focused_editor->cursor_y<focused_editor->lines_size-1)
                                {
                                    u32 i=0;
                                    char *curline=focused_editor->lines[focused_editor->cursor_y];
                                    char *nextline=focused_editor->lines[focused_editor->cursor_y+1];
                                    u32 strlen_nextline=strlen(nextline);
                                    focused_editor->lines[focused_editor->cursor_y]=str_cat(curline,nextline);
                                    
                                    for(i=focused_editor->cursor_y+1; i<focused_editor->lines_size-1; i++)
                                    {
                                        focused_editor->lines[i]=focused_editor->lines[i+1];
                                    }

                                    focused_editor->lines_size-=1;

                                    focused_editor->lines=realloc(focused_editor->lines,focused_editor->lines_size*sizeof(char*));
                                    
                                    text_block_renderer_set_text(focused_editor->tbr,focused_editor->lines,focused_editor->lines_size,focused_editor->font_color,NULL);
                                    
                                    free(curline);
                                    free(nextline);
                                }
                                */
                            }
                        }
                        if(e.type==KEY_END)
                        {
                            
                            if(focused_editor->current_text_selection)
                            {
                                dtor_text_selection(focused_editor->current_text_selection);
                                focused_editor->current_text_selection=NULL;
                                focused_editor->start_selection=false;
                            }
                            
                            char *curline=focused_editor->lines[focused_editor->cursor_y];
                            u32 last_character=0;
                            for(u32 i=focused_editor->cursor_x; i<strlen(curline); i++)
                            {
                                if(curline[i]!=' ')
                                {
                                    last_character=i+1;
                                }
                            }   

                            if(!last_character)
                            {
                                last_character=strlen(focused_editor->lines[focused_editor->cursor_y]);
                            }

                            editor_set_cursor_position(focused_editor, last_character, focused_editor->cursor_y);
                        }
                        if(e.type==KEY_HOME)
                        {
                            if(focused_editor->current_text_selection)
                            {
                                dtor_text_selection(focused_editor->current_text_selection);
                                focused_editor->current_text_selection=NULL;
                                focused_editor->start_selection=false;
                            }
                            char *curline=focused_editor->lines[focused_editor->cursor_y];
                            u32 last_character=0;

                            if(strlen(curline))
                            {
                                for(s32 i=focused_editor->cursor_x-1; i>=0; i--)
                                {
                                    if(curline[i] != ' ')
                                    {
                                        last_character=i;
                                    }
                                }
                            }

                            editor_set_cursor_position(focused_editor, last_character, focused_editor->cursor_y);
                        }
                        if(e.type==KEY_TAB)
                        {
                            if(focused_editor->current_text_selection)
                            {
                                delete_text(focused_editor->text_selection_origin, focused_editor->text_selection_end,focused_editor,true,ACTION_DELETE);
                                dtor_text_selection(focused_editor->current_text_selection);
                                focused_editor->current_text_selection=NULL;
                                
                                focused_editor->start_selection=false;
                            }
                            char *str=strcpy(malloc(5),"    ");
                            add_text(focused_editor, str,true);
                        }
                        if(e.type==KEY_F11)
                        {
                            focused_editor->is_fullscreen=!focused_editor->is_fullscreen;
                            window_toggle_fullscreen(focused_editor->win,focused_editor->is_fullscreen);
                        }
                        if(e.type >=KEY_SPACE && e.type <= KEY_Z)
                        {
                            if(focused_editor->current_text_selection)
                            {
                                delete_text(focused_editor->text_selection_origin, focused_editor->text_selection_end,focused_editor,true,ACTION_DELETE);
                                dtor_text_selection(focused_editor->current_text_selection);
                                focused_editor->current_text_selection=NULL;
                                focused_editor->start_selection=false;
                            }

                            char *curline=focused_editor->lines[focused_editor->cursor_y];
                            char char_to_insert=e.type;
                            
                            keystate.shift_pressed=false;
                            for(u32 i=0; i<keystate.pressed_keys_size; i++)
                            {
                                if(keystate.pressed_keys[i]==KEY_LEFT_SHIFT || keystate.pressed_keys[i]==KEY_RIGHT_SHIFT)
                                {
                                    keystate.shift_pressed=true;
                                    break;
                                }
                            }
                            
                            if(keystate.shift_pressed)
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
                            add_text(focused_editor,str,true); 
                        }
                        if(e.type==KEY_LEFT_SHIFT||e.type==KEY_RIGHT_SHIFT)
                        {
                            /*prolly dont do anything in here since itll get called on heldkey wot
                            also these keys get just checked in the array instead of changing state on press*/
                        }
                        if(e.type==KEY_LEFT_CONTROL||e.type==KEY_RIGHT_CONTROL)
                        {
                            /*prolly dont do anything in here since itll get called on heldkey wot
                            also these keys get just checked in the array instead of changing state on press*/
                        }
                        if(e.type==KEY_LEFT_ALT||e.type==KEY_RIGHT_ALT)
                        {
                            /*prolly dont do anything in here since itll get called on heldkey wot
                            also these keys get just checked in the array instead of changing state on press*/                            
                        }
                        if(e.type==KEY_ENTER)
                        {
                            if(focused_editor->current_text_selection)
                            {
                                delete_text(focused_editor->text_selection_origin, focused_editor->text_selection_end,focused_editor,true,ACTION_DELETE);
                                dtor_text_selection(focused_editor->current_text_selection);
                                focused_editor->current_text_selection=NULL;
                                focused_editor->start_selection=false;
                            }
                            add_text(focused_editor,strcpy(malloc(2),"\n"), true);
                            /*
                            u32 i;
                            char *curline=focused_editor->lines[focused_editor->cursor_y];
                            char *startsplice;
                            char *endsplice;
                            if(strlen(curline))
                            {
                                startsplice=malloc_str_slice(curline,0,focused_editor->cursor_x-1);
                                endsplice=malloc_str_slice(curline,focused_editor->cursor_x,strlen(curline)-1);
                                free(curline);
                            }
                            else
                            {
                                startsplice=malloc(1);
                                startsplice[0]=(char)NULL;
                                
                                endsplice=malloc(1);
                                endsplice[0]=(char)NULL;
                            }
                            
                            focused_editor->lines[focused_editor->cursor_y]=startsplice;
                            
                            focused_editor->lines_size+=1;
                            focused_editor->lines=realloc(focused_editor->lines,focused_editor->lines_size*sizeof(char*));
                            for(i=focused_editor->lines_size-1; i>focused_editor->cursor_y+1; i--)
                            {
                                focused_editor->lines[i]=focused_editor->lines[i-1];
                            }
                            focused_editor->lines[focused_editor->cursor_y+1]=endsplice;
                            text_block_renderer_set_text(focused_editor->tbr,focused_editor->lines,focused_editor->lines_size,focused_editor->font_color,NULL);
                            editor_set_cursor_position(focused_editor,0,focused_editor->cursor_y+1);
                            */                             
                        }
                        if(e.type==KEY_BACKSPACE)
                        {
                            if(focused_editor->current_text_selection)
                            {
                                delete_text(focused_editor->text_selection_origin, focused_editor->text_selection_end,focused_editor,true,ACTION_DELETE);
                                dtor_text_selection(focused_editor->current_text_selection);
                                focused_editor->current_text_selection=NULL;
                                focused_editor->start_selection=false;
                            }
                            else
                            {
                                vec2 begin;
                                begin.x=((s32)focused_editor->cursor_x)-1;
                                begin.y=focused_editor->cursor_y;
                                vec2 end;
                                end.x=focused_editor->cursor_x;
                                end.y=focused_editor->cursor_y;

                                  
                                if(begin.x<0)
                                {                           
                                    if(focused_editor->cursor_y>0)
                                    {
                                        begin.x=strlen(focused_editor->lines[focused_editor->cursor_y-1]);
                                        begin.y--;    
                                        delete_text(begin,end,focused_editor,true,ACTION_DELETE);
                                    }
                                }
                                else
                                {
                                    delete_text(begin,end,focused_editor,true,ACTION_BACKSPACE);
                                }
                                
                                 /*
                                if(focused_editor->cursor_x)
                                {
                                    char *curline=focused_editor->lines[focused_editor->cursor_y];
                                    focused_editor->cursor_x-=1;
                                    focused_editor->lines[focused_editor->cursor_y]=str_remove(curline,focused_editor->cursor_x);
                                    
                                    text_block_renderer_set_text(focused_editor->tbr,focused_editor->lines,focused_editor->lines_size,focused_editor->font_color,NULL);
                                    editor_set_cursor_position(focused_editor,focused_editor->cursor_x,focused_editor->cursor_y);
                                }
                                else if(focused_editor->cursor_y)
                                {
                                    u32 i=0;
                                    char *curline=focused_editor->lines[focused_editor->cursor_y];
                                    char *prevline=focused_editor->lines[focused_editor->cursor_y-1];
                                    u32 strlen_prevline=strlen(prevline);
                                    focused_editor->lines[focused_editor->cursor_y-1]=str_cat(prevline,curline);
                                    
                                    for(i=focused_editor->cursor_y; i<focused_editor->lines_size-1; i++)
                                    {
                                        focused_editor->lines[i]=focused_editor->lines[i+1];
                                    }

                                    focused_editor->lines_size-=1;

                                    focused_editor->lines=realloc(focused_editor->lines,focused_editor->lines_size*sizeof(char*));
                                    
                                    text_block_renderer_set_text(focused_editor->tbr,focused_editor->lines,focused_editor->lines_size,focused_editor->font_color,NULL);
                                    editor_set_cursor_position(focused_editor,strlen_prevline,focused_editor->cursor_y-1);
                                    
                                    free(curline);
                                    free(prevline);
                                }
                                */
                            }
                        }
                        if(e.type==KEY_CAPS_LOCK)
                        {
                            /*we handle this differently now, but might want to do something if they actually click the key someday ? ?*/
                        }
                        if(e.type==KEY_LEFT)
                        {
                            if(focused_editor->current_text_selection)
                            {
                                dtor_text_selection(focused_editor->current_text_selection);
                                focused_editor->current_text_selection=NULL;
                                focused_editor->start_selection=false;
                            }
                            if(focused_editor->cursor_x)
                            {
                                editor_set_cursor_position(focused_editor,focused_editor->cursor_x - 1, focused_editor->cursor_y);
                            }
                            else
                            {
                                if(focused_editor->cursor_y>0)
                                {
                                    u32 ypos=focused_editor->cursor_y-1;
                                    editor_set_cursor_position(focused_editor,strlen(focused_editor->lines[ypos]), focused_editor->cursor_y-1);
                                }
                            }
                            
                             
                        }
                        if(e.type==KEY_RIGHT)
                        {
                            
                            if(focused_editor->current_text_selection)
                            {
                                dtor_text_selection(focused_editor->current_text_selection);
                                focused_editor->current_text_selection=NULL;
                                focused_editor->start_selection=false;
                            }
                            if(focused_editor->cursor_x < strlen(focused_editor->lines[focused_editor->cursor_y]))
                            {
                                editor_set_cursor_position(focused_editor,focused_editor->cursor_x + 1, focused_editor->cursor_y);
                            }
                            else
                            {
                                if(focused_editor->cursor_y<focused_editor->lines_size-1)
                                {
                                    u32 ypos=focused_editor->cursor_y+1;
                                    editor_set_cursor_position(focused_editor,0, focused_editor->cursor_y+1);
                                }
                            }

                             
                        }
                        if(e.type==KEY_UP)
                        {
                            
                            if(focused_editor->current_text_selection)
                            {
                                dtor_text_selection(focused_editor->current_text_selection);
                                focused_editor->current_text_selection=NULL;
                                focused_editor->start_selection=false;
                            }
                            if(focused_editor->cursor_y>0)
                            {
                                if(focused_editor->cursor_x>=strlen(focused_editor->lines[focused_editor->cursor_y-1]))
                                {
                                    focused_editor->cursor_x=strlen(focused_editor->lines[focused_editor->cursor_y-1]);
                                }
                                editor_set_cursor_position(focused_editor,focused_editor->cursor_x, focused_editor->cursor_y - 1);
                            }
                            
                             
                        }
                        if(e.type==KEY_DOWN)
                        {
                            
                            if(focused_editor->current_text_selection)
                            {
                                dtor_text_selection(focused_editor->current_text_selection);
                                focused_editor->current_text_selection=NULL;
                                focused_editor->start_selection=false;
                            }
                            if(focused_editor->cursor_y<focused_editor->lines_size-1)
                            {
                                
                                if(focused_editor->cursor_x>=strlen(focused_editor->lines[focused_editor->cursor_y+1]))
                                {
                                    focused_editor->cursor_x=strlen(focused_editor->lines[focused_editor->cursor_y+1]);
                                }
                                editor_set_cursor_position(focused_editor,focused_editor->cursor_x, focused_editor->cursor_y + 1);
                            }
                            
                             
                        }
                    }
                    
                    cursor_blink_state=true;
                    entity_set_visible(focused_editor->cursor,cursor_blink_state);
                    cursor_blink_timer=0;
                }
            }           
        }
        
        if(cursor_blink_timer>500)
        {
            cursor_blink_state=!cursor_blink_state;
            entity_set_visible(focused_editor->cursor,cursor_blink_state);
            cursor_blink_timer=0;
        }
        cursor_blink_timer+=milli_current_time()-last_time;
        last_time=milli_current_time();

        for(i=0; i<editors_size; i++)
        {
            int w;
            int h;
            window_get_size(editors[i]->win,&w,&h);
            entity_set_size(editors[i]->root,value_vec2(w,h));

            clear(editors[i]->win);
            update_entity_recursive(editors[i]->root);
            render_entity_recursive(editors[i]->root);
            flip(editors[i]->win);
                
            if(!editors[i]->wheel_override)
            {
                vec2 cursorpos=entity_get_render_position(editors[i]->cursor);
                vec2 cursorsize=entity_get_render_size(editors[i]->cursor);

                if(cursorpos.x+cursorsize.x>w)
                {
                    offset.x+=w-(cursorpos.x+cursorsize.x);
                }
                if(cursorpos.y+cursorsize.y>h)
                {
                    offset.y+=h-(cursorpos.y+cursorsize.y);
                }
                if(cursorpos.x<0)
                {
                    offset.x+=-(cursorpos.x);
                }
                if(cursorpos.y<0)
                {
                    offset.y+=-(cursorpos.y);
                }
            }
            
            if(offset.x > 0)
            {
                offset.x=0;
            }
            if(offset.y > 0)
            {
                offset.y=0;
            }

            entity_set_position(editors[i]->text_holder,offset);
        }

        if(milli_current_time()-frame_time_stamp<1000.0f/60.0f)
        {
            sleep_milli(1000.0f/60.0f-(milli_current_time()-frame_time_stamp));
        }
    }
}