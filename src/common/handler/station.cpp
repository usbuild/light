#include "handler/station.h"
#include "proto/station_generated.h"

namespace light {
namespace handler {

const char *Station::name = "station";
#define HANDLER_LOG(x)                                                         \
  DLOG(x) << name << "[" << static_cast<int>(station_id_) << "] "

using nsm_t = light::service::NetworkServiceMessage;
using nsmt_t = light::service::NetworkServiceMessageType;

Station::Station(uint8_t station_id,
                 const light::network::INetEndPoint &endpoint,
                 network_service_ptr_t network_service)
    : station_id_(station_id), endpoint_(endpoint),
      network_service_(network_service) {}

Station::~Station() {}

light::utils::ErrorCode Station::init() { return LS_OK_ERROR(); }

void Station::on_install() {
  using ns_t = light::service::NetworkService;
  network_service_->post<ns_t>(
      &ns_t::create_tcp_server, endpoint_, 5,
      ctx_->get_looper().wrap([this](light::utils::ErrorCode ec,
                                     uint32_t handle) {
        UNUSED(handle);
        if (!ec.ok()) {
          HANDLER_LOG(INFO)
              << "unable to create tcp server, will exit or restart";
        } else {
          HANDLER_LOG(INFO) << "server listening on : " << endpoint_.to_string()
                            << ":" << endpoint_.get_port();
        }
      }),
      id_);
}

void Station::connect_to_station(const light::network::INetEndPoint &point) {
  using ns_t = light::service::NetworkService;
  network_service_->post<ns_t>(
      &ns_t::connect_tcp_server, point, 5000000LL,
      ctx_->get_looper().wrap([this, point](light::utils::ErrorCode ec,
                                            uint32_t handle) {
        UNUSED(handle);
        if (ec.ok()) {
          HANDLER_LOG(INFO) << "connect: " << point.to_string() << ":"
                            << point.get_port() << " " << ec.message();
        } else {
          HANDLER_LOG(INFO) << "error while connect: " << point.to_string()
                            << ":" << point.get_port() << " " << ec.message();
        }
      }),
      id_);
}

void Station::post_message(light::core::light_message_ptr_t msg) {
  if (msg->type == light::core::MessageType::SOCKET) {
    handle_socket_message(msg);
  } else {
    handle_local_message(msg);
  }
}

light::service::CommonPacket Station::generate_network_packet(
    light::core::mq_handler_id_t to, light::core::mq_handler_id_t from,
    uint8_t type, uint32_t handle, flatbuffers::FlatBufferBuilder &fbb) {

  using pmc_t = light::core::PackedMessage;

  light::service::CommonPacket pkt;
  pkt.size = fbb.GetSize() + sizeof(pmc_t);
  pkt.data = new char[pkt.size];
  pkt.handle = handle;

  pmc_t *pmsg = reinterpret_cast<pmc_t *>(pkt.data);

  pmsg->to = to;
  pmsg->from = from;
  pmsg->type = type;
  pmsg->size = fbb.GetSize();
  ::memcpy(pkt.data + sizeof(pmc_t), fbb.GetBufferPointer(), fbb.GetSize());

  char *ptr = pkt.data;
  pkt.destroy = [ptr]() { delete[] ptr; };
  return pkt;
}

void Station::handle_socket_message(light::core::light_message_ptr_t msg) {
  auto nsm = static_cast<nsm_t *>(msg->data);
  switch (nsm->type) {
  case nsmt_t::NET_MSG_TYPE_CONNECT: {
    HANDLER_LOG(INFO) << "got connection from: " << nsm->endpoint.to_string()
                      << ":" << nsm->endpoint.get_port();
    register_to_station(nsm->handle, true);
  } break;

  case nsmt_t::NET_MSG_TYPE_DATA:
    add_partial_message(msg);
    break;

  case nsmt_t::NET_MSG_TYPE_EXECPTION:
    HANDLER_LOG(WARNING) << "connection exception " << nsm->ec.message();

  case nsmt_t::NET_MSG_TYPE_CLOSE:
    LOG(WARNING) << "connection closed " << nsm->endpoint.to_string() << ":"
                 << nsm->endpoint.get_port();
    {
      auto handle = nsm->handle;
      auto it = other_conn_station_.find(handle);
      if (it != other_conn_station_.end()) {
        other_station_conn_.erase(it->second);
        other_conn_station_.erase(it);
      }
    }
    break;

  default:
    break;
  }
}

void Station::register_to_station(uint32_t target_handle, bool need_back) {
  flatbuffers::FlatBufferBuilder fbb;
  light::proto::ConnectRequestBuilder crb(fbb);
  crb.add_my_id(station_id_);
  crb.add_need_back(need_back);
  auto mloc = crb.Finish();
  fbb.Finish(mloc);
  auto pkt = generate_network_packet(
      0, station_id_, StationMessageType::REGISTER, target_handle, fbb);
  network_service_->send_common_packet(pkt, true, 0);
}

void Station::handle_local_message(light::core::light_message_ptr_t msg) {
  UNUSED(msg);
}

void Station::handle_inter_station_message(light::core::light_message_ptr_t msg,
                                           uint32_t handle) {
  if (msg->type == StationMessageType::REGISTER) {
    auto conn_request =
        flatbuffers::GetRoot<light::proto::ConnectRequest>(msg->data);
    auto it = other_station_conn_.find(conn_request->my_id());
    if (it == other_station_conn_.end() || !std::get<1>(it->second)) {
      other_station_conn_[conn_request->my_id()] = std::make_pair(handle, true);
      if (conn_request->need_back()) {
        register_to_station(handle, false);
      }
      other_conn_station_[handle] = conn_request->my_id();
      HANDLER_LOG(INFO) << "get register info from station_id="
                        << conn_request->my_id();
    } else {
      HANDLER_LOG(INFO) << "station with id=" << conn_request->my_id()
                        << " already registered";
    }
  }
}

void Station::add_partial_message(light::core::light_message_ptr_t msg) {
  auto nsm = static_cast<nsm_t *>(msg->data);
  auto pkg = nsm->packet;
  light::core::mq_handler_id_t conn_id = pkg.handle;
  char *data = static_cast<char *>(pkg.data);
  size_t size = pkg.size;
  auto partial = other_station_data_[conn_id];

  size_t total_read_size = 0;
  while (total_read_size < size) {
    total_read_size += partial.input_buffer(data, size - total_read_size);
    if (partial.is_ready()) {
      auto lmsg = ctx_->get_mq().new_message();
      partial.to_message(*lmsg);
      // this is for station, don't forward
      if (lmsg->to == 0) {
        handle_inter_station_message(lmsg, conn_id);
      } else {
        ctx_->push_message(lmsg);
      }
    } else {
      break;
    }
  }
}

void Station::on_unstall() {}

void Station::fini() {}

} /* handler */
} /* light */
