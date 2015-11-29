#pragma once
#include "enet/enet.h"
#include "network/acceptor.h"
#include "network/endpoint.h"
#include "network/tcp_connection.h"
#include "network/tcp_client.h"
#include "core/message.h"
#include "core/service.h"
#include "utils/allocator.h"
#include "utils/buffer.h"

namespace light {
namespace service {

typedef int socket_handle_t;

struct CommonPacket {
  light::core::mq_handler_id_t handle;
  char *data;
  size_t size;
  std::function<void()> destroy;
};

enum NetworkServiceMessageType {
  NET_MSG_TYPE_CONNECT,
  NET_MSG_TYPE_DATA,
  NET_MSG_TYPE_EXECPTION,
  NET_MSG_TYPE_CLOSE
};

struct NetworkServiceMessage {
  NetworkServiceMessageType type;
  uint32_t handle;
  light::utils::ErrorCode ec;
  CommonPacket packet;
  light::network::INetEndPoint endpoint;
};

class NetworkService : public light::core::Service {

  enum {
    CONN_TYPE_SHIFT = 29,
    CONN_TYPE_TCP_SERVER = 1,
    CONN_TYPE_UDP_SERVER = 2,
    CONN_TYPE_TCP_CLIENT = 3,
    CONN_TYPE_UDP_CLIENT = 4,
  };

public:
  typedef std::function<void(light::utils::ErrorCode, uint32_t)>
      network_service_callback_t;

public:
  NetworkService(light::core::Context &ctx);
  NetworkService(light::network::Looper &looper, light::core::MessageQueue &mq);

  light::utils::ErrorCode init();

  light::utils::ErrorCode fini();

  void create_tcp_server(const light::network::INetEndPoint &endpoint,
                         int backlog, network_service_callback_t func,
                         uint32_t opaque);

  void create_udp_server(const light::network::INetEndPoint &point,
                         int max_peer, int max_channel,
                         network_service_callback_t func, uint32_t opaque);

  void connect_tcp_server(const light::network::INetEndPoint &point,
                          uint64_t micro_sec, network_service_callback_t func,
                          uint32_t opaque);

  void create_udp_stub(int max_peer, int max_channel,
                       network_service_callback_t func, uint32_t opaque);

  void connect_udp_server(const light::network::INetEndPoint &point,
                          uint64_t micro_sec, int32_t stub_id, int channels,
                          network_service_callback_t func, uint32_t opaque);

  void close(uint32_t handle);

  /**
   * @brief 发送一个packet
   *
   * @param packet 传输的packet
   * @param reliable 是否是可靠的包，仅针对UDP
   */
  void send_common_packet(const CommonPacket &packet, bool reliable,
                          int channel = 0);

private:
  bool check_handle_exists(uint32_t handle);

  void on_loop();

  void forward_message(const NetworkServiceMessage &msg, uint32_t opaque);

  void forward_data_message(NetworkServiceMessageType type, uint32_t opaque,
                            uint32_t handle, const CommonPacket &packet,
                            const light::network::INetEndPoint &peer);

  void forward_error_message(NetworkServiceMessageType type, uint32_t opaque,
                             uint32_t handle, const light::utils::ErrorCode &ec,
                             const light::network::INetEndPoint &peer);

  void forward_event_message(NetworkServiceMessageType type, uint32_t opaque,
                             uint32_t handle,
                             const light::network::INetEndPoint &peer);

  uint32_t install_udp_connection(uint32_t host_handle, ENetPeer *peer,
                                  uint32_t opaque);

  void on_connect(uint32_t handle, ENetHost &host, const ENetEvent &event);

  void on_receive(uint32_t handle, ENetHost &host, const ENetEvent &event);

  void on_disconnect(uint32_t handle, ENetHost &host, const ENetEvent &event);

  void on_get_message_from_remote(uint32_t handle, const CommonPacket &pkt,
                                  const light::network::INetEndPoint &point, uint32_t opque);

  void handle_tcp_error(uint32_t handle, const light::utils::ErrorCode &ec);

  void handle_tcp_close(uint32_t handle);

  void async_read_tcp_connection(light::network::TcpConnection *conn,
                                 uint32_t handle);

  std::tuple<uint32_t, light::network::TcpConnection *>
  install_tcp_connection(int sockfd, uint32_t opaque);

  void internal_close(uint32_t handle, bool active_close = false);

public:
  constexpr static const char *name = "network";

private:

  template<typename T>
  struct ConnectionContainer
  {
	  ConnectionContainer() = default;
	  ConnectionContainer(std::shared_ptr<T> p, uint32_t opa):ptr(p), opaque(opa) {}

	  std::shared_ptr<T> ptr;
	  uint32_t opaque;
  };

  std::unordered_map<uint32_t, ConnectionContainer<light::network::Acceptor> > acceptor_map_;
  std::unordered_map<uint32_t, ConnectionContainer<ENetHost> > enet_host_map_;
  std::unordered_map<uint32_t, ConnectionContainer<light::network::TcpConnection> > tcp_connection_map_;
  std::unordered_map<uint32_t, std::tuple<ENetPeer*, uint32_t> > enet_peer_map_;

  std::unordered_map<
      ENetPeer *,
      std::tuple<light::network::TimerId, network_service_callback_t, uint32_t>>
      connect_callbacks_;
  uint32_t last_socket_id_;
  uint32_t last_callback_idx_;
  int loop_idx_;
  light::utils::FixedAllocator<2 * 1024> fixed_alloc_;
  std::set<uint32_t> active_close_handlers_;
};

} /* core */
} /* light */
