#pragma once
#include <cassert>
#include <mutex>
#include <unordered_map>
#include "core/handler.h"
#include "utils/logger.h"
namespace light {
	namespace core {

		class MessageQueue {

			public:

				bool has_callback(mq_handler_id_t id) {
					std::lock_guard<std::mutex> lk(lock_);
					return mq_handler_map_.find(id) != mq_handler_map_.end();
				}

				MessageHandler& get_callback(mq_handler_id_t id) {
					std::lock_guard<std::mutex> lk(lock_);
					assert(mq_handler_map_.find(id) != mq_handler_map_.end());
					return *mq_handler_map_[id];
				}

				void register_callback(mq_handler_id_t id, MessageHandler& handler) {
					std::lock_guard<std::mutex> lk(lock_);
					assert(mq_handler_map_.find(id) == mq_handler_map_.end());
					mq_handler_map_[id] = &handler;
				}

				MessageHandler& unregister_callback(mq_handler_id_t id) {
					std::lock_guard<std::mutex> lk(lock_);
					assert(mq_handler_map_.find(id) != mq_handler_map_.end());
					auto it = mq_handler_map_.find(id);
					MessageHandler& mh = *it->second;
					mq_handler_map_.erase(it);
					return mh;
				}

				void push_message(light_message_ptr_t msg) {
					std::lock_guard<std::mutex> lk(lock_);
					auto handler_id = msg->to;
					auto it = mq_handler_map_.find(handler_id);
					if (GET_LOCAL_MSGID(handler_id) == 0) {
						for(auto &p : mq_handler_map_) {
							p.second->post_message(msg);
						}
						return;
					}
					if (it != mq_handler_map_.end()) {
						it->second->post_message(msg);
					}
				}

				light_message_ptr_t new_message() {
					std::shared_ptr<LightMessage> msg(new LightMessage, [this](LightMessage *ptr){
						this->del_message(ptr);
					});
					msg->reset();
					return msg;
				}

			private:
				void del_message(LightMessage *msg) {
					if (msg->destroy) msg->destroy(*msg);
					msg->data = nullptr;
					delete msg;
				}

			private:
				std::unordered_map<mq_handler_id_t, MessageHandler*> mq_handler_map_;
				std::mutex lock_;
		};
		
	} /* core */
} /* light */
