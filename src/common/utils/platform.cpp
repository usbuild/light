#include "config.h"
#include "utils/platform.h"
#ifdef WIN32
#include <network/socket.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

int socketclose(int fd) {
#ifdef WIN32
  return ::closesocket(fd);
#else
  return ::close(fd);
#endif
}

#ifdef WIN32
unsigned int writev(int fd, struct iovec *vec, unsigned int size) {
  unsigned int s = 0;
  for (unsigned int i = 0; i < size; ++i) {
    int ret =
        ::send(fd, static_cast<char *>(vec[i].iov_base), vec[i].iov_len, 0);

    if (ret == vec[i].iov_len) {
      s += ret;
    } else if (ret >= 0) {
      s += ret;
      return s;
    } else {
      if (s > 0) {
        return s;
      } else {
        return ret;
      }
    }
  }
  return s;
}
#endif
