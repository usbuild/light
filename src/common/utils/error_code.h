#pragma once
#include <cassert>
extern "C" {
	#include <errno.h>
}
#include <string>
#include <string.h>
#include <netdb.h>
#include "utils/noncopyable.h"

namespace light {
	namespace utils {
		namespace generic_errors {
			enum basic
			{/*{{{*/
				access_denied = EACCES,
				address_family_not_supported = EAFNOSUPPORT,
				address_in_use = EADDRINUSE,
				address_not_available = EADDRNOTAVAIL,
				already_connected = EISCONN,
				already_started = EALREADY,
				argument_list_too_long = E2BIG,
				argument_out_of_domain = EDOM,
				bad_address = EFAULT,
				bad_descriptor = EBADF,
				bad_file_descriptor = EBADF,
				bad_message = EBADMSG,
				broken_pipe = EPIPE,
				connection_aborted = ECONNABORTED,
				connection_already_in_progress = EALREADY,
				connection_refused = ECONNREFUSED,
				connection_reset = ECONNRESET,
				cross_device_link = EXDEV,
				destination_address_required = EDESTADDRREQ,
				device_or_resource_busy = EBUSY,
				directory_not_empty = ENOTEMPTY,
				executable_format_error = ENOEXEC,
				fault = EFAULT,
				file_exists = EEXIST,
				filename_too_long = ENAMETOOLONG,
				file_too_large = EFBIG,
				function_not_supported = ENOSYS,
				host_unreachable = EHOSTUNREACH,
				identifier_removed = EIDRM,
				illegal_byte_sequence = EILSEQ,
				inappropriate_io_control_operation = ENOTTY,
				in_progress = EINPROGRESS,
				interrupted = EINTR,
				invalid_argument = EINVAL,
				invalid_seek = ESPIPE,
				io_error = EIO,
				is_a_directory = EISDIR,
				message_size = EMSGSIZE,
				name_too_long = ENAMETOOLONG,
				network_down = ENETDOWN,
				network_reset = ENETRESET,
				network_unreachable = ENETUNREACH,
				no_buffer_space = ENOBUFS,
				no_child_process = ECHILD,
				no_descriptors = EMFILE,
				no_link = ENOLINK,
				no_lock_available = ENOLCK,
				no_memory = ENOMEM,
				no_message_available = ENODATA,
				no_message = ENOMSG,
				no_permission = EPERM,
				no_protocol_option = ENOPROTOOPT,
				no_space_on_device = ENOSPC,
				no_stream_resources = ENOSR,
				no_such_device = ENODEV,
				no_such_device_or_address = ENXIO,
				no_such_file_or_directory = ENOENT,
				no_such_process = ESRCH,
				not_a_directory = ENOTDIR,
				not_a_socket = ENOTSOCK,
				not_a_stream = ENOSTR,
				not_connected = ENOTCONN,
				not_enough_memory = ENOMEM,
				not_socket = ENOTSOCK,
				not_supported = ENOTSUP,
				operation_aborted = ECANCELED,
				operation_canceled = ECANCELED,
				operation_in_progress = EINPROGRESS,
				operation_not_permitted = EPERM,
				operation_not_supported = EOPNOTSUPP,
				operation_would_block = EWOULDBLOCK,
				owner_dead = EOWNERDEAD,
				permission_denied = EACCES,
				protocol_error = EPROTO,
				protocol_not_supported = EPROTONOSUPPORT,
				read_only_file_system = EROFS,
				resource_deadlock_would_occur = EDEADLK,
				resource_unavailable_try_again = EAGAIN,
				result_out_of_range = ERANGE,
				shut_down = ESHUTDOWN,
				state_not_recoverable = ENOTRECOVERABLE,
				stream_timeout = ETIME,
				success = 0,
				text_file_busy = ETXTBSY,
				timed_out = ETIMEDOUT,
				too_many_files_open = EMFILE,
				too_many_files_open_in_system = ENFILE,
				too_many_links = EMLINK,
				too_many_symbolic_link_levels = ELOOP,
				try_again = EAGAIN,
				value_too_large = EOVERFLOW,
				would_block = EWOULDBLOCK,
				wrong_protocol_type = EPROTOTYPE,

				host_not_found = HOST_NOT_FOUND,
				host_not_found_try_again = TRY_AGAIN,
				no_data = NO_DATA,
				no_recovery = NO_RECOVERY,

				service_not_found = EAI_SERVICE,
				socket_type_not_supported = EAI_SOCKTYPE

			};/*}}}*/
		}

		enum misc_errors
		{
			already_open = 1,
			eof,
			not_found,
			fd_set_failure,
			name_occupied,
			unknown
		};

		class BasicErrorCategory : public NonCopyable {
		public:
			virtual ~BasicErrorCategory();
			virtual const char* name() const;
			virtual const std::string message(int value);
		};

		class MiscErrorCategory: public BasicErrorCategory {
		public:
			const char* name() const;

			const std::string message(int value) {
				switch(value) {
				case already_open:
					return "Already open";
				case eof:
					return "End of file";
				case not_found:
					return "Not Found";
				case fd_set_failure:
					return "FD SET Failure";
				case name_occupied:
					return "name occupied";
				default:
					return "Unknown";
				}
			}
		};

		class ErrorCode {
		public:

			explicit ErrorCode(int error_code=0) {
				*this = error_code;
			}
			ErrorCode(int error_code, BasicErrorCategory& category): raw_error_code_(error_code), category_(&category) {}

			ErrorCode(const ErrorCode& ec) {*this = ec;}
			ErrorCode(const ErrorCode&& ec) {*this = ec;}
			ErrorCode& operator=(const ErrorCode &ec) {
				raw_error_code_ = ec.raw_error_code_;
				category_ = ec.category_;
				return *this;
			}

			template <typename ErrorCodeEnum>
				explicit ErrorCode(ErrorCodeEnum e) {
					*this = e;
				}

			template <typename ErrorCodeEnum>
				ErrorCode& operator=(ErrorCodeEnum e) {
					UNUSED(e);
					assert(false);
					return *this;
				}

			bool operator==(const ErrorCode &other) {
				return raw_error_code_ == other.raw_error_code_;
			}

			operator std::string() const {
				return message();
			}
			bool ok() const {
				return raw_error_code_ == 0;
			}

#define DEFINE_MAKE_ERROR_CODE_FUNCTION(Category, ErrorCodeEnum) \
			ErrorCode& operator=(ErrorCodeEnum e) { \
				static Category cat; \
				raw_error_code_ = static_cast<int>(e); \
				category_ = &cat; \
				return *this; \
			}

			DEFINE_MAKE_ERROR_CODE_FUNCTION(BasicErrorCategory, int)
			DEFINE_MAKE_ERROR_CODE_FUNCTION(BasicErrorCategory, generic_errors::basic)
			DEFINE_MAKE_ERROR_CODE_FUNCTION(MiscErrorCategory, misc_errors)

			inline const char* name() const {
				return category_->name();
			}

			inline const std::string message() const {
				return category_->message(raw_error_code_);
			}

			inline int error_code() const {
				return raw_error_code_;
			}
		private:
			int raw_error_code_;
			BasicErrorCategory *category_;
		};



	} /* utils */

} /* light */
//extern int errno;
#define LS_GENERIC_ERROR(err) light::utils::ErrorCode(static_cast<light::utils::generic_errors::basic>(err))
#define LS_GENERIC_ERR_OBJ(err) light::utils::ErrorCode(light::utils::generic_errors::basic::err)

#define LS_MISC_ERROR(err) light::utils::ErrorCode(static_cast<light::utils::misc_errors>(err))
#define LS_MISC_ERR_OBJ(err) light::utils::ErrorCode(light::utils::misc_errors::err)
#define LS_OK_ERROR() light::utils::ErrorCode(static_cast<light::utils::generic_errors::basic>(0))

