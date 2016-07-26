#ifndef LIB_H
#define LIB_H

#include <lua.hpp>
#include "common.h"

using LuaState = lua_State*;
using LibraryType = const luaL_Reg[];

bool InitLua();

void stackdump();

#endif