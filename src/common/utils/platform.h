#pragma once

#ifdef WIN32
#pragma warning(disable : 4800)
typedef short sa_family_t;

struct iovec {
  void *iov_base;
  unsigned int iov_len;
};

unsigned int writev(int fd, struct iovec *, unsigned int);
#endif
int socketclose(int fd);
