#pragma once
#include "core/message_queue.h"
#include "network/looper.h"


namespace light {
namespace core {
class Service;

class Context final : public light::utils::NonCopyable {
public:
  Context()
      : looper_(new light::network::Looper), mq_(new light::core::MessageQueue),
        last_handler_id_(1000), last_service_id_(5000) {}

	~Context ();

  inline light::network::Looper &get_looper() const { return *looper_; }

  inline MessageQueue &get_mq() const { return *mq_; }

  void install_handler(MessageHandler &mh);
  void uninstall_handler(mq_handler_id_t id);
  void push_message(light_message_ptr_t msg);

  static Context &instance() {
    static Context context;
    return context;
  }


  bool has_service(const std::string &name) const {
    return services_.find(name) != services_.end();
  }
  bool has_handler(const std::string &name) const {
    return handlers_.find(name) != handlers_.end();
  }


  std::error_code add_handler(const std::string &name,
    std::shared_ptr<MessageHandler> handler);

  std::error_code add_service(const std::string &name,
    std::shared_ptr<Service> service);

  std::shared_ptr<MessageHandler> get_handler(const std::string &name);

  std::shared_ptr<Service> get_service(const std::string &name);

private:
  std::unique_ptr<light::network::Looper> looper_;
  std::unique_ptr<light::core::MessageQueue> mq_;
  mq_handler_id_t last_handler_id_;

  std::map<std::string, std::shared_ptr<MessageHandler> > handlers_;
  std::map<std::string, std::shared_ptr<Service>> services_;
  int last_service_id_;
};
} /* co */

} /* li */
