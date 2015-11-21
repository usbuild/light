#pragma once
#include <exception>
#include "utils/error_code.h"

namespace light {
	namespace exception {

		class Exception : public std::exception {
		public:
			explicit Exception(light::utils::ErrorCode &ec): ec_(ec) {}
			explicit Exception(light::utils::ErrorCode &&ec): ec_(ec) {}
			const char* what() const noexcept {
				return ec_.message().c_str();
			}
		private:
			light::utils::ErrorCode ec_;
		};

#define ADD_SIMPLE_EC_CONSTRUCTOR(Clazz) \
			explicit Clazz(light::utils::ErrorCode &ec): Exception(ec) {} \
			explicit Clazz(light::utils::ErrorCode &&ec): Exception(ec) {}


		class SocketException : public Exception {
		public:
			ADD_SIMPLE_EC_CONSTRUCTOR(SocketException)
		};

		class EventException : public Exception {
		public:
			ADD_SIMPLE_EC_CONSTRUCTOR(EventException)
		};
		
	} /* exception */
} /* light */
