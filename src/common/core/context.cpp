#include "core/context.h"
#include "core/service.h"

namespace light {
namespace core {
Context::~Context () {
	DLOG(INFO) << __FUNCTION__ << " " << this;
  for (auto &kv : handlers_) {
    kv.second->fini();
  }
  for (auto &kv : services_) {
    kv.second->fini();
  }
}

void Context::install_handler(MessageHandler &mh) {
  mh.set_id(++last_handler_id_);
  mh.set_context(*this);
  mq_->register_callback(mh.get_id(), mh);

  mh.on_install();
}

void Context::uninstall_handler(mq_handler_id_t id) {
  if (!mq_->has_callback(id)) {
    LOG(FATAL) << "try to uninstall a nonexist module";
  } else {
    mq_->unregister_callback(id);
  }
}

void Context::push_message(light_message_ptr_t msg) { mq_->push_message(msg); }
} /* co */

} /* li */
