#ifndef _LIBRARY_SCRIPT_H_
#define _LIBRARY_SCRIPT_H_

#include <lua.h>

#include "library.h"

extern int script_global_init(lua_State* lua);
extern int script_load_config(lua_State* lua, library_conf_t* conf, const char* file_path);

#endif

