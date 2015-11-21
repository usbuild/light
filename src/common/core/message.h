#pragma once
#include <functional>
#include <memory>
namespace light {
	namespace core {

		typedef uint32_t mq_handler_id_t;

		class MessageQueue;

		enum MessageType {
			SOCKET = 1
		};

#define PACKED_MSG_HEADER \
			mq_handler_id_t to; \
			mq_handler_id_t from; \
			uint8_t type; \
			uint32_t size;

		/**
		 * @brief 网络传输用，只在gate和station中使用
		 */
		struct PackedMessage {
			PACKED_MSG_HEADER
		};

		struct PackedMessageCookie {
			PACKED_MSG_HEADER
		};

		/**
		 * @brief 内部传输用
		 */
		struct LightMessage {
		private:
			LightMessage() {}

		public:
			mq_handler_id_t to;
			uint8_t type;
			mq_handler_id_t from;
			uint32_t size;
			std::function<void(LightMessage&)> destroy;
			void *data;
			friend class MessageQueue;

		public:
			void reset() {
				from = 0;
				to = 0;
				type = 0;
				data = nullptr;
				destroy = 0;
			}
		};
		typedef std::shared_ptr<LightMessage> light_message_ptr_t;
	} /* core */

} /* light */
#define GET_LOCAL_MSGID(x) (static_cast<light::core::mq_handler_id_t>(x) & ((~static_cast<light::core::mq_handler_id_t>(0)) >> sizeof(uint8_t)))
#define GET_STATION_ID(x) (static_cast<light::core::mq_handler_id_t>(x) >> (sizeof(light::core::mq_handler_id_t) - sizeof(uint8_t)))
