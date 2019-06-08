#include "ote/src/h/base.h"
#include "ote/src/h/action.h"
#include "ote/src/h/editor.h"

#include "oul/src/h/oul.h"
#include "ovp/src/h/ovp.h"
#include "opl/src/h/input.h"
#include "opl/src/h/system.h"

#include "../h/shell.h"

#include <stdlib.h>
#include <string.h>

texture *ctor_texture_file(window *w, char const *path){
    surface *s=ctor_surface_file(path);
    texture *t=ctor_texture(w,s);
    dtor_surface(s);
    return t;
}

static void editor_update_page_tabs(editor *e);
static void editor_close_tab(editor *e, page_tab *p);

typedef struct page_tab
{
    renderer *ent_renderer;
    entity *ent;

    entity *close_button;
    entity *file_name;
    renderer *close_button_renderer;
    renderer *file_name_renderer;
    texture *close_button_normal;
    texture *file_name_tex;
    texture *close_button_hover;

    entity *file_text;

    texture *tab_color_good;
    texture *tab_color_bad;
    char *file_path;//if NULL working with a new buffer

    color font_color;
    ttf_font *font;

    keystate_interpreter_info keystate;
    bool start_selection_mouse;
    bool start_selection_key;
    bool wheel_override;

    action **action_list;
    s32 action_list_size;
    s32 action_list_index;

    s32 action_on_save;

    text_selection *current_text_selection;
    vec2 text_selection_origin;
    vec2 text_selection_end;

    entity *text_holder;
    entity *text_entity;

    text_block_renderer *tbr;

    char **lines;
    s32 lines_size;
    s32 cursor_x;
    s32 cursor_y;

    entity *cursor;
    bool cursor_blink_state;
    s64 cursor_blink_timer;
    s64 last_time;
    s64 held_key;
    s64 held_key_time_stamp;
    vec2 offset;

    char *ext;
} page_tab;

typedef struct editor
{   
    entity *highest;
    
    bool is_fullscreen;
    window *win;
    entity *root;
    bool mouse_dirty;
    entity *focused_entity;//used for UI later, will be current_page_tab or some UI entity
    page_tab *current_page_tab;
    page_tab **page_tabs;
    s32 page_tabs_size;

    /*
    shell UI
    */
    texture *shell_bg;
    texture *shell_cursor;
    texture *shell_start;
    char *shell_text;
    texture *shell_text_tex;//created every frame
    int mode;
    int shell_pos;
} editor;



//@TODO move checks in here, rather than doing them outside also
void editor_set_cursor_position(editor *e, s32 x, s32 y)
{
    //@todo modify current text selection if it exists
    
    if(y>=editor_get_lines_size(e))
    {
        y=e->current_page_tab->lines_size-1;
    }
    else if(y<0)
    {
        y=0;
    }
    else if(x>strlen(editor_get_line(e,y)))
    {
        x=strlen(editor_get_line(e,y));
    }
    else if(x<0)
    {
        x=0;
    }

    if(e->current_page_tab->current_text_selection)
    {
        dtor_text_selection(e);
        
        vec2 new_selection=value_vec2(x,y);
        vec2 pos=e->current_page_tab->text_selection_origin;
        //printf("ENTRY: new_selection: (%d,%d), pos: (%d,%d)\n",(int)new_selection.x,(int)new_selection.y,(int)pos.x,(int)pos.y);
        if((new_selection.y<e->current_page_tab->text_selection_origin.y) || (e->current_page_tab->text_selection_origin.y==new_selection.y && e->current_page_tab->text_selection_origin.x > new_selection.x))
        {
            vec2 temp=new_selection;
            new_selection=pos;
            pos=temp;
        }

        //printf("AFTER: new_selection: (%d,%d), pos: (%d,%d)\n\n",(int)new_selection.x,(int)new_selection.y,(int)pos.x,(int)pos.y);
        e->current_page_tab->current_text_selection=ctor_text_selection(pos, new_selection, e);
        
        e->current_page_tab->text_selection_end=value_vec2(x,y);
    }            
    
    s32 i;
    e->current_page_tab->cursor_x=x;
    e->current_page_tab->cursor_y=y;

    text_block_renderer_set_curline(e->current_page_tab->tbr,y);
    
    u32 width;
    u32 height;
    e->current_page_tab->wheel_override=false;
    char *spliced_str=NULL;
    if(x==0)
    {//str_slice returns null or something if the end and begin are the same index, and ttf_size_text has undefined behavior if string is null
        spliced_str=strcpy(malloc(2),"_");

        size_ttf_font(e->current_page_tab->font,spliced_str,&width,&height);
        
        entity_set_position(e->current_page_tab->cursor,value_vec2(0,height*y));
    }
    else
    {
        spliced_str=alloc_str_slice(editor_get_line(e,y),0,x-1);

        size_ttf_font(e->current_page_tab->font,spliced_str,&width,&height);
        
        entity_set_position(e->current_page_tab->cursor,value_vec2(width,height*y));
    }

    if(spliced_str)
    {
        free(spliced_str);
    }
}

page_tab *ctor_page_tab(editor *e, char *filepath, ovp *config)
{    
    create_empty_file_if_not_exists(filepath);

    //roots are ent and text_holder
    page_tab *p=malloc(sizeof(page_tab));
    memset(p,0,sizeof(page_tab));
    u32 i;
    entity *button_holder=ctor_entity(e->root);
    
    p->ent=button_holder;
    p->action_on_save=-1;
    entity_set_visible(p->ent, false);

    char *base_path=get_base_path();

    char *adjpath=str_cat(base_path,"../res/close.png");
    //@todo make these entities and implement editor_close_tab
    p->close_button=ctor_entity(p->ent);
    
    p->close_button_normal=ctor_texture_file(e->win,adjpath);
    free(adjpath);
    adjpath=str_cat(base_path,"../res/close_hover.png");
    p->close_button_hover=ctor_texture_file(e->win,adjpath);
    free(adjpath);
    
    p->close_button_renderer=(renderer*)ctor_image_renderer(e->win,p->close_button_normal,NULL);
    entity_add_renderer(p->close_button,p->close_button_renderer);
    
    entity_set_size(p->close_button, value_vec2(10,10));
    entity_set_relpos(p->close_button,value_vec2(1,.5));
    entity_set_relposme(p->close_button,value_vec2(-1,-.5));
    entity_set_position(p->close_button,value_vec2(-25/2,0));
    
    /*@todo
    dont even create this with the normal renderer here. instead rework the renderers so that 
    they are used in a binding layer which is the only object that includes the platform.
    just a switch over render types instead of virtual functions blah blah blah
    then will be easier to add the new text renderer type
    */
    p->file_text=ctor_entity(p->ent);
    

    p->file_path=strcpy(malloc(strlen(filepath)+1),filepath);//might be parameter
    
    //entity_set_relsize(button_holder,value_vec2(.2,.05));
    
    entity_set_order(button_holder,999);


    color whitecolor={255,255,255,255};
    color redcolor={255,0,0,255};

    p->tab_color_bad=ctor_texture_pixels(e->win,&redcolor,1,1);
    
    p->tab_color_good=ctor_texture_pixels(e->win,&whitecolor,1,1);
    
    
    p->ent_renderer=(renderer*)ctor_image_renderer(e->win,p->tab_color_good,NULL);
    entity_add_renderer(button_holder,(renderer*)p->ent_renderer);
    
    d_append(e->page_tabs,e->page_tabs_size,p);
    
    p->text_holder=ctor_entity(e->root);
    entity_set_solid(p->text_holder,false);
    entity_set_relsize(p->text_holder, value_vec2(1,1));
    entity_set_visible(p->text_holder, false);
    //entity_set_relpos(e->current_page_tab->text_holder, value_vec2(0,0));

    
    memset(&p->keystate,0,sizeof(p->keystate));

    p->cursor_blink_state=true;
    p->cursor_blink_timer=0;
    p->last_time=milli_current_time();

    p->held_key=0;
    p->held_key_time_stamp=0;

    p->offset=value_vec2(0,0);
    
    p->wheel_override=false;
    p->current_text_selection=NULL;
    p->text_selection_origin.x=0;
    p->text_selection_origin.y=0;
    p->text_selection_end.x=0;
    p->text_selection_end.y=0;

    p->text_entity=ctor_entity(p->text_holder);
    entity_set_solid(p->text_entity,false);
    entity_set_relsize(p->text_entity, value_vec2(1,1));
    entity_set_order(p->text_entity,100);

    p->font_color=value_color(255,255,255,255);

    char **c=malloc(sizeof(char*));
    c[0]=strcpy(malloc(strlen("")+1),"");

    p->action_list=NULL;
    p->action_list_size=0;
    p->action_list_index=0;


    p->lines=c;
    p->lines_size=1;

    p->cursor=ctor_entity(p->text_entity);
    entity_set_solid(p->cursor,false);
    
    entity_set_size(p->cursor,value_vec2(2,global_font_size));//@todo size text and set to actual render size, in addition to rendering cursor in code multicolor support etc, have to size text width to move cursor anyway
    
    adjpath=str_cat(base_path,"../res/cursor.png");
    entity_add_renderer(p->cursor,(renderer*)ctor_image_renderer(e->win,ctor_texture_file(e->win,adjpath),NULL));
    free(adjpath);

    p->cursor_x=0;
    p->cursor_y=0;
    
    adjpath=str_cat(base_path,(char*)global_font_url);
    p->font=ctor_ttf_font(adjpath,global_font_size);
    p->ext=get_ext(filepath);
    ttf_font *fn_font=ctor_ttf_font(adjpath,12);
    free(adjpath);
    //@todo also change this so i construct a ttf font before hand instead of loading it from url
    text_block_renderer *r=ctor_text_block_renderer(e->win,p->font,true,&global_text_margin,"left", config,p->font_color);
    text_block_renderer_set_curline(r,p->cursor_y);
    text_block_renderer_add_lines(r,0,1);
    
    p->tbr=r;
    entity_add_renderer(p->text_entity,(renderer*)r);

    p->start_selection_mouse=false;
    p->start_selection_key=false;

    {/*file name entity*/
        p->file_name=ctor_entity(p->ent);
        entity_set_solid(p->file_name,false);
        color black_color={0,0,0,255};
        p->file_name_tex=ctor_texture_font(e->win,fn_font,filepath,black_color);
    
        p->file_name_renderer=(renderer*)ctor_image_renderer(e->win,p->file_name_tex,NULL);
        entity_add_renderer(p->file_name,p->file_name_renderer);
        int fn_h,fn_w;
        size_ttf_font(fn_font,filepath,&fn_w,&fn_h);
        
        entity_set_size(p->file_name, value_vec2(fn_w,fn_h));
        entity_set_relpos(p->file_name,value_vec2(.5,.5));
        entity_set_relposme(p->file_name,value_vec2(-.5,-.5));

        dtor_ttf_font(fn_font);
    }

    editor_set_current_page_tab(e,p);
    editor_add_text(e,alloc_file_to_str(filepath),false);//@leak can free here since false add_action

    system_free(base_path);

    editor_update_page_tabs(e);
    //@bug @note have to sort children manually now
    entity_sort_children(e->root);
    
    editor_set_cursor_position(e, 0, 0);
    
    
    return p;
}
void dtor_page_tab(page_tab *p)
{
    //@todo free lines after refactor
    if(p->ext)free(p->ext);
    for(u32 i=0; i<p->action_list_size; i++)
    {
        //@valgrind
        dtor_action(p->action_list[i]);
    }
    free(p->action_list);
    dtor_entity(p->ent);
    dtor_entity(p->text_holder);
    //@todo @leak free all textures, renderers, etc.
    //dtor_ttf_font(p->font);
    //dtor_text_block_renderer(p->tbr);
}

static void editor_update_page_tabs(editor *e)
{
    for(s32 i=0; i<e->page_tabs_size; i++)
    {
        entity_set_size(e->page_tabs[i]->ent, value_vec2(standard_button_width,standard_button_height));
        entity_set_relposme(e->page_tabs[i]->ent,value_vec2(0,i));
        entity_set_position(e->page_tabs[i]->ent,value_vec2(0,1*i));
    }
}
static void editor_close_tab(editor *e, page_tab *p)
{
    for(s32 i=0; i<e->page_tabs_size; i++)
    {
        if(e->page_tabs[i]==p)
        {
            page_tab *dead_tab=e->page_tabs[i];
            if(dead_tab==e->current_page_tab)
            {
                e->current_page_tab=NULL;
            }
            d_remove(e->page_tabs,e->page_tabs_size,i,dtor_page_tab);
            break;
        }
    }
    
    editor_update_page_tabs(e);

    if(!e->current_page_tab)
    {
        if(e->page_tabs_size)
        {
            editor_set_current_page_tab(e,e->page_tabs[0]);
        }
    }
    e->mouse_dirty=true;
}    

editor *ctor_editor()
{
    editor *e=(editor*)malloc(sizeof(editor));


    e->win=ctor_window("ote", 800, 450);
    e->root=ctor_entity(NULL);
    e->current_page_tab=NULL;
    e->highest=NULL;
    e->page_tabs=NULL;
    e->page_tabs_size=0;    
    e->mouse_dirty=false;
    e->is_fullscreen=false;//assumes windows start in windowed mode

    e->mode=0;
    color shell_bg_color={255,255,255,255};
    e->shell_bg=ctor_texture_pixels(e->win,&shell_bg_color,1,1);
    if(0){
        e->shell_text="> replace \"asd\",\"asd2\"";
    }
    else{
        e->shell_text=malloc(1);
        e->shell_text[0]=0;
    }
    e->shell_text_tex=NULL;
    e->shell_pos=0;
    color black2={0,0,0,255};
    e->shell_cursor=ctor_texture_pixels(e->win,&black2,1,1);
    e->shell_start=NULL;

    char *base_path=get_base_path();
    char *adjpath=str_cat(base_path,"../script/icon/OTE.png");
    window_set_icon(e->win,adjpath);
    free(adjpath);
    system_free(base_path);

    window_set_position(e->win,100,100);
    flush_events(MOUSE_EVENTS);
    return e;
}
void dtor_editor(editor *e)
{
    dtor_window(e->win);

    for(s32 i=0; i<e->page_tabs_size; i++)
    {
        dtor_page_tab(e->page_tabs[i]);
    }

    //this has to be after dtor_page_tab because it holds references to entities that it frees explicitly
    dtor_entity(e->root);/*@todo implement this function and it should free all children*/

    if(e->shell_bg){
        dtor_texture(e->shell_bg);
    }
    if(e->shell_text){
        free(e->shell_text);
    }
    if(e->shell_text_tex){
        dtor_texture(e->shell_text_tex);
    }
    if(e->shell_cursor){
        dtor_texture(e->shell_cursor);
    }
    if(e->shell_start){
        dtor_texture(e->shell_start);
    }

    free(e);
}

vec2 editor_find_starting_at(editor *e, char *str, int x, int y){
    page_tab *pt=e->current_page_tab;
    int i;
    int j=x;
    int strlen_str=strlen(str);
    for(i=y; i<pt->lines_size; i++){
        for(;pt->lines[i][j];j++){
            char *check=alloc_str_slice(pt->lines[i],j,j+strlen_str-1);
            
            if(str_eq(check,str)){
            
                editor_set_cursor_position(e,j+strlen_str,i);
                free(check);
                return value_vec2(j,i);
            }
            else{
                free(check);
            }
        }
        j=0;
    }
    return value_vec2(-1,-1);
}
bool editor_find(editor *e, char *str){
    page_tab *pt=e->current_page_tab;
    vec2 pos=editor_find_starting_at(e,str,pt->cursor_x,pt->cursor_y);

    /*search wrap*/
    if(pos.x==-1){
        pos=editor_find_starting_at(e,str,0,0);
    }

    return pos.x!=-1;
}
bool editor_replace(editor *e, char *old, char *new){
    page_tab *pt=e->current_page_tab;
    vec2 pos=editor_find_starting_at(e,old,pt->cursor_x,pt->cursor_y);

    /*search wrap*/
    if(pos.x==-1){
        pos=editor_find_starting_at(e,old,0,0);
    }

    if(pos.x!=-1){
        vec2 end;
        
        end.x=pos.x+strlen(old);
        end.y=pos.y;
        
        editor_delete_text(e,pos,end,true,ACTION_DELETE);
        editor_set_cursor_position(e,pos.x,pos.y);
        editor_add_text(e, strcpy(malloc(strlen(new)+1),new) ,true);
        return true;
    }
    return false;
}
bool editor_replace_all(editor *e, char *old, char *new){
    bool found_any=false;
    while(editor_replace(e,old,new)){
        found_any=true;
    }
    return found_any;
}

void editor_set_current_page_tab(editor *e, page_tab *p)
{
    if(e->current_page_tab)
    {
        entity_set_visible(e->current_page_tab->text_holder,false);
    }
    e->current_page_tab=p;

    for(u32 i=0; i<e->page_tabs_size; i++)
    {
        entity_set_alpha(e->page_tabs[i]->ent,standard_button_alpha);
    }
    entity_set_alpha(p->ent,standard_button_selected_alpha);
    
    entity_set_visible(e->current_page_tab->text_holder,true);
}
page_tab *editor_get_current_page_tab(editor *e)
{
    return e->current_page_tab;
}
char *editor_get_line(editor *e, s32 line_y)
{
    return e->current_page_tab->lines[line_y];
}
s32 editor_get_lines_size(editor *e)
{
    return e->current_page_tab->lines_size;
}
void editor_set_mouse_dirty(editor *e)
{
    e->mouse_dirty=true;
}
void editor_update(editor *e_instance)
{

    int w;
    int h;

    window_get_size(e_instance->win,&w,&h);
    entity_set_size(e_instance->root,value_vec2(w,h));

    //only update current page tab?
    page_tab *pt=e_instance->current_page_tab;
    if(pt)
    {

        //calculate offset after updating all entities
        update_entity_recursive(e_instance->root);

        /*@todo
        fix mouse_dirty, right now its not effective.
        it should trigger mouse_motion next frame after the entities are updated to get the new highest
        it doesnt for some reason

        @leak in dtor_page_tab
        @bug some numbers are being printed idk where its happening

        @bug entity destructor on program exit
        @bug undo/redo
        @bug horizontal scrolling
        */
        if(e_instance->mouse_dirty)
        {
            //printf("%f,%f\n",get_mouse_position().x,get_mouse_position().y);
            editor_handle_mouse_motion(e_instance,get_mouse_position());
            e_instance->mouse_dirty=false;
        }

        //there is no valid action list_index, so the file is unmodified (a full undo has happened perhaps)
        if(pt->action_list_index<=0)
        {
            image_renderer_set_texture((image_renderer*)e_instance->current_page_tab->ent_renderer,e_instance->current_page_tab->tab_color_good);
        }
        else if(pt->action_list_size)
        {            
            if(pt->action_on_save==pt->action_list[pt->action_list_index]->uid)
            {
                image_renderer_set_texture((image_renderer*)e_instance->current_page_tab->ent_renderer,e_instance->current_page_tab->tab_color_good);
            }
            else
            {
                image_renderer_set_texture((image_renderer*)e_instance->current_page_tab->ent_renderer,e_instance->current_page_tab->tab_color_bad);
            }
        }

        if(global_text_margin)
        {
            char buffer[MAXIMUM_LINE_NUMBER_LENGTH];
            s32 size_width;
            s32 size_height;
            itoa(pt->lines_size,buffer,10);
            size_ttf_font(pt->font,buffer,&size_width,&size_height);
            if(size_width+offset_margin > global_text_margin){
                global_text_margin=size_width+offset_margin;   
            }
        }
        
        if(pt->cursor_blink_timer>500)
        {
            pt->cursor_blink_state=!pt->cursor_blink_state;
            entity_set_visible(pt->cursor,pt->cursor_blink_state);
            pt->cursor_blink_timer=0;
        }
        pt->cursor_blink_timer+=milli_current_time()-pt->last_time;
        pt->last_time=milli_current_time();

        if(!pt->wheel_override)
        {
            vec2 cursorpos=entity_get_render_position(pt->cursor);
            vec2 cursorsize=entity_get_render_size(pt->cursor);

            s32 cpos_x=cursorpos.x;
            s32 cpos_y=cursorpos.y;
            s32 csize_x=cursorsize.x;
            s32 csize_y=cursorsize.y;


            if((cpos_x+csize_x)>w)
            {
                pt->offset.x += ((w)-(cpos_x+csize_x));
            }
            if(cpos_y+csize_y>h)
            {
                pt->offset.y += h-(cpos_y+csize_y);
            }
            if(cpos_x<global_text_margin)
            {
                pt->offset.x += (global_text_margin-cpos_x);
            }
            if(cpos_y<0)
            {
                pt->offset.y += -(cpos_y);
            }
        }

        
        vec2 v;
        v.x=pt->offset.x;
        v.y=pt->offset.y;
        entity_set_position(pt->text_holder,v);   
    }
}
void draw_shell(editor *e_instance){

    if(e_instance->shell_text_tex){
        dtor_texture(e_instance->shell_text_tex);
        e_instance->shell_text_tex=NULL;
    }
    if(e_instance->shell_start){
        
        dtor_texture(e_instance->shell_start);
        e_instance->shell_start=NULL;
    }

    if(e_instance->mode){/*draw shell UI*/
        int win_w,win_h;
        window_get_size(e_instance->win,&win_w,&win_h);
        
        rect bgdest;
        bgdest.x=0;
        bgdest.y=0;
        bgdest.w=win_w;

        int shw,shh;
        rect textrect;
        textrect.y=0;
        textrect.w=0;
        textrect.h=0;
        

        color shelltextcolor={0,0,0,255};

        int single_shw,single_shh;

        size_ttf_font(e_instance->current_page_tab->font,">",&single_shw,&single_shh);

        e_instance->shell_start=ctor_texture_font(e_instance->win,e_instance->current_page_tab->font,">",shelltextcolor);
        int margin=single_shh/8;
        rect shell_start;
        shell_start.x=margin;
        shell_start.y=0;
        shell_start.w=single_shw;
        shell_start.h=single_shh;

        textrect.x=shell_start.x+shell_start.w+margin;


        bgdest.h=single_shh;

        if(e_instance->shell_text && strlen(e_instance->shell_text)){

            e_instance->shell_text_tex=ctor_texture_font(e_instance->win,e_instance->current_page_tab->font,e_instance->shell_text,shelltextcolor);
            
            size_ttf_font(e_instance->current_page_tab->font,e_instance->shell_text,&shw,&shh);
            
            textrect.y=0;
            textrect.w=shw;
            textrect.h=shh;
        }
        
        draw_texture(e_instance->win,e_instance->shell_bg,&bgdest,0,NULL,NULL,NULL);     
        
        draw_texture(e_instance->win,e_instance->shell_start,&shell_start,0,NULL,NULL,NULL);     
        
        if(e_instance->shell_text){
            draw_texture(e_instance->win,e_instance->shell_text_tex,&textrect,0,NULL,NULL,NULL);
        }

        int cursor_margin=2;
        rect cursor_rect;
        cursor_rect.x=textrect.x+(single_shw*e_instance->shell_pos);
        cursor_rect.y=cursor_margin/2;
        cursor_rect.w=single_shw;
        cursor_rect.h=single_shh-cursor_margin;
        
        draw_texture(e_instance->win,e_instance->shell_cursor,&cursor_rect,0,NULL,NULL,NULL);
    }
}
void editor_draw(editor *e_instance)
{
    clear(e_instance->win);
    render_entity_recursive(e_instance->root);
    draw_shell(e_instance);
    flip(e_instance->win);
}


window *editor_get_window(editor *e)
{
    return e->win;
}
ttf_font *editor_get_font(editor *e)
{
    return e->current_page_tab->font;
}
entity *editor_get_text_holder(editor *e)
{
    return e->current_page_tab->text_holder;
}
text_selection *editor_get_current_text_selection(editor *e)
{
    return e->current_page_tab->current_text_selection;
}
void editor_set_current_text_selection(editor *e, text_selection *ts)
{
    e->current_page_tab->current_text_selection=ts;
}
void editor_set_line(editor *e,s32 y,char *str)
{
    e->current_page_tab->lines[y]=str;
}
entity *editor_get_highest(editor *e)
{
    return e->highest;
}
entity *editor_get_root(editor *e)
{
    return e->root;
}

void editor_remove_line(editor *focused_editor,s32 y)
{
    for(s32 i2=y; i2<editor_get_lines_size(focused_editor)-1; i2++)
    {
        editor_set_line(focused_editor,i2,editor_get_line(focused_editor,i2+1));
    }

    focused_editor->current_page_tab->lines_size-=1;
    focused_editor->current_page_tab->lines=realloc(focused_editor->current_page_tab->lines,focused_editor->current_page_tab->lines_size*sizeof(char*));
}
void editor_insert_line(editor *focused_editor, s32 y, char *str)
{
    focused_editor->current_page_tab->lines_size+=1;
    focused_editor->current_page_tab->lines=realloc(focused_editor->current_page_tab->lines,focused_editor->current_page_tab->lines_size*sizeof(char*));
    for(u32 i=focused_editor->current_page_tab->lines_size-1; i>y; i--)
    {
        focused_editor->current_page_tab->lines[i]=focused_editor->current_page_tab->lines[i-1];
    }
    focused_editor->current_page_tab->lines[y]=str;
            
}
void editor_handle_mouse_motion(editor *e, vec2 mouse_position)
{
    s32 i;
    set_cursor(CURSOR_TEXT);
    entity *highest=hit_test_recursive(mouse_position,e->root,e->root);
    for(i=0; i<e->page_tabs_size; i++)
    {
        if(e->page_tabs[i]!=e->current_page_tab)
        {
            if(entity_is_or_is_recursive_child(e->page_tabs[i]->ent,highest))
            {
                entity_set_alpha(e->page_tabs[i]->ent,standard_button_hover_alpha);
            }
            else
            {
                entity_set_alpha(e->page_tabs[i]->ent,standard_button_alpha);
            }
        }

        if(entity_is_or_is_recursive_child(e->page_tabs[i]->ent,highest))
        {
            set_cursor(CURSOR_NORMAL);      
        }

        if(highest==e->page_tabs[i]->close_button)
        {
            set_cursor(CURSOR_HAND);
            image_renderer_set_texture((image_renderer*)e->page_tabs[i]->close_button_renderer,e->page_tabs[i]->close_button_hover);
        }
        else
        {
            image_renderer_set_texture((image_renderer*)e->page_tabs[i]->close_button_renderer,e->page_tabs[i]->close_button_normal);
        }
    }
    e->highest=highest;
    
    if(e->page_tabs_size)
    {
        if(mouse_position.x<=global_text_margin-10)
        {
            for(i=0; i<e->page_tabs_size; i++)
            {
                entity_set_visible(e->page_tabs[i]->ent,true);
            }                
        }
        if(mouse_position.x>entity_get_render_size(e->page_tabs[0]->ent).x)
        {
            for(i=0; i<e->page_tabs_size; i++)
            {
                entity_set_visible(e->page_tabs[i]->ent,false);
            }                
        }
    }
                
    if(e->current_page_tab->start_selection_mouse)
    {
        vec2 new_selection=editor_position_to_cursor(e,vec2_sub(mouse_position,e->current_page_tab->offset));
        editor_set_cursor_position(e, new_selection.x, new_selection.y);
    }
}
void editor_handle_mouse_wheel(editor *e, r32 mousewheel)
{
    /*disabled for now, super wonky
    e->current_page_tab->offset.y-=mousewheel*(entity_get_render_size(e->current_page_tab->cursor).y)*5;
    e->current_page_tab->wheel_override=true;
    */
}
void editor_handle_left_mouse_up(editor *e)
{
    e->current_page_tab->start_selection_mouse=false;
}
bool editor_handle_left_mouse_down(editor *e, vec2 mouse_position)
{
    for(s32 i=0; i<e->page_tabs_size; i++)
    {
        if(e->highest==e->page_tabs[i]->ent)
        {
            editor_set_current_page_tab(e,e->page_tabs[i]);
            return false;
        }
        if(e->highest==e->page_tabs[i]->close_button)
        {
            editor_close_tab(e,e->page_tabs[i]);
            return true;
        }
    }

    if(e->current_page_tab->current_text_selection)
    {
        dtor_text_selection(e);
    }
    
    /*
    MUST RESET TEXT_SELECTION_ORIGIN AND END BEFORE CALL TO SET_CURSOR_POSITION BECAUSE:
    IT WILL USE THAT TO CTOR THE NEXT TEXT_SELECTION ZZZ
    */
    vec2 cursor_position=editor_position_to_cursor(e,vec2_sub(mouse_position,e->current_page_tab->offset));
    e->current_page_tab->text_selection_origin=cursor_position;
    e->current_page_tab->text_selection_end=cursor_position;
    
    e->current_page_tab->current_text_selection=ctor_text_selection(mouse_position,mouse_position,e);
    
    editor_set_cursor_position(e, cursor_position.x, cursor_position.y);
    
    e->current_page_tab->start_selection_mouse=true;
    e->current_page_tab->start_selection_key=false;
    
    return false;
}
bool editor_process_keys(editor *e, event ev)
{
    /*if(e->highest != e->root)return false;*/

    bool key_is_already_pressed=false;
    u32 i;
    for(i=0; i<e->current_page_tab->keystate.pressed_keys_size; i++)
    {
        if(e->current_page_tab->keystate.pressed_keys[i]==ev.type)
        {
            key_is_already_pressed=true;
            break;
        }
    }
    
    bool repeat_key=false;
    if(key_is_already_pressed && ev.pressed)
    {
        if(e->current_page_tab->held_key==ev.type)
        {
            if(milli_current_time() - e->current_page_tab->held_key_time_stamp > 500)
            {
                repeat_key=true;
            }
        }
    }

    if(key_is_already_pressed && !ev.pressed)
    {
        /*start with index from previous for loop to remove unpressed key*/
        for(;i<e->current_page_tab->keystate.pressed_keys_size-1; i++)
        {
            e->current_page_tab->keystate.pressed_keys[i]=e->current_page_tab->keystate.pressed_keys[i+1];
        }
        e->current_page_tab->keystate.pressed_keys_size-=1;
        e->current_page_tab->keystate.pressed_keys=(s64*)realloc(e->current_page_tab->keystate.pressed_keys,sizeof(s64)*(e->current_page_tab->keystate.pressed_keys_size));
        e->current_page_tab->held_key=0;
    }
    
    if((!key_is_already_pressed && ev.pressed) ||  repeat_key)
    {
        if((!key_is_already_pressed && ev.pressed))
        {
            /*@todo add other keys that I don't want to be held down to this condition*/
            if(ev.type != KEY_CAPS_LOCK && ev.type != KEY_F11)
            {
                e->current_page_tab->held_key_time_stamp=milli_current_time();
                e->current_page_tab->held_key=ev.type;
            }
            
            e->current_page_tab->keystate.pressed_keys_size+=1;
            e->current_page_tab->keystate.pressed_keys=realloc(e->current_page_tab->keystate.pressed_keys,sizeof(s64)*(e->current_page_tab->keystate.pressed_keys_size));
            e->current_page_tab->keystate.pressed_keys[e->current_page_tab->keystate.pressed_keys_size-1]=ev.type;
        }
        if(ev.type != e->current_page_tab->held_key)
        {
            e->current_page_tab->held_key=0;
        }
            
        e->current_page_tab->keystate.ctrl_pressed=false;
        for(u32 i=0; i<e->current_page_tab->keystate.pressed_keys_size; i++)
        {
            if(e->current_page_tab->keystate.pressed_keys[i]==KEY_LEFT_CONTROL || e->current_page_tab->keystate.pressed_keys[i]==KEY_RIGHT_CONTROL
                || e->current_page_tab->keystate.pressed_keys[i]==KEY_LGUI || e->current_page_tab->keystate.pressed_keys[i]==KEY_RGUI)
            {
                e->current_page_tab->keystate.ctrl_pressed=true;
                break;
            }
        }
        
        return true;
    }
    
    return false;
}

#define d_delete_selection editor_delete_text(e, e->current_page_tab->text_selection_origin,e->current_page_tab->text_selection_end, true, ACTION_DELETE);\
dtor_text_selection(e);\
e->current_page_tab->current_text_selection=NULL;\
e->current_page_tab->start_selection_mouse=false;

        
#define d_shift_check if(e->current_page_tab->current_text_selection && !(get_mod_state() & KEY_MOD_SHIFT))\
{\
    dtor_text_selection(e);\
    e->current_page_tab->current_text_selection=NULL;\
    e->current_page_tab->start_selection_mouse=false;\
    e->current_page_tab->start_selection_key=false;\
}

static void on_key_page_down(editor *e)
{
    d_shift_check                  
    editor_set_cursor_position(e, e->current_page_tab->cursor_x, e->current_page_tab->cursor_y+LINES_ON_PAGE_DOWNUP);
}
static void on_key_page_up(editor *e)
{
    d_shift_check
    editor_set_cursor_position(e, e->current_page_tab->cursor_x, e->current_page_tab->cursor_y-LINES_ON_PAGE_DOWNUP);
}
static void on_key_home(editor *e)
{

    d_shift_check
    char *curline=editor_get_line(e,e->current_page_tab->cursor_y);
    u32 last_character=0;

    if(strlen(curline))
    {
        for(s32 i=e->current_page_tab->cursor_x-1; i>=0; i--)
        {
            if(curline[i] != ' ')
            {
                last_character=i;
            }
        }
    }

    editor_set_cursor_position(e, last_character, e->current_page_tab->cursor_y);
}
static void on_key_end(editor *e)
{
    d_shift_check
    
    char *curline=editor_get_line(e,e->current_page_tab->cursor_y);
    u32 last_character=0;
    for(u32 i=e->current_page_tab->cursor_x; i<strlen(curline); i++)
    {
        if(curline[i]!=' ')
        {
            last_character=i+1;
        }
    }   

    if(!last_character)
    {
        last_character=strlen(editor_get_line(e,e->current_page_tab->cursor_y));
    }

    editor_set_cursor_position(e, last_character, e->current_page_tab->cursor_y);
}
static void svil(vec2 *begin, vec2 *end){
    vec2 temp;
    if(begin->y > end->y){
        temp=*end;
        *end=*begin;
        *begin=temp;
    }
    else if(begin->x > end->x){
        temp=*end;
        *end=*begin;
        *begin=temp;
    }
}
static void copy_selection(editor *e){
    printf("end:%f,%f\n",e->current_page_tab->text_selection_end.x,e->current_page_tab->text_selection_end.y);
    printf("origin:%f,%f\n",e->current_page_tab->text_selection_origin.x,e->current_page_tab->text_selection_origin.y);
    
    vec2 begin=e->current_page_tab->text_selection_origin;
    vec2 end=e->current_page_tab->text_selection_end;
    svil(&begin,&end);
    
    end.x--;

    set_clipboard_text(editor_get_text(e,begin,end));
}
s32 editor_handle_keys(editor *e, event ev)
{
    s32 exit_code=0;
    /*
    if(e->highest != e->root)return exit_code;
    */   
    {/*global key handling*/
        if(ev.type==KEY_F10)
        {
            /*global_text_margin=global_text_margin?0:1;
            
            text_block_renderer_set_text(e->current_page_tab->tbr,e->current_page_tab->lines,
                e->current_page_tab->lines_size,e->current_page_tab->font_color,0,e->current_page_tab->lines_size-1,e->current_page_tab->ext);
            */
        }
        else if(ev.type==KEY_F11)
        {
            e->is_fullscreen=!e->is_fullscreen;
            window_toggle_fullscreen(e->win,e->is_fullscreen);
        }
    }

    if(e->mode){/*shell key handling*/
        if(ev.type==KEY_ESCAPE){
            e->mode=!e->mode;
            free(e->shell_text);
            e->shell_text=malloc(1);
            e->shell_text[0]=0;
            e->shell_pos=0;
            return exit_code;
        }
        else if(ev.type==KEY_DELETE)
        {
            if(e->shell_pos<strlen(e->shell_text)-1 && strlen(e->shell_text)){
                e->shell_text=str_remove(e->shell_text,e->shell_pos+1);
            }
        }
        else if(ev.type==KEY_ENTER)
        {
            if(shell_execute(e,e->shell_text)){
                /*disable clearing on enter
                e->mode=!e->mode;
                free(e->shell_text);
                e->shell_text=malloc(1);
                e->shell_text[0]=0;
                e->shell_pos=0;
                */
            }
        }
        else if(ev.type==KEY_BACKSPACE)
        {
            if(e->shell_pos>0 && strlen(e->shell_text)){
                e->shell_text=str_remove(e->shell_text,e->shell_pos-1);
                e->shell_pos--;
            }
        }
        else if(ev.type >=KEY_SPACE && ev.type <= KEY_Z)
        {
            char char_to_insert=ev.type;

            e->current_page_tab->keystate.shift_pressed=false;
            for(u32 i=0; i<e->current_page_tab->keystate.pressed_keys_size; i++)
            {
                if(e->current_page_tab->keystate.pressed_keys[i]==KEY_LEFT_SHIFT || e->current_page_tab->keystate.pressed_keys[i]==KEY_RIGHT_SHIFT)
                {
                    e->current_page_tab->keystate.shift_pressed=true;
                    break;
                }
            }
            
            if(e->current_page_tab->keystate.shift_pressed)
            {
                char_to_insert=apply_shift(char_to_insert, true);
            }
            else if(get_mod_state() & KEY_MOD_CAPS)
            {
                char_to_insert=apply_shift(char_to_insert, false);
            }

            e->shell_text=str_insert(e->shell_text,char_to_insert,e->shell_pos);
            e->shell_pos++;
        }

        if(ev.type==KEY_LEFT)
        {
            if(e->shell_pos>0){
                e->shell_pos--;
            }
        }
        else if(ev.type==KEY_RIGHT)
        {                 
            if(e->shell_pos<strlen(e->shell_text)){
                e->shell_pos++;
            }
        }
    }
    else{/*normal editor key handling*/
        /*ACTUAL KEY HANDLING STARTS HERE*/
        if(e->current_page_tab->keystate.ctrl_pressed)
        {
            /*@todo make these rebindable*/
            
            /*open*/
            if(ev.type==KEY_O)
            {

            }
            /*save*/
            if(ev.type==KEY_S)
            {
                char *str=malloc(1);
                str[0]=0;
                char *newlinestr=malloc(2);
                newlinestr[0]='\n';
                newlinestr[1]=0;
                for(u32 i=0; i<e->current_page_tab->lines_size; i++)
                {
                    char *temp=str;
                    str=str_cat(str,e->current_page_tab->lines[i]);
                    free(temp);
                    if(i<e->current_page_tab->lines_size-1)
                    {
                        temp=str;
                        str=str_cat(str,newlinestr);
                        free(temp);
                    }
                }
                str_to_file(e->current_page_tab->file_path,str);
                free(str);

                //what should we do if we have no actions and they save? how to track action_list_index? need to at all?
                if(e->current_page_tab->action_list_size && e->current_page_tab->action_list_index>0)
                {
                    e->current_page_tab->action_on_save=e->current_page_tab->action_list[e->current_page_tab->action_list_index]->uid;
                    image_renderer_set_texture((image_renderer*)e->current_page_tab->ent_renderer,e->current_page_tab->tab_color_good);
                }
            }
            /*find, one for just window if pressing alt for all open windows*/
            if(ev.type==KEY_F)
            {

            }
            /*replace, one for just window if pressing alt for all open windows*/
            if(ev.type==KEY_H)
            {

            }
            /*move to home*/
            if(ev.type==KEY_LEFT)
            {
                on_key_home(e);
            }
            /*move to end*/
            if(ev.type==KEY_RIGHT)
            {
                on_key_end(e);
            }
            /*move to pageup*/
            if(ev.type==KEY_UP)
            {
                on_key_page_up(e);
            }
            /*move to pagedown*/
            if(ev.type==KEY_DOWN)
            {
                on_key_page_down(e);
            }
            /*select all*/
            if(ev.type==KEY_A)
            {
                if(!e->current_page_tab->current_text_selection)
                {
                    vec2 begin=value_vec2(0,0);
                    vec2 end=value_vec2(strlen(editor_get_line(e,e->current_page_tab->lines_size-1)),e->current_page_tab->lines_size-1);

                    e->current_page_tab->current_text_selection=ctor_text_selection(begin, end, e);
                    e->current_page_tab->text_selection_origin=begin;
                    e->current_page_tab->text_selection_end=end;
                }
            }
            /*open new window*/
            if(ev.type==KEY_N)
            {
                /*disable multiple editors for now - is buggy and probably not needed
                exit_code=REQUEST_SPAWN_EDITOR;*/
            }
            if(ev.type==KEY_C)
            {
                if(e->current_page_tab->current_text_selection)
                {
                    copy_selection(e);
                }
            }
            if(ev.type==KEY_Z)
            {
                if(get_mod_state() & KEY_MOD_SHIFT){
                    editor_do_action(e,false);
                }
                else{
                    editor_do_action(e,true);
                }
            }
            if(ev.type==KEY_Y)
            {
                editor_do_action(e,false);
            }
            if(ev.type==KEY_V)
            {
                char *sdl_clip=get_clipboard_text();
                char *clip=strcpy(malloc(strlen(sdl_clip)+1),sdl_clip);
                system_free(sdl_clip);
                if(clip)
                {
                    editor_add_text(e,clip,true);
                }
            }
            if(ev.type==KEY_X)
            {
                if(e->current_page_tab->current_text_selection)
                {   
                    copy_selection(e);
                    d_delete_selection
                }
            }
        }
        else/*NON CONTROL KEY HANDLING, TEXT SELECTION DELETING KEYS*/
        {   
            if(ev.type==KEY_ESCAPE){
                e->mode=1;
                return exit_code;
            }
            else if(ev.type==KEY_DELETE)
            {
                if(e->current_page_tab->current_text_selection)
                {
                    d_delete_selection
                }
                else
                {
                    vec2 begin;
                    begin.x=e->current_page_tab->cursor_x;
                    begin.y=e->current_page_tab->cursor_y;
                    vec2 end;
                    end.x=e->current_page_tab->cursor_x+1;
                    end.y=e->current_page_tab->cursor_y;

                    
                    if(end.x>strlen(editor_get_line(e,e->current_page_tab->cursor_y)))
                    {                                    
                        if(e->current_page_tab->cursor_y<e->current_page_tab->lines_size-1)
                        {
                            end.x=0;
                            end.y++;    
                            editor_delete_text(e,begin,end,true, ACTION_DELETE);
                        }
                    }
                    else
                    {
                        editor_delete_text(e,begin,end,true,ACTION_DELETE);
                    }
                }
            }
            else if(ev.type==KEY_ENTER)
            {
                if(e->current_page_tab->current_text_selection)
                {
                    d_delete_selection
                }
                
                char *indent_str=strcpy(malloc(2),"\n");
                if(AUTO_INDENT)
                {
                    s32 indent_str_size=2;
                    u32 i;
                    
                    s32 x=e->current_page_tab->cursor_x;
                    
                    char *editor_line=editor_get_line(e,e->current_page_tab->cursor_y);
                    
                    for(int i=0; editor_line[i]; i++){
                        if(editor_line[i]!=' '){
                            /*subtract difference between it and cursor position, 
                            so that it stays at the same indentation level (even if the string it is pushing to the next line (after the cursor) contains spaces!)
                            */
                            if(i>x){
                                x=i-(i-e->current_page_tab->cursor_x);   
                            }
                            else{
                                x=i;
                            }
                            break;
                        }        
                    }                
                
                    for(i=0;i<x;i++)
                    {                            
                        indent_str_size+=1;
                        indent_str=realloc(indent_str,indent_str_size);
                        indent_str[indent_str_size-2]=' ';
                    }
                
                    indent_str[indent_str_size-1]=0; 
                }
                editor_add_text(e, indent_str, true);                            
            }
            else if(ev.type==KEY_BACKSPACE)
            {
                if(e->current_page_tab->current_text_selection)
                {
                    d_delete_selection
                }
                else
                {
                    vec2 begin;
                    begin.x=((s32)e->current_page_tab->cursor_x)-1;
                    begin.y=e->current_page_tab->cursor_y;
                    vec2 end;
                    end.x=e->current_page_tab->cursor_x;
                    end.y=e->current_page_tab->cursor_y;

                        
                    if(begin.x<0)
                    {                           
                        if(e->current_page_tab->cursor_y>0)
                        {
                            begin.x=strlen(editor_get_line(e,e->current_page_tab->cursor_y-1));
                            begin.y--;    
                            editor_delete_text(e,begin,end,true,ACTION_DELETE);
                        }
                    }
                    else
                    {
                        editor_delete_text(e,begin,end,true,ACTION_BACKSPACE);
                    }
                    
                }
            }
            else if(ev.type >= KEY_SPACE && ev.type <= KEY_Z)
            {
                if(e->current_page_tab->current_text_selection)
                {
                    d_delete_selection
                }

                char *curline=editor_get_line(e,e->current_page_tab->cursor_y);
                char char_to_insert=ev.type;
                
                e->current_page_tab->keystate.shift_pressed=false;
                for(u32 i=0; i<e->current_page_tab->keystate.pressed_keys_size; i++)
                {
                    if(e->current_page_tab->keystate.pressed_keys[i]==KEY_LEFT_SHIFT || e->current_page_tab->keystate.pressed_keys[i]==KEY_RIGHT_SHIFT)
                    {
                        e->current_page_tab->keystate.shift_pressed=true;
                        break;
                    }
                }
                
                if(e->current_page_tab->keystate.shift_pressed)
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
                editor_add_text(e,str,true);
                
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

                            char *editor_line=editor_get_line(e,e->current_page_tab->cursor_y);
                            s32 indent_level=indentation_level_spaces(editor_line);
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
                            
                            editor_add_text(e,indent_str,true);
                            editor_set_cursor_position(e, strlen(editor_get_line(e,e->current_page_tab->cursor_y-1)), e->current_page_tab->cursor_y-1);
                        }
                        else
                        {
                            char *str=strcpy(malloc(2),"}");
                            editor_add_text(e,str,true);
                            editor_set_cursor_position(e, e->current_page_tab->cursor_x-1, e->current_page_tab->cursor_y);
                        }
                    }
                }
            }
            else /*TEXT_SELECTION CLOSING KEYS IF SHIFT ISNT PRESSED*/
            {            
                
                if(!e->current_page_tab->current_text_selection && (get_mod_state() & KEY_MOD_SHIFT))
                {
                    if(e->current_page_tab->current_text_selection)
                    {
                        dtor_text_selection(e);
                    }
                                
                    vec2 cpos=value_vec2(e->current_page_tab->cursor_x,e->current_page_tab->cursor_y);
                    e->current_page_tab->text_selection_origin=cpos;
                    e->current_page_tab->text_selection_end=cpos;             
                    e->current_page_tab->current_text_selection=ctor_text_selection(cpos,cpos,e);
                    
                    e->current_page_tab->start_selection_mouse=false;
                    e->current_page_tab->start_selection_key=true;
                }
                
                if(ev.type==KEY_END)
                {
                    on_key_end(e);
                }
                else if(ev.type==KEY_HOME)
                {
                    on_key_home(e);
                }
                else if(ev.type==KEY_PAGE_DOWN)
                {      
                    on_key_page_down(e);
                }
                else if(ev.type==KEY_PAGE_UP)
                {
                    on_key_page_up(e);
                }             
                else if(ev.type==KEY_TAB && !(get_mod_state() & KEY_MOD_ALT) && !(get_mod_state() & KEY_MOD_GUI))/*last two conditions are to ensure that this tab event wasnt caused by tabbing into the window*/
                {
                    char *str=strcpy(malloc(5),"    ");
                    editor_add_text(e, str,true);
                }
                else if(ev.type==KEY_LEFT)
                {
                    d_shift_check
                    if(e->current_page_tab->cursor_x)
                    {
                        editor_set_cursor_position(e,e->current_page_tab->cursor_x - 1, e->current_page_tab->cursor_y);
                    }
                    else
                    {
                        if(e->current_page_tab->cursor_y>0)
                        {
                            u32 ypos=e->current_page_tab->cursor_y-1;
                            editor_set_cursor_position(e,strlen(editor_get_line(e,ypos)), e->current_page_tab->cursor_y-1);
                        }
                    }
                    
                        
                }
                else if(ev.type==KEY_RIGHT)
                {
                    d_shift_check
                    if(e->current_page_tab->cursor_x < strlen(editor_get_line(e,e->current_page_tab->cursor_y)))
                    {
                        editor_set_cursor_position(e,e->current_page_tab->cursor_x + 1, e->current_page_tab->cursor_y);
                    }
                    else
                    {
                        if(e->current_page_tab->cursor_y<e->current_page_tab->lines_size-1)
                        {
                            u32 ypos=e->current_page_tab->cursor_y+1;
                            editor_set_cursor_position(e,0, e->current_page_tab->cursor_y+1);
                        }
                    }                         
                }
                else if(ev.type==KEY_UP)
                {
                    d_shift_check
                    if(e->current_page_tab->cursor_y>0)
                    {
                        if(e->current_page_tab->cursor_x>=strlen(editor_get_line(e,e->current_page_tab->cursor_y-1)))
                        {
                            e->current_page_tab->cursor_x=strlen(editor_get_line(e,e->current_page_tab->cursor_y-1));
                        }
                        editor_set_cursor_position(e,e->current_page_tab->cursor_x, e->current_page_tab->cursor_y - 1);
                    }
                    
                        
                }
                else if(ev.type==KEY_DOWN)
                {
                    d_shift_check
                    if(e->current_page_tab->cursor_y<e->current_page_tab->lines_size-1)
                    {
                        
                        if(e->current_page_tab->cursor_x>=strlen(editor_get_line(e,e->current_page_tab->cursor_y+1)))
                        {
                            e->current_page_tab->cursor_x=strlen(editor_get_line(e,e->current_page_tab->cursor_y+1));
                        }
                        editor_set_cursor_position(e,e->current_page_tab->cursor_x, e->current_page_tab->cursor_y + 1);
                    }                         
                }
            }
        }
        
        e->current_page_tab->cursor_blink_state=true;
        entity_set_visible(e->current_page_tab->cursor,e->current_page_tab->cursor_blink_state);
        e->current_page_tab->cursor_blink_timer=0;
        
    }
   
    return exit_code;
}
/*
if you pass the end of one line and the beginning of the next line it concats them

*/
void editor_delete_text(editor *focused_editor, vec2 begin, vec2 end, bool do_add_action, u32 type)
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
        char *curline=editor_get_line(focused_editor,y);

        if(!curline){
            continue;
        }

        if(begin.y==end.y)
        {
            char *first=alloc_str_slice(curline, 0, begin.x-1);
            
            char *second=alloc_str_slice(curline, end.x, strlen(curline)-1);

            if(do_add_action)
            {
                built_string=alloc_str_slice(curline,begin.x,end.x-1);
            }

            editor_set_line(focused_editor,y,str_cat(first,second));

            free(first);
            free(second);
            free(curline);
            break;
        }
        else if(y==begin.y)
        {
            first_line=alloc_str_slice(curline,0,begin.x-1);
            if(do_add_action)
            {
                built_string=alloc_str_slice(curline,begin.x,strlen(curline)-1);
            }
        }
        else if(y==end.y)
        {
            //breakpoint set --file editor.c --line 1372
            u32 i;
            last_line=alloc_str_slice(curline,end.x,strlen(curline)-1);

            if(do_add_action)
            {
                char *temp=built_string;
                built_string=str_cat(built_string,"\n");
                free(temp);
                temp=built_string;
                built_string=str_cat(built_string,alloc_str_slice(curline,0,end.x-1));
                free(temp);
            }

            editor_set_line(focused_editor,y,NULL);
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
                built_string=str_cat(built_string,editor_get_line(focused_editor,y));
                free(temp);
            }
            editor_set_line(focused_editor,y,NULL);
            //free(curline);
        }
        
    }
    for(s32 i=0; i<editor_get_lines_size(focused_editor); i++)
    {
        if(!editor_get_line(focused_editor,i))
        {
            editor_remove_line(focused_editor,i);
            text_block_renderer_remove_lines(focused_editor->current_page_tab->tbr,i,1);
            i--;
        }
    }
    if(first_line && last_line)
    {
        free(editor_get_line(focused_editor,(int)begin.y));
        editor_set_line(focused_editor,(int)begin.y,str_cat(first_line,last_line));
        free(first_line);
        free(last_line);
    }
    
    if(do_add_action)
    {
        editor_add_action(focused_editor,ctor_action(begin, end, built_string, type));
    }
    editor_set_cursor_position(focused_editor, begin.x, begin.y);
    text_block_renderer_set_text(focused_editor->current_page_tab->tbr,focused_editor->current_page_tab->lines,
        focused_editor->current_page_tab->lines_size,begin.y,end.y,focused_editor->current_page_tab->ext);


}
/*
@todo change to pass position explicitly
it takes ownership of clip, by passing to action
dont free clip, it is passed to an action which frees it
dont reference clip after passing it
*/
void editor_add_text(editor *focused_editor, char *clip, bool do_add_action)
{
    clip=str_remove_characters(clip,'\r');
    
    if(focused_editor->current_page_tab->current_text_selection)
    {
        editor_delete_text(focused_editor, focused_editor->current_page_tab->text_selection_origin, focused_editor->current_page_tab->text_selection_end,true,ACTION_DELETE);
        dtor_text_selection(focused_editor);
        focused_editor->current_page_tab->current_text_selection=NULL;
        focused_editor->current_page_tab->start_selection_mouse=false;
    }

    u32 clip_index=0;
    u32 strlen_clip=strlen(clip);

    vec2 cursor_position_start;
    cursor_position_start.x=focused_editor->current_page_tab->cursor_x;
    cursor_position_start.y=focused_editor->current_page_tab->cursor_y;

    while(clip_index<strlen_clip)
    {   
        if(clip[clip_index]=='\n')
        {                                 
            u32 next_line_splice_start=focused_editor->current_page_tab->cursor_x;
            u32 next_line_splice_end=strlen(editor_get_line(focused_editor,focused_editor->current_page_tab->cursor_y))-1;
            text_block_renderer_add_lines(focused_editor->current_page_tab->tbr,focused_editor->current_page_tab->cursor_y,1);

            editor_insert_line(focused_editor,focused_editor->current_page_tab->cursor_y+1,alloc_str_slice(editor_get_line(focused_editor,focused_editor->current_page_tab->cursor_y),next_line_splice_start,next_line_splice_end));

            char *temp=editor_get_line(focused_editor,focused_editor->current_page_tab->cursor_y);
            editor_set_line(focused_editor,focused_editor->current_page_tab->cursor_y,alloc_str_slice(temp,0,focused_editor->current_page_tab->cursor_x-1));
            free(temp);

            editor_set_cursor_position(focused_editor,0,focused_editor->current_page_tab->cursor_y+1);
        }
        else if(clip[clip_index]=='\t')
        {
            for(u32 i=0; i<num_spaces_to_insert_on_tab; i++)
            {
                char *curline=editor_get_line(focused_editor,focused_editor->current_page_tab->cursor_y);
                char *modstring=str_insert(curline,' ',focused_editor->current_page_tab->cursor_x);//@leak?
                editor_set_line(focused_editor,focused_editor->current_page_tab->cursor_y,modstring);
                editor_set_cursor_position(focused_editor,focused_editor->current_page_tab->cursor_x+1,focused_editor->current_page_tab->cursor_y);
            }
        }
        else
        {
            char *curline=editor_get_line(focused_editor,focused_editor->current_page_tab->cursor_y);
            char *modstring=str_insert(curline,clip[clip_index],focused_editor->current_page_tab->cursor_x);//@leak?
            editor_set_line(focused_editor,focused_editor->current_page_tab->cursor_y,modstring);
            editor_set_cursor_position(focused_editor,focused_editor->current_page_tab->cursor_x+1,focused_editor->current_page_tab->cursor_y);
        }
        clip_index++;
    }
    if(do_add_action)
    {
        editor_add_action(focused_editor,ctor_action(cursor_position_start, value_vec2(0,0), clip, ACTION_TEXT));
    }
    
    text_block_renderer_set_text(focused_editor->current_page_tab->tbr,focused_editor->current_page_tab->lines,
        focused_editor->current_page_tab->lines_size,cursor_position_start.y,focused_editor->current_page_tab->cursor_y,focused_editor->current_page_tab->ext);
}


vec2 editor_position_to_cursor(editor *focused_editor, vec2 mouse_position)
{
    s32 i;
    s32 w;
    s32 h;
    
    size_ttf_font(focused_editor->current_page_tab->font,editor_get_line(focused_editor,0),&w,&h);

    s32 cursor_x=0;
    s32 cursor_y=mouse_position.y;
    
    s32 mpos_x=mouse_position.x;

    if(!h){
        size_ttf_font(focused_editor->current_page_tab->font,"_",&w,&h);
        w=0;
        /*printf("editor.c line 1546 !h: %d\n",h);*/
    }
    
    cursor_y/=h;

    if(cursor_y<0)cursor_y=0;
    
    if(mpos_x<0)
    {
        cursor_x=0;
    }
    else if(cursor_y<focused_editor->current_page_tab->lines_size)
    {
        char *curline=editor_get_line(focused_editor,cursor_y);

        u32 row_strlen=strlen(curline);
        for(i=0; i<row_strlen; i++)
        {
            char *slice=alloc_str_slice(curline, 0, i);
            size_ttf_font(focused_editor->current_page_tab->font,slice,&w,&h);
            free(slice);
            w+=global_font_size/4;
            if(w>mpos_x)
            {
                if(i)
                {
                    s32 dist2=0;/*distance to the character that passes the mouse*/
                    s32 dist=0;/*distance to the previous character to dist2*/

                    char *slice2=alloc_str_slice(curline,0,i-1);
                    size_ttf_font(focused_editor->current_page_tab->font,slice2,&dist,0);
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
    
    if(cursor_y>=focused_editor->current_page_tab->lines_size)
    {
        cursor_y=focused_editor->current_page_tab->lines_size-1;
        cursor_x=strlen(editor_get_line(focused_editor,cursor_y));
    }

    return value_vec2(cursor_x,cursor_y);
}
char *editor_get_text(editor *focused_editor,vec2 begin, vec2 end)
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
        char *curline=editor_get_line(focused_editor,y);
        u32 strlen_curline=strlen(curline);
        
        if(begin.y==end.y)
        {
            retval=alloc_str_slice(curline,begin.x,end.x);
            break;
        }
        else if(y==begin.y)
        {
            retval=alloc_str_slice(curline,begin.x,strlen_curline-1);
        }
        else if(y==end.y)
        {
            temp=retval;
            retval=str_cat(retval,"\n");
            free(temp);
            if(end.x>=0){
                temp=retval;
                retval=str_cat(retval,alloc_str_slice(curline,0,end.x));            
                free(temp);
            }
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


void editor_add_action(editor *e, action *a)
{   
    sprintf(ote_log_buffer,"(ENTRY) add_action - e->current_page_tab->action_list_size: %d\n",e->current_page_tab->action_list_size);
    ote_log(ote_log_buffer,LOG_TYPE_ACTION);
    sprintf(ote_log_buffer,"(ENTRY) add_action - action_list_index: %d\n",e->current_page_tab->action_list_index);
    ote_log(ote_log_buffer,LOG_TYPE_ACTION);
    /*@todo delete all actions after the index*/
    if(e->current_page_tab->action_list_index < e->current_page_tab->action_list_size-1)
    {
        for(u32 i=e->current_page_tab->action_list_index+1; i<e->current_page_tab->action_list_size; i++)
        {
            if(e->current_page_tab->action_list[i])
            {
                dtor_action(e->current_page_tab->action_list[i]);
            }
            e->current_page_tab->action_list[i]=NULL;
        }
        ote_log("CUTTING OFF HEAD\n",LOG_TYPE_ACTION);

        e->current_page_tab->action_list_size=e->current_page_tab->action_list_index+1;
        e->current_page_tab->action_list_size++;
        e->current_page_tab->action_list=realloc(e->current_page_tab->action_list, e->current_page_tab->action_list_size*sizeof(action*));
        
        e->current_page_tab->action_list_index=e->current_page_tab->action_list_size-1;
        e->current_page_tab->action_list[e->current_page_tab->action_list_index]=a;
    }
    else
    {
        e->current_page_tab->action_list_size++;
        e->current_page_tab->action_list=realloc(e->current_page_tab->action_list, e->current_page_tab->action_list_size*sizeof(action*));
        e->current_page_tab->action_list[e->current_page_tab->action_list_size-1]=a;    
        e->current_page_tab->action_list_index=e->current_page_tab->action_list_size-1;    
    }
    sprintf(ote_log_buffer,"(EXIT) add_action - e->current_page_tab->action_list_size: %d\n",e->current_page_tab->action_list_size);
    ote_log(ote_log_buffer,LOG_TYPE_ACTION);
    sprintf(ote_log_buffer,"(EXIT) add_action - action_list_index: %d\n",e->current_page_tab->action_list_index);
    ote_log(ote_log_buffer,LOG_TYPE_ACTION);
}
void editor_do_action(editor *e, bool un)
{
    sprintf(ote_log_buffer,"do_action - (action_list_index,action_list_size): %d,%d\n",e->current_page_tab->action_list_index,e->current_page_tab->action_list_size);
    printf(ote_log_buffer);
    action *a=NULL;

    if(e->current_page_tab->current_text_selection){
        dtor_text_selection(e);
        e->current_page_tab->current_text_selection=NULL;
        e->current_page_tab->start_selection_mouse=false;
    }

    if(un)
    {
        if(!e->current_page_tab->action_list_size || e->current_page_tab->action_list_index < 0)
        {
            return;
        }
        a = e->current_page_tab->action_list[e->current_page_tab->action_list_index];
        
        vec2 cursor_pos=e->current_page_tab->action_list[e->current_page_tab->action_list_index]->cursor_position;
        editor_set_cursor_position(e, cursor_pos.x, cursor_pos.y);

        if(a->type==ACTION_TEXT)
        {
            if(e->current_page_tab->action_list_index >=0)
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
                
                editor_delete_text(e,cursor_pos, end, false, ACTION_DELETE);

                e->current_page_tab->action_list_index--;
            }
        }
        else if(a->type==ACTION_BACKSPACE)
        {
            /*
            for these that use a->text copy the action string - dont want it to get freed in the add_text function
            */
            editor_add_text(e,strcpy(malloc(strlen(a->text)+1),a->text),false);
            e->current_page_tab->action_list_index--;
        }
        else if(a->type==ACTION_DELETE)
        {
            editor_add_text(e,strcpy(malloc(strlen(a->text)+1),a->text),false);
            e->current_page_tab->action_list_index--;
            editor_set_cursor_position(e, cursor_pos.x, cursor_pos.y);
        }
    }
    else
    {/*redo*/
        if(e->current_page_tab->action_list_index+1 > e->current_page_tab->action_list_size-1)
        {
            return;
        }

        e->current_page_tab->action_list_index++;
        a = e->current_page_tab->action_list[e->current_page_tab->action_list_index];
        vec2 cursor_pos=e->current_page_tab->action_list[e->current_page_tab->action_list_index]->cursor_position;
        editor_set_cursor_position(e, cursor_pos.x, cursor_pos.y);
        
        if(a->type==ACTION_TEXT)
        {
            editor_add_text(e, strcpy(malloc(strlen(a->text)+1),a->text), false);
        }
        else if(a->type==ACTION_DELETE)
        {
            editor_delete_text(e, a->cursor_position, a->optional_end_position, false, ACTION_DELETE);
            editor_set_cursor_position(e, cursor_pos.x, cursor_pos.y);
        }
        else if(a->type==ACTION_BACKSPACE)
        {
            editor_delete_text(e, a->cursor_position, a->optional_end_position, false, ACTION_DELETE);
        }
    }
}
