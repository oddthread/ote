#include "../h/shell.h"

static bool shell_replace(editor *e, char *old, char *new){
    printf("replace: %s,%s\n",old,new);
    if(!old || !new || !strlen(old) || !strlen(new)){
        return false;
    }
    return editor_replace(e,old,new);
}
static bool shell_replace_all(editor *e, char *old, char *new){
    printf("replaceall: %s,%s\n",old,new);
    if(!old || !new || !strlen(old) || !strlen(new)){
        return false;
    }
    return editor_replace_all(e,old,new);
}
static bool shell_find(editor *e, char *str){
    printf("find: %s\n",str);
    if(!str || !strlen(str)){
        return false;
    }
    
    return editor_find(e,str);
}
static bool shell_goto(editor *e, int line_index){
    printf("goto: %d\n",line_index);
    editor_set_cursor_position(e,0,line_index);
    return true;
}

typedef struct token{
    char *token;
    int end;
} token;

char *remove_escapes(char *data, int begin, int end){
    int i,j;
    char *retval;
    int newlen;

    /*find characters to escape to get size of retval*/
    int escape=0;
    for(i=begin; i<=end; i++){
        if(data[i]=='\\')escape++;
    }
    
    /*alloc*/
    newlen=(end-begin+1)+1-escape;
    retval=malloc(newlen);
    retval[newlen-1]=0;
    
    /*copy*/
    for(i=begin, j=0; i<=end; i++){
        if(data[i]!='\\'){
            retval[j]=data[i];
            j++;
        }
    }

    printf("remove escapes returning: %s\n",retval);
    return retval;
}
token get_next_token(char *data, int index, char const *sep){
    /*
    separators dont apply to escaped characters
    also new token at end of string
    */
    int token_begin=-1;
    token retval;
    retval.end=-1;
    retval.token=NULL;

    for(;;index++){
        
        if(token_begin==-1){
            if(!str_contains(sep,data[index])){
                /*quotes/escaping*/
                if(data[index]=='"' || data[index]=='\''){
                    char quote=data[index];
                    index++;
                    token_begin=index;
                    for(;;index++){
                        if(!data[index]){
                            printf("get_next_token reached end of string while looking for %c\n",quote);
                            return retval;
                        }
                        if(data[index]==quote){
                            if(data[index-1]!='\\'){
                                retval.end=index+1;
                                retval.token=remove_escapes(data,token_begin,index-1);
                                return retval;
                            }
                        }
                    }
                }
                else{
                    token_begin=index;
                }
            }
        }
        else{
            if(str_contains(sep,data[index])){
                retval.end=index;
                retval.token=alloc_str_slice(data,token_begin,index-1);
                break;
            }
        }

        if(!data[index]){
            if(token_begin!=-1 && token_begin != index){
                retval.end=index;
                retval.token=alloc_str_slice(data,token_begin,index-1);
            }
            break;
        }
    }
    return retval;
}
/*
return true on success
check page_tab is valid,
parse command,
return false if invalid,
otherwise execute
*/
bool shell_execute(editor *e, char *cmd){
    if(!cmd || !strlen(cmd)){
        printf("shell_execute empty command, do nothing\n");
        return true;
    }
    printf("got shell_execute cmd: %s\n",cmd);
    char const *sep=", ";

    bool status=false;

    int index=0;
    token name=get_next_token(cmd,index,sep);
    
    if(str_eq(name.token,"exit")){
        exit(0);
    }
 
    if(str_eq(name.token,"fullscreen")){
        window_toggle_fullscreen(editor_get_window(e),true);
        free(name.token);
        return true;
    }
    
    if(str_eq(name.token,"unfullscreen")){
        free(name.token);
        window_toggle_fullscreen(editor_get_window(e),false);
        return true;
    }
    
     
    if(str_eq(name.token,"find")){
        token p1=get_next_token(cmd,name.end,sep);

        printf("find p1: %s\n",p1.token);
        char *extra_token=get_next_token(cmd,p1.end,sep).token;
        if(extra_token){
            printf("find returning, extra token(s): %s,%d\n",extra_token,strlen(extra_token));
            free(extra_token);
            free(p1.token);
            free(name.token);
            return false;
        }

        status=shell_find(e,p1.token);
        free(name.token);
        free(p1.token);
    }
    if(str_eq(name.token,"replace")){
        token p1=get_next_token(cmd,name.end,sep);
        token p2=get_next_token(cmd,p1.end,sep);

        char *extra_token=get_next_token(cmd,p2.end,sep).token;
        if(extra_token){
            
            printf("find returning, extra token(s): %s,%d\n",extra_token,strlen(extra_token));
            free(extra_token);
            free(p1.token);
            free(p2.token);
            free(name.token);
            return false;
        }
        
        status=shell_replace(e,p1.token,p2.token);
        free(p1.token);
        free(name.token);
        free(p2.token);
    }
    if(str_eq(name.token,"goto")){
        token p1=get_next_token(cmd,name.end,sep);
        
        char *extra_token=get_next_token(cmd,p1.end,sep).token;
        if(extra_token){
            free(extra_token);
            free(p1.token);
            free(name.token);
            return false;
        }

        int gotoval=atoi(p1.token);

        status=shell_goto(e,gotoval-1);
        free(p1.token);
        free(name.token);
    }
    if(str_eq(name.token,"replaceall")){
        token p1=get_next_token(cmd,name.end,sep);
        token p2=get_next_token(cmd,p1.end,sep);

        char *extra_token=get_next_token(cmd,p2.end,sep).token;
        if(extra_token){
            free(extra_token);
            free(p1.token);
            free(p2.token);
            free(name.token);
            return false;
        }
        
        status=shell_replace_all(e,p1.token,p2.token);
        free(p1.token);
        free(name.token);
        free(p2.token);
    }

    return status;
}