#pragma once
#include <algorithm>
#include "core/service.h"
#include "utils/logger.h"

namespace light {
namespace service {

class LuaService : public light::core::Service {

public:
  LuaService(light::core::Context &ctx);
  ~LuaService();
  std::error_code install_new_handler(light::core::Context& ctx, const std::string& file, const std::string& iname, const std::string& args);
  
  std::error_code init() override;

  std::error_code fini() override;

  static const char *name;

private:
  light::core::Context *ctx_;
};

} /* handler */
} /* light */
