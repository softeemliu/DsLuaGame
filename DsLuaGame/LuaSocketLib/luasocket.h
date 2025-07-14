#ifndef LUASOCKET_H
#define LUASOCKET_H
/*=========================================================================*\
* LuaSocket toolkit
* Networking support for the Lua language
* Diego Nehab
* 9/11/1999
\*=========================================================================*/

/*-------------------------------------------------------------------------* \
* Current socket library version
\*-------------------------------------------------------------------------*/
#define LUASOCKET_VERSION    "LuaSocket 3.1.0"
#define LUASOCKET_COPYRIGHT  "Copyright (C) 1999-2013 Diego Nehab"


#include "lua.h"
#include "lauxlib.h"
#include "compat.h"

/*-------------------------------------------------------------------------*\
* Initializes the library.
\*-------------------------------------------------------------------------*/
LUALIB_API int luaopen_socket_core(lua_State *L);

#endif /* LUASOCKET_H */
