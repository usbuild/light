#pragma once
#include "core/context_loader.h"
namespace light {
namespace core {

class DefaultContextLoader : public ContextLoader {
public:
  static DefaultContextLoader &instance() {
    static DefaultContextLoader loader;
    return loader;
  }

  template <typename T, typename... ARGS>
  std::shared_ptr<T> require_handler(Context &ctx, const std::string &handle_class_name,
                                     const std::string &name, ARGS &&... args) {
    UNUSED(handle_class_name);
    std::shared_ptr<MessageHandler> mh;
    if (ctx.has_handler(name)) {
      mh = ctx.get_handler(name);
    } else {
      T *t = new T(std::forward<ARGS>(args)...);
      if (t == nullptr) {
        return nullptr;
      }
      auto ec = t->init();
      if (ec) {
        DLOG(FATAL) << "handler init failed: " << ec.message();
        return nullptr;
      }
      mh.reset(t);
      if (!mh) {
        return nullptr;
      }
      ctx.add_handler(name, mh);
    }
    return std::dynamic_pointer_cast<T>(mh);
  }

  template <typename T, typename... ARGS>
  std::shared_ptr<T> require_service(Context &ctx, const std::string &service_class_name,
                                     const std::string &name, ARGS &&... args) {
    UNUSED(service_class_name);
    std::shared_ptr<Service> svc;
    if (ctx.has_service(name)) {
      svc = ctx.get_service(name);
    } else {
      T *t = new T(std::forward<ARGS>(args)...);
      if (t == nullptr) {
        return nullptr;
      }
      auto ec = t->init();
      if (ec) {
        DLOG(FATAL) << "handler init failed: " << ec.message();
        return nullptr;
      }
      svc.reset(t);
      if (!svc) {
        return nullptr;
      }
      ctx.add_service(name, svc);
    }
    return std::dynamic_pointer_cast<T>(svc);
  }
};
}
}
