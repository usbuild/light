#include "utils/error_code.h"
namespace light {
  namespace utils {
    std::error_code make_error_code(error_code_t e) {
      return std::error_code(static_cast<int>(e), error_category());
    }

    std::error_condition make_error_condition(error_condition_t e) {
      return std::error_condition(static_cast<int>(e), error_category());
    }
  }

  const char *error_category_impl::name() const noexcept { return "light"; }

  std::string error_category_impl::message(int ev) const noexcept {
    switch (ev) {
#define ERROR_CODE_MESSAGE_SWITCH
#include "utils/error_code.h"
#undef ERROR_CODE_MESSAGE_SWITCH
    default:
      return "unknown";
    }
  }

  std::error_condition error_category_impl::default_error_condition(int ev) const noexcept {
    switch (ev) {
    default:
      return std::error_condition(ev, *this);
    }
  }

  const std::error_category &error_category() {
    static error_category_impl instance;
    return instance;
  }
}
