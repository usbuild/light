#pragma once
#include <fcntl.h>
#include <functional>
#include "utils/error_code.h"
#include "utils/noncopyable.h"
namespace light {
	namespace utils {
		int set_nonblocking(int fd);

		light::utils::ErrorCode check_socket_error(int fd);

		class ScopeExit {
		public:
			ScopeExit(std::function<void (void)> &&f) : f_(f) {}
			~ScopeExit() {if(f_) f_();}
		private:
			std::function<void (void)> f_;
		};
		uint64_t get_timestamp();

		template<typename T> struct icast_identity { typedef T type; };

		int SocketGlobalInitialize();
		void SocketGlobalFinitialize();
	} /* utils */

	template <typename T>
		inline T implicit_cast (typename light::utils::icast_identity<T>::type x) {
			return x;
		}

} /* light */


#define STRING_JOIN2(arg1, arg2) DO_STRING_JOIN2(arg1, arg2)
#define DO_STRING_JOIN2(arg1, arg2) arg1 ## arg2

#define SCOPE_EXIT\
	light::utils::ScopeExit STRING_JOIN2(scope_exit_, __LINE__)

#define UNUSED(x) (void)((x))
