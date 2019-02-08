#ifndef SHELL_H
#define SHELL_H

#include "editor.h"

/*return true on success*/
bool shell_execute(editor *pt, char *cmd);

#endif