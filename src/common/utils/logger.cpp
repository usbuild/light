#include "utils/logger.h"
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

namespace light {
	namespace utils {

		void set_log_level(int level) {
			global_log_level = level;
		}

		const char* LocalLoggerProxy::get_time_str() {
			static thread_local char t_buff[32];
			memset(t_buff, 0, 32);
			struct timeval tv;
			gettimeofday(&tv, nullptr);
			struct tm *tm = localtime(&tv.tv_sec);
			int len = strftime(t_buff, sizeof(t_buff), "%m/%d/%y %H:%M:%S", tm);
			snprintf(&t_buff[len], 32 - len, ".%ld", static_cast<long>(tv.tv_usec));
			return t_buff;
		}

	}
}
