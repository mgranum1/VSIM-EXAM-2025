#include "LuaManager.h"

LuaManager::LuaManager()
{
    L = luaL_newstate();
    if (!L)
    {
        std::cerr << "Failed to create Lua state!" << std::endl;
        return;
    }
    luaL_openlibs(L);
    std::cout << "Lua initialized successfully." << std::endl;
}

LuaManager::~LuaManager()
{
    if (L)
    {
        lua_close(L);
        L = nullptr;
        std::cout << "Lua state closed." << std::endl;
    }
}

bool LuaManager::runFile(const std::string& filename)
{
    if (luaL_dofile(L, filename.c_str()) != LUA_OK)
    {
        std::cerr << "Lua error: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
        return false;
    }
    return true;
}

bool LuaManager::runString(const std::string& code)
{
    if (luaL_dostring(L, code.c_str()) != LUA_OK)
    {
        std::cerr << "Lua error: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
        return false;
    }
    return true;
}
