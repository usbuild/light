#include "core/context.h"
namespace light {
	namespace core {

		void Context::install_handler(MessageHandler &mh) {
			mh.set_id(++last_handler_id_);
			mh.set_context(*this);
			mq_->register_callback(mh.get_id(), mh);
			
			mh.on_install();
		}

		void Context::uninstall_handler(mq_handler_id_t id) {
			if (!mq_->has_callback(id)) {
				LOG(ERROR) << "try to uninstall a nonexist module";
			} else {
				mq_->unregister_callback(id);
			}
		}

		void Context::push_message(light_message_ptr_t msg) {
			mq_->push_message(msg);
		}
	} /* co */

} /* li */
