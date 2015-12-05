#include "lua_utils.h"
#include <sstream>
namespace light {
  namespace lua {
    std::vector<std::string> split_string(const std::string &s, char delim)
    {
      std::stringstream ss(s);
      std::string item;
      std::vector<std::string> ret;
      while (std::getline(ss, item, delim))
      {
        ret.push_back(item);
      }
      return ret;
    }
    void begin_lua_module(lua_State *lstate, int idx, const std::string &ns)
    {
      std::vector<std::string> splitNamespaces = split_string(ns, '.');
      int top = lua_gettop(lstate);
      if (idx < 0 && (idx + top + 1) > 0)
      {
        idx = top + idx + 1;
      }

      for (std::vector<std::string>::const_iterator it = splitNamespaces.begin(); it != splitNamespaces.end(); ++it)
      {

        lua_getfield(lstate, idx, it->c_str());
        if (lua_isnil(lstate, -1))
        {
          lua_newtable(lstate);
          lua_setfield(lstate, idx, it->c_str());
          lua_getfield(lstate, idx, it->c_str());
        }
        idx = lua_gettop(lstate);
      }
    }

    static int preload_lua_module(lua_State *lstate)
    {
      lua_pushvalue(lstate, lua_upvalueindex(1));
      return 1;
    }

    void register_as_lua_module(lua_State *lstate, int idx, const char* className)
    {
      int top = lua_gettop(lstate);
      if (idx < 0 && (idx + top + 1) > 0)
      {
        idx = top + idx + 1;
      }

      lua_getglobal(lstate, "package");
      lua_getfield(lstate, -1, "preload");
      lua_pushvalue(lstate, idx);
      lua_pushcclosure(lstate, preload_lua_module, 1);
      lua_setfield(lstate, -2, className);
    }

    void register_as_lua_module(lua_State *lstate, int table_indx, const char* filedName, const char* className)
    {
      lua_getfield(lstate, table_indx, filedName);
      register_as_lua_module(lstate, -1, className);
    }

    void register_function_to_lua(lua_State* lstate, const char *name, lua_CFunction function)
    {
      lua_pushcfunction(lstate, function);
      lua_setfield(lstate, -2, name);
    }

  }
}