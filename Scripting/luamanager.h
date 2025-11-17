#ifndef LUAMANAGER_H
#define LUAMANAGER_H

#include <string>
#include <iostream>

extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

class LuaManager
{
public:
    LuaManager();
    ~LuaManager();

    bool runFile(const std::string& filename);
    bool runString(const std::string& code);

private:
    lua_State* L;
};

#endif // LUAMANAGER_H
