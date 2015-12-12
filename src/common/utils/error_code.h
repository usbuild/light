#if defined(ENUM_DECLARE_ERROR_CODE)
#define ADD_ERROR_CODE_DEF(code, desc) code,
#elif defined(ERROR_CODE_MESSAGE_SWITCH)
#define ADD_ERROR_CODE_DEF(code, desc)                                                             \
  case light::utils::error_code_t::code:                                                              \
    return desc;
#else
#define ADD_ERROR_CODE_DEF(code, desc)
#endif

/////////add your custom error code definition here
ADD_ERROR_CODE_DEF(already_open, "Already open")
ADD_ERROR_CODE_DEF(eof, "End of file")
ADD_ERROR_CODE_DEF(not_found, "Not Found")
ADD_ERROR_CODE_DEF(fd_set_failure, "FD SET Failure")
ADD_ERROR_CODE_DEF(name_occupied, "name occupied")
ADD_ERROR_CODE_DEF(unknown, "Unknown")

#undef ADD_ERROR_CODE_DEF

#ifndef __LIGHT_COMMON_UTILS_ERROR_CODE_HPP
#define __LIGHT_COMMON_UTILS_ERROR_CODE_HPP

#include <system_error>
#include <cerrno>

#ifdef WIN32
#define ERRNO() (WSAGetLastError())
#define CERR(err) WSA##err
#else
#define ERRNO() errno
#define CERR(err) err
#endif
namespace light {

  namespace utils {
    enum error_code_t {
      ok = 0,
#define ENUM_DECLARE_ERROR_CODE
#include __FILE__
#undef ENUM_DECLARE_ERROR_CODE
      end
    };

    enum error_condition_t {};

    std::error_code make_error_code(error_code_t e);
    std::error_condition make_error_condition(error_condition_t e);

  }

  class error_category_impl : public std::error_category {
  public:
    virtual const char *name() const noexcept;
    virtual std::string message(int ev) const noexcept;
    virtual std::error_condition default_error_condition(int ev) const noexcept;
  };

  const std::error_category &error_category();

}

namespace std {
  template <> struct is_error_code_enum<light::utils::error_code_t> : public true_type {};

  template <> struct is_error_condition_enum<light::utils::error_condition_t> : public true_type {};
}


#define LS_GENERIC_ERROR(err)                                                  \
  std::error_code(err, std::system_category())
#define LS_GENERIC_ERR_OBJ(err)   std::error_code(static_cast<int>(std::errc::err), std::system_category())

#define LS_MISC_ERR_OBJ(err)                                                   \
  light::utils::make_error_code(light::utils::error_code_t::err)
#define LS_OK_ERROR()                                                          \
  LS_GENERIC_ERROR(0)

#endif
