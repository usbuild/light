#pragma once
#include <iostream>
#include <sstream>
#include <time.h>
#include <thread>
#include "utils/noncopyable.h"
#include "utils/platform.h"

enum {
	DEBUG = 1,
	INFO,
	WARNING,
	FATAL
};

namespace light {
	namespace utils {

		static int global_log_level = DEBUG;
		void set_log_level(int level);

		class LocalLoggerProxy {

		public:
			explicit LocalLoggerProxy(bool priority):dummy_(priority), ss_() { }
		public:
			template<typename T>
				LocalLoggerProxy& operator<<(const T &t) {
					if (!dummy_) ss_ << t;
					return *this;
				}

			~LocalLoggerProxy() {
				if (!dummy_) {
					ss_ << "\n";
					std::cout << ss_.str();
				}
			}

			static const char* get_time_str();

		private:
			bool dummy_;
			std::stringstream ss_;
		};

		constexpr int32_t basename_index (const char * const path, const int32_t index = 0, const int32_t slash_index = -1)
		{
			return path [index]
				? ( path [index] == '/'
					? basename_index (path, index + 1, index)
					: basename_index (path, index + 1, slash_index)
					)
				: (slash_index + 1)
				;
		}

	} /* utils */
} /* light */

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

#define LOG_FILEANDLINE __FILE__ ":" STRINGIZE(__LINE__) + light::utils::basename_index(__FILE__)

#define LOG(priority) light::utils::LocalLoggerProxy(priority < light::utils::global_log_level) << "[" << light::utils::LocalLoggerProxy::get_time_str() << " " << std::this_thread::get_id() << " " << LOG_FILEANDLINE <<"][" << STRINGIZE(priority) << "] "

#ifdef NDEBUG
#define DLOG(priority) light::utils::LocalLoggerProxy(priority < light::utils::global_log_level)
#else
#define DLOG(priority) LOG(priority)
#endif

