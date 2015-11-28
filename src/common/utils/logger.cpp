#include "utils/logger.h"
#include <stdio.h>
#include <time.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <string.h>
#include <network/timer.h>

namespace light {
namespace utils {

void set_log_level(int level) { global_log_level = level; }

const char *LocalLoggerProxy::get_time_str() {
  static thread_local char t_buff[32];
  memset(t_buff, 0, 32);
  uint64_t stamp = get_timestamp();
  time_t sec = static_cast<time_t>(stamp / 1000000);
  int64_t usec = stamp % 1000000;
  struct tm *tm = localtime(&sec);
  int len = strftime(t_buff, sizeof(t_buff), "%m/%d/%y %H:%M:%S", tm);
  snprintf(&t_buff[len], 32 - len, ".%ld", static_cast<long>(usec));
  return t_buff;
}
}
}
