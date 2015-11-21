#include <sys/time.h>
#include "utils/helpers.h"
namespace light {
	namespace utils {


		int set_nonblocking(int fd) {
			int flags;
			if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
				flags = 0;
			return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
		}

		int set_fd_option(int fd, int option) {
			int flags;
			if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
				flags = 0;
			return fcntl(fd, F_SETFL, flags | option);
		}


		int unset_fd_option(int fd, int option) {
			int flags;
			if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
				flags = 0;
			return fcntl(fd, F_SETFL, flags & (~option));
		}


		light::utils::ErrorCode check_socket_error(int fd) {
				int err = 0;
				socklen_t len = sizeof(int);
				int i = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
				if (i == -1) {
					return LS_GENERIC_ERROR(errno);
				}
				else if (err != 0) {
					return LS_GENERIC_ERROR(err);
				}
				return LS_OK_ERROR();
		}

		uint64_t get_timestamp() {
			struct timeval tv;
			::gettimeofday(&tv, nullptr);
			return static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
		}
	} /* ut */
} /* light */
