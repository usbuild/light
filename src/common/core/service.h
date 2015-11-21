#pragma once

#include "network/looper.h"
#include "utils/noncopyable.h"
#include "core/context.h"

namespace light {
	namespace core {

		class Service : public light::utils::NonCopyable {
			public:
				/**
				 * @brief ���캯����������ctx���ù����߳�
				 *
				 * @param ctx Context
				 */
				Service(light::core::Context &ctx): looper_(&ctx.get_looper()), mq_(&ctx.get_mq()){}

				/**
				 * @brief ���캯����������looper.loop���߳�������
				 *
				 * @param looper
				 */
				Service(light::network::Looper &looper, light::core::MessageQueue &mq): looper_(&looper), mq_(&mq) {}

				virtual ~Service() {}

				virtual light::utils::ErrorCode init() = 0;
				virtual light::utils::ErrorCode fini() = 0;

				template<typename CLASS, typename FUNC, typename... ARGS>
				void post(FUNC func, ARGS&&... args) {
					get_looper().post(std::bind(func, static_cast<CLASS*>(this), std::forward<ARGS>(args)...));
				}

				template<typename CLASS, typename FUNC, typename RET, typename... ARGS>
					light::network::SafeCallWrapper<RET> wrap(FUNC func, ARGS&&... args) {
						return get_looper().wrap(std::bind(func, static_cast<CLASS*>(this), std::forward<ARGS>(args)...));
					}

				light::core::MessageQueue& get_mq() {
					return *mq_;
				}

				light::network::Looper& get_looper() {
					return *looper_;
				}

			protected:
				light::network::Looper *looper_;
				light::core::MessageQueue *mq_;
		};
		
	} /* core */
} /* light */
//#define CALL_SERVICE_METHOD(SVC, FUNC, ...) SVC.post<decltype(SVC)>(&decltype(SVC)::FUNC, ##__VA_ARGS__);
