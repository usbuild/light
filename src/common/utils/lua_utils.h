#pragma once
#include <cstring>
#include <string>
#include <vector>
#include <typeinfo>
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

namespace light {
  namespace lua {
    template<typename CLASS>
    int lua_class_on_gc(lua_State* lstate)
    {
      const char* classDef = typeid(CLASS).name();
      lua_getmetatable(lstate, -1);
      lua_getfield(lstate, -1, ".class");
      if (std::strncmp(lua_tostring(lstate, -1), classDef, strlen(classDef)))
      {
        luaL_error(lstate, "FATAL ERROR! try to release %s expected %s", lua_tostring(lstate, -1), typeid(CLASS).name());
        return 0;
      }
      CLASS *addr = reinterpret_cast<CLASS*>(lua_touserdata(lstate, -3));
      addr->~CLASS();

      return 0;
    }


    template<typename CLASS>
    void register_class_to_lua(lua_State* lstate, const char* className, const luaL_Reg *static_methods, const luaL_Reg *class_methods, lua_CFunction constructor)
    {
      int top = lua_gettop(lstate);
      lua_newtable(lstate); //ns LuaAddr
      lua_setfield(lstate, -2, className);//ns
      lua_getfield(lstate, -1, className);//ns className
      lua_pushstring(lstate, typeid(CLASS).name()); //ns className .class
      lua_setfield(lstate, -2, ".class"); //rn className

      lua_newtable(lstate);//rn className __index
      lua_setfield(lstate, -2, "__index");
      lua_getfield(lstate, -1, "__index");//rn className __index

      if (class_methods) {
        for (int i = 0; class_methods[i].func != nullptr; ++i)
        {
          lua_pushcfunction(lstate, class_methods[i].func);
          lua_setfield(lstate, -2, class_methods[i].name);
        }
      }
      lua_pop(lstate, 1);//rn className

      if (static_methods) {
        for (int i = 0; static_methods[i].func != nullptr; ++i)
        {
          lua_pushcfunction(lstate, static_methods[i].func);
          lua_setfield(lstate, -2, static_methods[i].name);
        }
      }


      lua_pushcfunction(lstate, constructor);//rn className new
      lua_setfield(lstate, -2, "new");

      lua_pushcfunction(lstate, lua_class_on_gc<CLASS>);
      lua_setfield(lstate, -2, "__gc");

      lua_settop(lstate, top);
    }

    void register_function_to_lua(lua_State* lstate, const char *name, lua_CFunction function);


    std::vector<std::string> split_string(const std::string &s, char delim);

    void begin_lua_module(lua_State *lstate, int idx, const std::string &ns);

    template<typename T>
    bool lua_is_class(lua_State *lstate, int idx)
    {
      if (!lua_isuserdata(lstate, idx))
      {
        return false;
      }

      lua_getmetatable(lstate, idx);
      if (lua_isnil(lstate, -1))
      {
        lua_pop(lstate, 1);
        return false;
      }
      lua_getfield(lstate, -1, ".class");
      if (!lua_isstring(lstate, -1))
      {
        lua_pop(lstate, 2);
        return false;
      }
      const char *needClassName = typeid(T).name();
      if (std::strncmp(needClassName, lua_tostring(lstate, -1), strlen(needClassName)))
      {
        lua_pop(lstate, 2);
        return false;
      }
      else
      {
        lua_pop(lstate, 2);
        return true;
      }
    }

    template<typename T>
    bool set_lua_class_metatable(lua_State *lstate, int classNameIdx, const char *className)
    {
      const char *typeName = typeid(T).name();
      if (!lua_isuserdata(lstate, -1))
      {
        luaL_error(lstate, "expect userdata class[%s]", typeName);
        return false;
      }
      int top = lua_gettop(lstate);

      std::vector<std::string> splitNamespaces = split_string(className, '.');
      for (std::vector<std::string>::const_iterator it = splitNamespaces.begin(); it != splitNamespaces.end(); ++it)
      {
        lua_getfield(lstate, classNameIdx, it->c_str());
        if (!lua_istable(lstate, -1))
        {
          lua_settop(lstate, top);
          luaL_error(lstate, "%s is not a table", it->c_str());
          return false;
        }
        classNameIdx = -1;
      }

      lua_getfield(lstate, -1, ".class");
      if (!lua_isstring(lstate, -1))
      {
        lua_settop(lstate, top);
        luaL_error(lstate, "%s is not valid lua meta class", className);
        return false;
      }
      if (std::strncmp(typeName, lua_tostring(lstate, -1), strlen(typeName)))
      {
        lua_settop(lstate, top);
        luaL_error(lstate, "%s is %s, while the instance is %s", className, lua_tostring(lstate, -1), typeName);
        return false;
      }

      lua_pop(lstate, 1); // pop .class

      lua_setmetatable(lstate, top);
      lua_settop(lstate, top);

      return true;
    }

    template<typename T>
    bool class_to_lua_class(lua_State *lstate, const T &t, int metaIdx, const char* className)
    {
      void* ptr = lua_newuserdata(lstate, sizeof(T));
      new (ptr)T(t);
      return set_lua_class_metatable<T>(lstate, metaIdx, className);
    }

    
    void register_as_lua_module(lua_State *lstate, int idx, const char* className);
    void register_as_lua_module(lua_State *lstate, int table_indx, const char* filedName, const char* className);
  }
}

#define CHECK_LUA_ARG_COUNT(lstate, need) if (lua_gettop(lstate) != (need)) \
{\
	luaL_error(lstate, "invalid argument count expect %d args, actually %d", need, lua_gettop(lstate));\
	return 0; \
}

#define CHECK_LUA_ARG_COUNT_GEQ(lstate, need) if (lua_gettop(lstate) < (need)) \
{\
	luaL_error(lstate, "invalid argument count expect at least %d args, actually %d", need, lua_gettop(lstate));\
	return 0; \
}

#define CHECK_LUA_ARG_TYPE(lstate, idx, func, info) if (!func(lstate, idx)) { \
	luaL_argcheck(lstate, func(lstate, idx), idx, "expect "#info);\
	return 0; \
}

#define PCALL_WITH_ERROR(lstate, nArgs, nRets, errorFunc) if (lua_pcall(lstate, nArgs, nRets, errorFunc)) { \
	puts(lua_tostring(lstate, -1));
