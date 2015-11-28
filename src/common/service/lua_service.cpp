#include "core/default_context_loader.h"
#include "service/lua_service.h"
#include "handler/lua_handler.h"
namespace light {
namespace service {
const char *LuaService::name = "lua_service";

LuaService::LuaService(light::core::Context &ctx) : Service(ctx), ctx_(&ctx) {}

LuaService::~LuaService() {}

light::utils::ErrorCode
LuaService::install_new_handler(const std::string &file,
                                const std::string &iname,
                                const std::string &args) {
  auto lctx = light::handler::LuaContext::new_context();
  auto lua_handler = light::core::DefaultContextLoader::instance()
                         .require_handler<light::handler::LuaHandler>(
                             file, iname, *lctx, file, iname, args);
  if (!lua_handler) {
    return LS_GENERIC_ERR_OBJ(not_enough_memory);
  }
  ctx_->install_handler(*lua_handler);
  return LS_OK_ERROR();
}

light::utils::ErrorCode LuaService::init() { return LS_OK_ERROR(); }

light::utils::ErrorCode LuaService::fini() { return LS_OK_ERROR(); }
} /* ha */

} /* li */
