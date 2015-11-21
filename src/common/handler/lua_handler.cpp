#include "core/context.h"
#include "handler/lua_handler.h"
//test
#include "proto/station_generated.h"
//test
namespace light {
	namespace handler {

		static int light_send(lua_State *l) {
			uint32_t to = lua_tonumber(l, -3);
			uint32_t type = lua_tonumber(l, -2);
			void *data = lua_touserdata(l, -1);

			lua_getglobal(l, "light_context");
			LuaHandler *handler = reinterpret_cast<LuaHandler*>(lua_touserdata(l, -1));

			auto msg = handler->get_context().get_mq().new_message();
			msg->to = static_cast<light::core::mq_handler_id_t>(to);
			msg->from = handler->get_id();
			msg->type = static_cast<uint8_t>(type);
			msg->data = data;
			handler->get_context().push_message(msg);
			return 0;
		}

		static int create_message(lua_State *l) {
			lua_getglobal(l, "light_context");
			LuaHandler *handler = reinterpret_cast<LuaHandler*>(lua_touserdata(l, -1));
			auto t = handler->create_fbs_message<light::proto::ConnectRequestBuilder>();
			lua_pushlightuserdata(l, t);
			return 1;
		}

		static int preload_light_module(lua_State *l) {
			static const luaL_Reg lualibs[] = {
				{"post", light_send},
				{"create_message", create_message},
				{nullptr, nullptr},
			};
			luaL_newlib(l, lualibs);
			return 1;
		}

		void LuaHandler::register_core_functions() {
			auto l = lua_ctx_->L();
			lua_getglobal(l, "package");
			lua_getfield(l, -1, "preload");
			lua_pushcfunction(l, preload_light_module);
			lua_setfield(l, -2, "light");
		}

	} /* ha */
} /* li */
