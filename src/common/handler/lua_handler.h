#pragma once
#include "flatbuffers/flatbuffers.h"
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "core/message.h"
#include "core/handler.h"
#include "utils/noncopyable.h"
#include "utils/error_code.h"
#include "utils/logger.h"

namespace light {
	namespace handler {
		typedef uint32_t mq_handler_id_t;

		class LuaContext {
		public:
			~LuaContext() {
				if (l_ != nullptr) {
					lua_close(l_);
					l_ = nullptr;
				}
			}

			static LuaContext *new_context() {
				auto ctx = new (std::nothrow) LuaContext;
				if (ctx == nullptr) return ctx;
				ctx->l_ = luaL_newstate();
				if (ctx->l_ == nullptr) {
					LOG(FATAL) << "cannot create lua state: not enough memory";
					return nullptr;
				}
				luaL_openlibs(ctx->l_);
				return ctx;
			}

			inline lua_State * L() {
				return l_;
			}

		private:
			lua_State *l_ = nullptr;

		};

		class LuaHandler : public light::core::MessageHandler {
		public:
			LuaHandler(LuaContext &lua_ctx, const std::string& script_name, const std::string& name, const std::string &args): lua_ctx_(&lua_ctx),
			script_name_(script_name), name_(name), args_(args) {}
			virtual ~LuaHandler() {}

			light::utils::ErrorCode init() {
				if (!lua_ctx_) {
					return LS_GENERIC_ERR_OBJ(not_enough_memory);
				}
				register_core_functions();
				if(luaL_dofile(lua_ctx_->L(), script_name_.c_str())) {
					LOG(FATAL) << "Error while loading script: " << script_name_ << " " << lua_tostring(lua_ctx_->L(), -1);
					return LS_MISC_ERR_OBJ(unknown);
				}
				return LS_OK_ERROR();
			}

			void on_install() {
				lua_pushlightuserdata(lua_ctx_->L(), this);
				lua_setglobal(lua_ctx_->L(), "light_context");

				handler_func_ = [this](light::core::light_message_ptr_t msg){
					auto l = lua_ctx_->L();
					lua_getglobal(l, "on_message");
					lua_pushinteger(l, msg->type);
					lua_pushinteger(l, msg->from);
					lua_pushlightuserdata(l, msg->data);
					lua_pcall(L(), 3, 0, 0);
				};

				if (!lua_getglobal(lua_ctx_->L(), "on_install")) {
					LOG(FATAL) << "cannot find on_install";
					return;
				}
				if (!lua_isfunction(lua_ctx_->L(), -1)) {
					LOG(FATAL) << "cannot find on_install function";
					return;
				}

				lua_pushinteger(lua_ctx_->L(), get_id());
				if (lua_pcall(lua_ctx_->L(), 1, 0, 0)) {
					LOG(FATAL) << "error call install function " << lua_tostring(lua_ctx_->L(), -1);
				}

				if (!lua_getglobal(lua_ctx_->L(), "on_message")) {
					LOG(FATAL) << "cannot find on_message";
					return;
				}

				if (!lua_isfunction(lua_ctx_->L(), -1)) {
					LOG(FATAL) << "cannot find on_message function";
					return;
				}



			}

			void register_core_functions();

			void post_message(light::core::light_message_ptr_t msg) {
				if (handler_func_) handler_func_(msg);
			}
			void on_unstall() {}
			void fini() {}

			inline lua_State *L() {
				return lua_ctx_->L();
			}

			template<typename T>
				T* create_fbs_message() {
					return new T(fbb);
				}

			void delete_fbs_message(void *ptr) {
        UNUSED(ptr);
				//delete ptr;
			}

		private:
			LuaContext *lua_ctx_;
			std::function<void(light::core::light_message_ptr_t msg)> handler_func_;
			std::string script_name_;
			std::string name_;
			std::string args_;
			flatbuffers::FlatBufferBuilder fbb;

		};
	} /* core */

} /* li */
