#pragma once
#include "core/message_queue.h"
#include "network/looper.h"
namespace light {
	namespace core {
		class Context {
		public:
			Context(): looper_(new light::network::Looper), mq_(new light::core::MessageQueue), last_handler_id_(1000) {}

			inline light::network::Looper& get_looper() const {
				return *looper_;
			}

			inline MessageQueue& get_mq() const {
				return *mq_;
			}

			void install_handler(MessageHandler &mh);
			void uninstall_handler(mq_handler_id_t id);
			void push_message(light_message_ptr_t msg);

			static Context &instance() {
				static Context context;
				return context;
			}

		private:
			std::unique_ptr<light::network::Looper> looper_;
			std::unique_ptr<light::core::MessageQueue> mq_;
			mq_handler_id_t last_handler_id_;
		};
	} /* co */
	
} /* li */
