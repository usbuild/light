#include "utils/error_code.h"
namespace light {
namespace utils {
BasicErrorCategory::~BasicErrorCategory() {}
const char *BasicErrorCategory::name() const { return "system"; }

const std::string BasicErrorCategory::message(int value) {
  if (value == 0)
    return "Success";
  return std::string(strerror(value));
}

const char *MiscErrorCategory::name() const { return "misc"; }
} /* ut */

} /* li */
