#pragma once
#include "config.h"
#include <deque>
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#include <stdint.h>
#include "network/connection.h"
#include "network/dispatcher.h"
#include "network/socket.h"
namespace light {
namespace network {

class Looper;
class Dispatcher;

class TcpConnection : public TcpSocket, public Connection {

public:
  TcpConnection(Looper &looper);

  TcpConnection(Looper &looper, int fd);

  virtual ~TcpConnection();

  void buffer_write_callback();

  light::utils::ErrorCode close();

  template <typename T> void set_error_callback(T &&t);

  template <typename T> void set_close_callback(T &&t);

  template <typename ReadCallback>
  void async_read(void *read_buf, size_t bytes_to_read, ReadCallback cb);

  template <typename ReadCallback>
  void async_read_some(void *read_buf, size_t buf_len, ReadCallback cb);

  void async_write(void *buf, size_t len, const write_callback_t &func);

  light::utils::ErrorCode
  get_peer_endpoint(light::network::INetEndPoint &endpoint);

protected:
  std::unique_ptr<Dispatcher> dispatcher_;
  WriteBuffer write_buffer_;
  size_t bytes_has_read_;
  light::network::INetEndPoint peer_point_;
};
template <typename T> void TcpConnection::set_error_callback(T &&t) {
  dispatcher_->set_error_callback(std::forward<T>(t));
}
template <typename T> void TcpConnection::set_close_callback(T &&t) {
  dispatcher_->set_close_callback(std::forward<T>(t));
}

template <typename ReadCallback>
void TcpConnection::async_read(void *read_buf, size_t bytes_to_read,
                               ReadCallback cb) {
  bytes_has_read_ = 0;
  dispatcher_->enable_read();
  dispatcher_->set_read_callback([this, bytes_to_read, read_buf, cb] {
    assert(bytes_to_read > bytes_has_read_);

    size_t bytes_left = bytes_to_read - bytes_has_read_;
    ssize_t read_bytes =
        ::recv(this->sockfd_, static_cast<char *>(read_buf) + bytes_has_read_,
               bytes_left, 0);
    if (read_bytes < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {

      } else {
        LOG(WARNING) << "read failed: " << LS_GENERIC_ERROR(errno).message();
      }
    } else if (read_bytes == 0) {
      dispatcher_->disable_read();
    } else {
      bytes_has_read_ += read_bytes;
      if (bytes_has_read_ == bytes_to_read) {
        dispatcher_->disable_read();
        bytes_has_read_ = 0;
        cb();
      }
    }
  });
}

template <typename ReadCallback>
void TcpConnection::async_read_some(void *read_buf, size_t buf_len,
                                    ReadCallback cb) {
  dispatcher_->enable_read();

  dispatcher_->set_read_callback([this, read_buf, buf_len, cb] {
    ssize_t read_bytes =
        ::recv(this->sockfd_, static_cast<char *>(read_buf), buf_len, 0);
    dispatcher_->disable_read();
    if (read_bytes < 0) {
      cb(LS_GENERIC_ERROR(errno), 0);
    } else if (read_bytes == 0) {
      cb(LS_MISC_ERR_OBJ(eof), 0);
    } else {
      cb(LS_OK_ERROR(), read_bytes);
    }
  });
}

} /* network */

} /* light */
