#include <chrono>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef WIN32
#include <network/socket.h>
#endif

#include "utils/helpers.h"
namespace light {
	namespace utils {


		int set_nonblocking(int fd) {
#ifdef WIN32
			u_long flag = 1;
			return ioctlsocket(fd, FIONBIO, &flag);
#else
			int flags;
			if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
				flags = 0;
			return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
		}


		light::utils::ErrorCode check_socket_error(int fd) {
#ifdef WIN32
				char err = 0;
#else
				int err = 0;
#endif
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
			return std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
		}

		int SocketGlobalInitialize()
		{
#ifdef WIN32
			WORD versionRequested = MAKEWORD(1, 1);
			WSADATA wsaData;

			if (WSAStartup(versionRequested, &wsaData))
				return -1;

			if (LOBYTE(wsaData.wVersion) != 1 ||
				HIBYTE(wsaData.wVersion) != 1)
			{
				WSACleanup();

				return -1;
			}

#endif

			return 0;
		}

		void SocketGlobalFinitialize()
		{
#ifdef WIN32
			WSACleanup();
#endif
		}
	} /* ut */
} /* light */
