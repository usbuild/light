#include "proto/station_generated.h"
#include "core/context.h"
#include "handler/lua_handler.h"
namespace light {
namespace handler {

LuaHandler * get_handler_from_lua(lua_State *l)
{
  lua_getfield(l, LUA_REGISTRYINDEX, "lua_handler");
  auto ret = static_cast<LuaHandler*>(lua_touserdata(l, -1));
  lua_pop(l, 1);
  return ret;
}

static int light_send(lua_State *l) {
  CHECK_LUA_ARG_COUNT(l, 3);
  CHECK_LUA_ARG_TYPE(l, 1, lua_isnumber, "number");
  CHECK_LUA_ARG_TYPE(l, 2, lua_isnumber, "number");
  CHECK_LUA_ARG_TYPE(l, 3, lua_isuserdata, "data");

  uint32_t to = lua_tonumber(l, 1);
  uint32_t type = lua_tonumber(l, 2);
  void *data = lua_touserdata(l, 3);

  LuaHandler *handler = get_handler_from_lua(l);

  auto msg = handler->get_context().get_mq().new_message();
  msg->to = static_cast<light::core::mq_handler_id_t>(to);
  msg->from = handler->get_id();
  msg->type = static_cast<uint8_t>(type);
  msg->data = data;
  handler->get_context().push_message(msg);
  return 0;
}

void LuaHandler::register_core_functions() {
  auto l = lua_ctx_->L();
  lua_pushlightuserdata(l, this);
  lua_setfield(l, LUA_REGISTRYINDEX, "lua_handler");

  light::lua::begin_lua_module(l, LUA_REGISTRYINDEX, "light");
  light::lua::register_function_to_lua(l, "post", light_send);
  using crb_t = light::proto::ConnectRequestBuilder;
  
  light::lua::register_class_to_lua<crb_t>(l, "ConnectRequestBuilder", nullptr, nullptr, [](lua_State *ls)
  {
    return get_handler_from_lua(ls)->g_new_message<light::proto::ConnectRequestBuilder>(ls, "light.ConnectRequestBuilder");
  });
  light::lua::register_as_lua_module(l, -1, "light");
}

} /* ha */
} /* li */
