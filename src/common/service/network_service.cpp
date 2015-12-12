#include "enet/enet.h"
#include "network/acceptor.h"
#include "network/endpoint.h"
#include "network/tcp_connection.h"
#include "network/tcp_client.h"
#include "core/message_queue.h"
#include "core/service.h"
#include "service/network_service.h"
#include "utils/buffer.h"

namespace light {
namespace service {

#define GET_CONN_TYPE(key) (key >> CONN_TYPE_SHIFT)

light::utils::FixedAllocator<2 * 1024> NetworkService::fixed_alloc_;

static light::network::INetEndPointIpV4
ENetAddressToEndPoint(const ENetAddress &addr) {
  light::network::INetEndPointIpV4 v4;
  v4.set_addr_int(addr.host);
  v4.set_port(addr.port);
  return v4;
}

static light::network::INetEndPoint
get_tcp_peer_endpoint(light::network::TcpConnection *conn) {
  light::network::INetEndPoint point;
  auto iec = conn->get_peer_endpoint(point);
  if (iec) {
    DLOG(INFO) << "failed to get peer address Error:" << iec.message();
  }
  return point;
}

NetworkService::NetworkService(light::core::Context &ctx, int thread_count)
    : NetworkService(thread_count ? new light::network::Looper : &ctx.get_looper(), ctx.get_mq(), thread_count) {}

NetworkService::NetworkService(light::network::Looper *looper,
  light::core::MessageQueue &mq, int thread_count) : Service(*looper, mq), last_socket_id_(0), last_callback_idx_(0),
  loop_idx_(0), thread_count_(thread_count) {
  if (thread_count) {
    internal_looper_.reset(looper);
  }
}

NetworkService::~NetworkService() {

	DLOG(INFO) << __FUNCTION__ << " " << this;
}

light::utils::ErrorCode NetworkService::init() {
  if (enet_initialize() != 0) {
    DLOG(FATAL) << "An error occured while initializing ENet";
    return LS_MISC_ERR_OBJ(unknown);
  }
  loop_idx_ = get_looper().register_loop_callback(
      std::bind(&NetworkService::on_loop, this));
  for (int i = 0; i < thread_count_; ++i) {
    threads_.emplace_back([this]() {
      get_looper().loop();
    });
  }
  return LS_OK_ERROR();
}

void NetworkService::on_loop() {
  for (auto &kv : enet_host_map_) {
    ENetEvent event;

    enet_host_flush(kv.second.ptr.get());
    if (enet_host_service(kv.second.ptr.get(), &event, 0) > 0) {
      switch (event.type) {
      case ENET_EVENT_TYPE_CONNECT:
				DLOG(INFO) << "conn host service";
        on_connect(kv.first, *kv.second.ptr, event);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        on_receive(kv.first, *kv.second.ptr, event);
        break;
      case ENET_EVENT_TYPE_DISCONNECT:
        on_disconnect(kv.first, *kv.second.ptr, event);
        break;
      default:
        break;
      }
    }
  }
}

void NetworkService::forward_message(const NetworkServiceMessage &msg,
                                     uint32_t opaque) {
  auto message = get_mq().new_message();
  message->from = 0;
  message->type = light::core::MessageType::SOCKET;
  message->to = opaque;
  message->data = new NetworkServiceMessage(msg);
  message->size = sizeof(NetworkServiceMessage);
  message->destroy = [](light::core::LightMessage &self) {
    auto p = reinterpret_cast<NetworkServiceMessage *>(self.data);
    if (p->packet.destroy) {
      p->packet.destroy();
    }
    delete p;
  };
  get_mq().push_message(message);
}

void NetworkService::forward_data_message(
    NetworkServiceMessageType type, uint32_t opaque, uint32_t handle,
    const CommonPacket &packet, const light::network::INetEndPoint &peer) {
  NetworkServiceMessage msg;
  msg.type = type;
  msg.packet = packet;
  msg.endpoint = peer;
  msg.handle = handle;
  forward_message(msg, opaque);
}

void NetworkService::forward_error_message(
    NetworkServiceMessageType type, uint32_t opaque, uint32_t handle,
    const light::utils::ErrorCode &ec,
    const light::network::INetEndPoint &peer) {
  NetworkServiceMessage msg;
  msg.type = type;
  msg.ec = ec;
  msg.endpoint = peer;
  msg.handle = handle;
  forward_message(msg, opaque);
}

void NetworkService::forward_event_message(
    NetworkServiceMessageType type, uint32_t opaque, uint32_t handle,
    const light::network::INetEndPoint &peer) {
  NetworkServiceMessage msg;
  msg.type = type;
  msg.endpoint = peer;
  msg.handle = handle;
  forward_message(msg, opaque);
}

uint32_t NetworkService::install_udp_connection(uint32_t host_handle,
                                                ENetPeer *peer,
                                                uint32_t opaque) {
	UNUSED(host_handle);
  uint32_t key = ++last_socket_id_;
  key |= (CONN_TYPE_UDP_CLIENT << CONN_TYPE_SHIFT);

  peer->data = reinterpret_cast<void *>(key);

  enet_peer_map_[key] = std::make_tuple(peer, opaque);
  return key;
}

void NetworkService::on_connect(uint32_t handle, ENetHost &host,
                                const ENetEvent &event) {
  DLOG(INFO) << "on connect " << &host << " peer " << event.peer->data;
  auto udp_host = enet_host_map_[handle];
  assert(udp_host.ptr.get() == &host);
  auto it = connect_callbacks_.find(event.peer);
  if (it != connect_callbacks_.end()) {
    ENetPeer *peer = it->first;
    auto timer_id = std::get<0>(it->second);
    network_service_callback_t cb = std::get<1>(it->second);

    uint32_t opaque = std::get<2>(it->second);
    uint32_t key = install_udp_connection(handle, event.peer, opaque);
    cb(LS_OK_ERROR(), key);
    connect_callbacks_.erase(peer);

    // remove timeout timer
    light::utils::ErrorCode ec;
    get_looper().cancel_timer(ec, timer_id);
    if (ec) {
      DLOG(INFO) << "failed to remove timer: " << ec.message();
    }
  } else {
    uint32_t key = install_udp_connection(handle, event.peer, udp_host.opaque);
    forward_event_message(NetworkServiceMessageType::NET_MSG_TYPE_CONNECT,
                          udp_host.opaque, key,
                          ENetAddressToEndPoint(event.peer->address));
  }
}

void NetworkService::on_receive(uint32_t handle, ENetHost &host,
                                const ENetEvent &event) {
  UNUSED(handle);
  DLOG(INFO) << "receive data " << &host << " " << event.peer->data;
  uint32_t peer_handle = reinterpret_cast<uintptr_t>(event.peer->data);
  CommonPacket pkt;
  auto packet = event.packet;
  pkt.data = reinterpret_cast<char *>(packet->data);
  pkt.size = packet->dataLength;
  pkt.handle = peer_handle;
  pkt.destroy = [packet]() {
		enet_packet_destroy(packet);
	};
  auto opaque = std::get<1>(enet_peer_map_[peer_handle]);
  on_get_message_from_remote(peer_handle, pkt,
                             ENetAddressToEndPoint(event.peer->address), opaque);
}

void NetworkService::on_disconnect(uint32_t handle, ENetHost &host,
                                   const ENetEvent &event) {
  UNUSED(handle);
  uint32_t peer_handle = reinterpret_cast<uintptr_t>(event.peer->data);
  DLOG(INFO) << "on_disconnect " << peer_handle << " " << &host << " "
             << event.peer;
  enet_peer_reset(event.peer);
  // dont call internal_close!!
  if (active_close_handlers_.find(peer_handle) !=
      active_close_handlers_.end()) {
    active_close_handlers_.erase(peer_handle);
    // active close don't notify, already removed, is ok
  } else {
    // passive close
    auto opaque = std::get<1>(enet_peer_map_[peer_handle]);
    DLOG(INFO) << "passive close " << peer_handle << " " << opaque;
    forward_event_message(NetworkServiceMessageType::NET_MSG_TYPE_CLOSE,
		opaque, peer_handle,
                          ENetAddressToEndPoint(event.peer->address));
	enet_peer_map_.erase(peer_handle);
  }
}

light::utils::ErrorCode NetworkService::fini() {
  enet_deinitialize();
  if (loop_idx_) {
    get_looper().unregister_loop_callback(loop_idx_);
    loop_idx_ = 0;
  }
  if (thread_count_) {
    get_looper().stop();
    for(auto &thd : threads_) {
      thd.join();
    }
  }
  return LS_OK_ERROR();
}

void NetworkService::create_tcp_server(
    const light::network::INetEndPoint &endpoint, int backlog,
    network_service_callback_t func, uint32_t opaque) { /*{{{*/
	std::shared_ptr<light::network::Acceptor> acceptor(new light::network::Acceptor(get_looper()), [](light::network::Acceptor *acc)
	{
		acc->close();
		delete acc;
	});
      
  auto ec = acceptor->open(endpoint.get_protocol());
  if (ec)
    func(ec, 0);
  ec = acceptor->set_reuseaddr(1);
  if (ec)
    func(ec, 0);
  ec = acceptor->bind(endpoint);
  if (ec)
    func(ec, 0);
  ec = acceptor->listen(backlog);
  if (ec)
    func(ec, 0);

  uint32_t key = ++last_socket_id_;
  key |= (CONN_TYPE_TCP_SERVER << CONN_TYPE_SHIFT);
  acceptor_map_[key] = ConnectionContainer<light::network::Acceptor>(acceptor, opaque);
  acceptor->set_accept_handler(
      [this, opaque](const light::utils::ErrorCode &aec, int fd) {
        if (!aec) {
          uint32_t tcp_key;
          light::network::TcpConnection *conn;
          std::tie(tcp_key, conn) = install_tcp_connection(fd, opaque);
          forward_event_message(NetworkServiceMessageType::NET_MSG_TYPE_CONNECT,
                                opaque, tcp_key, get_tcp_peer_endpoint(conn));
        } else {
          DLOG(INFO) << "Error while accept: " << aec.message();
        }
      });
  func(ec, key);
} /*}}}*/

void NetworkService::create_udp_server(
    const light::network::INetEndPoint &point, int max_peer, int max_channel,
    network_service_callback_t func, uint32_t opaque) { /*{{{*/
  int addr = point.get_addr_int();
  if (addr < 0) {
    func(LS_GENERIC_ERR_OBJ(address_family_not_supported), 0);
    return;
  }

  ENetAddress address;
  ENetHost *server;
  address.host = point.get_addr_int();
  address.port = point.get_port();
  server = enet_host_create(&address, max_peer, max_channel, 0, 0);
  if (server == nullptr) {
    func(LS_MISC_ERR_OBJ(unknown), 0);
  }

  uint32_t key = ++last_socket_id_;
  key |= (CONN_TYPE_UDP_SERVER << CONN_TYPE_SHIFT);
  std::shared_ptr<ENetHost> enet_host(server, [this](ENetHost* h)
  {
	  enet_host_destroy(h);
  });
  enet_host_map_[key] = ConnectionContainer<ENetHost>(enet_host, opaque);
  DLOG(INFO) << "create_udp_server " << key;
  func(LS_OK_ERROR(), key);
} /*}}}*/

void NetworkService::on_get_message_from_remote(
    uint32_t handle, const CommonPacket &pkt,
    const light::network::INetEndPoint &endpoint, uint32_t opaque) {
  forward_data_message(NetworkServiceMessageType::NET_MSG_TYPE_DATA,
                       opaque, handle, pkt, endpoint);
}

void NetworkService::async_read_tcp_connection(
    light::network::TcpConnection *conn, uint32_t handle) {
  // use fixed allocator
	std::shared_ptr<char> buf_ptr(fixed_alloc_.alloc(), [this](char *buf){
				fixed_alloc_.dealloc(buf);
		}); // 2M

  conn->async_read_some(
      buf_ptr.get(), fixed_alloc_.node_size(),
      [this, conn, handle, buf_ptr](light::utils::ErrorCode ec, size_t bytes_read) {
				char *buf = buf_ptr.get();
        if (!ec) {
          CommonPacket pkt;
          pkt.data = buf;
          pkt.size = bytes_read;
          pkt.handle = handle;
		  auto opaque = tcp_connection_map_[handle].opaque;
          this->on_get_message_from_remote(handle, pkt,
                                           get_tcp_peer_endpoint(conn), opaque);
          this->async_read_tcp_connection(conn, handle);
        } else {
          handle_tcp_error(handle, ec);
        }
      });
}

void NetworkService::handle_tcp_error(uint32_t handle,
                                      const light::utils::ErrorCode &ec) {
  auto conn = tcp_connection_map_[handle];
  forward_error_message(NetworkServiceMessageType::NET_MSG_TYPE_EXECPTION,
                        conn.opaque, handle, ec,
                        get_tcp_peer_endpoint(conn.ptr.get()));
  internal_close(handle, false);
}

void NetworkService::handle_tcp_close(uint32_t handle) {
  auto conn = tcp_connection_map_[handle];
  forward_event_message(NetworkServiceMessageType::NET_MSG_TYPE_CLOSE,
                        conn.opaque, handle,
                        get_tcp_peer_endpoint(conn.ptr.get()));
  internal_close(handle, false);
}

std::tuple<uint32_t, light::network::TcpConnection *>
NetworkService::install_tcp_connection(int sockfd, uint32_t opaque) {
  uint32_t key = ++last_socket_id_;
  key |= (CONN_TYPE_TCP_CLIENT << CONN_TYPE_SHIFT);
  auto conn = new light::network::TcpConnection(get_looper(), sockfd);
  tcp_connection_map_[key] = ConnectionContainer<light::network::TcpConnection>(std::shared_ptr<light::network::TcpConnection>(conn, [](light::network::TcpConnection *p)
  {
	  p->close();
	  delete p;
  }), opaque);
  this->async_read_tcp_connection(conn, key);
  conn->set_error_callback([conn, this, key]() {
    auto ec = conn->get_last_error();
    handle_tcp_error(key, ec);
  });
  conn->set_close_callback([conn, this, key]() { handle_tcp_close(key); });
  return std::make_tuple(key, conn);
}

void NetworkService::connect_tcp_server(
    const light::network::INetEndPoint &point, uint64_t micro_sec,
    network_service_callback_t func, uint32_t opaque) { /*{{{*/
	auto tcp_client = 
      new light::network::TcpClient(get_looper());
  auto tmp_ec = tcp_client->open(point.get_protocol());
  if (tmp_ec) {
    func(tmp_ec, 0);
    return;
  }
  tcp_client->async_connect(
      point,
      [this, tcp_client, func, opaque](const light::utils::ErrorCode &ec) {
				uint32_t key = 0;
        if (!ec) {
          light::network::TcpConnection *conn;
          std::tie(key, conn) =
              install_tcp_connection(tcp_client->get_sockfd(), opaque);
        }
				// may be dangerous, but now ok, after all called, tcp_client is not
				// touched
				func(ec, key);
				delete tcp_client;
      },
      micro_sec);
} /*}}}*/

void NetworkService::create_udp_stub(int max_peer, int max_channel,
                                     network_service_callback_t func,
                                     uint32_t opaque) { /*{{{*/
  DLOG(INFO) << "udp stub";

  ENetHost *client;
  client = enet_host_create(nullptr, max_peer, max_channel, 0, 0);
  if (client == nullptr) {
    func(LS_MISC_ERR_OBJ(unknown), 0);
  }

  uint32_t key = ++last_socket_id_;
  key |= (CONN_TYPE_UDP_SERVER << CONN_TYPE_SHIFT);
  std::shared_ptr<ENetHost> enet_host(client, [](ENetHost *c)
  {
	  enet_host_destroy(c);
  });
  enet_host_map_[key] = ConnectionContainer<ENetHost>(enet_host, opaque);
  func(LS_OK_ERROR(), key);
} /*}}}*/

void NetworkService::connect_udp_server(
    const light::network::INetEndPoint &point, uint64_t micro_sec,
    int32_t stub_id, int channels, network_service_callback_t func,
    uint32_t opaque) { /*{{{*/
	DLOG(INFO) << __FUNCTION__ << " " << this;
  auto host = enet_host_map_.find(stub_id);
  if (host == enet_host_map_.end()) {
    func(LS_GENERIC_ERR_OBJ(invalid_argument), 0);
  }

  ENetAddress address;
  ENetPeer *peer;
  address.host = point.get_addr_int();
  address.port = point.get_port();
  ++last_callback_idx_;
  peer = enet_host_connect(host->second.ptr.get(), &address, channels,
                           last_callback_idx_);
  if (peer == nullptr) {
    func(LS_GENERIC_ERR_OBJ(too_many_files_open), 0);
  }
  light::utils::ErrorCode ec;
  auto tid = get_looper().add_timer(ec, micro_sec, 0, [peer, this]() {
    auto it = connect_callbacks_.find(peer);
    if (it == connect_callbacks_.end()) {
      DLOG(FATAL) << "peer not found!! " << peer;
      return;
    }
    enet_peer_reset(peer);
    std::get<1>(it->second)(LS_GENERIC_ERR_OBJ(timed_out), 0);
    connect_callbacks_.erase(peer);
  });
  if (ec) {
    func(ec, 0);
  } else {
    connect_callbacks_[peer] = std::make_tuple(tid, func, opaque);
  }
} /*}}}*/

void NetworkService::close(uint32_t handle) { internal_close(handle, true); }

void NetworkService::internal_close(uint32_t handle,
                                    bool active_close) { /*{{{*/
  if (!check_handle_exists(handle))
    return;
  switch (GET_CONN_TYPE(handle)) {
  case CONN_TYPE_TCP_SERVER: {
	 acceptor_map_.erase(handle);
  } break;

  case CONN_TYPE_UDP_SERVER: {
	 enet_host_map_.erase(handle);
  } break;

  case CONN_TYPE_TCP_CLIENT: {
	 tcp_connection_map_.erase(handle);
  } break;

  case CONN_TYPE_UDP_CLIENT: {
    ENetPeer *peer = std::get<0>(enet_peer_map_[handle]);
    enet_peer_disconnect(peer, 0);
    if (active_close) {
      active_close_handlers_.insert(handle);
	  enet_peer_map_.erase(handle);
    }

  } break;
  default:
    break;
  }
} /*}}}*/

/**
 * @brief 发送一个packet
 *
 * @param packet 传输的packet
 * @param reliable 是否是可靠的包，仅针对UDP
 */
void NetworkService::send_common_packet(CommonPacket packet,
                                        bool reliable, int channel) { /*{{{*/
  if (!check_handle_exists(packet.handle)) {
    LOG(FATAL) << "handle not exists handle_id: " << packet.handle;
    return;
  }

  switch (GET_CONN_TYPE(packet.handle)) {
  case CONN_TYPE_TCP_CLIENT: {
    auto conn = tcp_connection_map_[packet.handle];
    conn.ptr->async_write(packet.data, packet.size,
                               [packet] { packet.destroy(); });
  } break;
  case CONN_TYPE_UDP_CLIENT: {
    // use packet pool?
    ENetPacket *enet_pkt = enet_packet_create(
        packet.data, packet.size,
        (reliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNSEQUENCED) |
            ENET_PACKET_FLAG_NO_ALLOCATE);
    enet_pkt->userData = new CommonPacket(packet);

    enet_pkt->freeCallback = [](ENetPacket *pkt) {
      auto cp = reinterpret_cast<CommonPacket *>(pkt->userData);
      cp->destroy();
      delete cp;
    };

    ENetPeer *peer = std::get<0>(enet_peer_map_[packet.handle]);
    enet_peer_send(peer, channel, enet_pkt);

  } break;
  default:
    DLOG(FATAL) << "apply send packet to wrong socket type";
    break;
  }
} /*}}}*/

bool NetworkService::check_handle_exists(uint32_t handle) {
  switch(GET_CONN_TYPE(handle))
  {
  case CONN_TYPE_TCP_CLIENT:
	  return tcp_connection_map_.find(handle) != tcp_connection_map_.end();
  case CONN_TYPE_UDP_CLIENT:
	  return enet_peer_map_.find(handle) != enet_peer_map_.end();
  case CONN_TYPE_TCP_SERVER:
	  return acceptor_map_.find(handle) != acceptor_map_.end();
  case CONN_TYPE_UDP_SERVER:
	  return enet_host_map_.find(handle) != enet_host_map_.end();
  }
  return false;
}
} /* core */
} /* light */
