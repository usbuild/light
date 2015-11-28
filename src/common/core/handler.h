#pragma once
#include "core/message.h"
#include "utils/noncopyable.h"
#include "utils/error_code.h"

namespace light {
namespace core {
typedef uint32_t mq_handler_id_t;

class Context;

class MessageHandler : public light::utils::NonCopyable {
public:
  MessageHandler() : id_(0), ctx_(nullptr) {}
  virtual ~MessageHandler() {}

  virtual light::utils::ErrorCode init() = 0;
  virtual void on_install() = 0;
  virtual void post_message(light::core::light_message_ptr_t msg) = 0;
  virtual void on_unstall() = 0;
  virtual void fini() = 0;

  mq_handler_id_t get_id() const { return id_; }
  void set_id(mq_handler_id_t id) { id_ = id; }

  Context &get_context() const { return *ctx_; }
  void set_context(Context &ctx) { ctx_ = &ctx; }

protected:
  mq_handler_id_t id_;
  Context *ctx_;
};
} /* core */

} /* li */
