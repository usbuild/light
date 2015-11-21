#pragma once
#include "core/context.h"
#include "core/service.h"
#include "core/handler.h"
#include "utils/error_code.h"
namespace light {
	namespace core {

		class ContextLoader {
		public:
			light::utils::ErrorCode add_handler(const std::string &name, std::shared_ptr<MessageHandler> handler) {
				if (has_handler(name)) {
					return LS_MISC_ERR_OBJ(already_open);
				}
				handlers_[name] = handler;
				return LS_OK_ERROR();
			}

			light::utils::ErrorCode add_service(const std::string &name, std::shared_ptr<Service> service) {
				if (has_service(name)) {
					return LS_MISC_ERR_OBJ(already_open);
				}
				services_[name] = service;
				return LS_OK_ERROR();
			}

			bool has_service(const std::string &name) const {
				return services_.find(name) != services_.end();
			}
			bool has_handler(const std::string &name) const {
				return handlers_.find(name) != handlers_.end();
			}

			std::shared_ptr<MessageHandler> get_handler(const std::string &name) {
				if (!has_handler(name)) return nullptr;
				return handlers_[name];
			}

			std::shared_ptr<Service>  get_service(const std::string &name) {
				if (!has_service(name)) return nullptr;
				return services_[name];
			}

		protected:
			std::map<std::string, std::shared_ptr<MessageHandler> > handlers_;
			std::map<std::string, std::shared_ptr<Service> > services_;
		};
		
	} /* co */
} /* li */
