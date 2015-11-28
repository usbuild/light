#include "flatbuffers/flatbuffers.h"
#include <algorithm>
#include "core/handler.h"
#include "network/endpoint.h"
#include "service/network_service.h"
#include "utils/buffer.h"

namespace light {
namespace handler {

enum StationMessageType {
  ALL_MSG_BASE = 100,
  REGISTER = 101,
};

struct PartialPacketMessage {
  union {
    light::core::PackedMessage pm;
    char buf[sizeof(light::core::PackedMessage)];
  };
  char *data = nullptr;
  uint32_t read_size = 0;

  inline bool header_ready() const { return data != nullptr; }

  inline size_t need_header_size() const {
    if (header_ready())
      return 0;
    return sizeof(pm) - read_size;
  }

  inline bool is_ready() const {
    return header_ready() && read_size == pm.size;
  }

  size_t input_buffer(char *buffer, size_t len) {
    size_t consumed = 0;
    if (!header_ready()) {
      consumed = (std::min)(len, need_header_size());
      ::memcpy(buf + read_size, buffer, consumed);
      read_size += consumed;
      if (read_size == sizeof(pm)) {
        data = new char[pm.size];
      }
      read_size = 0;
    }
    if (!header_ready())
      return consumed;

    size_t data_read =
        (std::min)(static_cast<size_t>(pm.size - read_size), len - consumed);
    ::memcpy(data + read_size, buffer + consumed, data_read);
    read_size += data_read;
    return consumed;
  }

  void to_message(light::core::LightMessage &msg) {
    msg.to = pm.to;
    msg.from = pm.from;
    msg.type = pm.type;
    msg.data = data;
    msg.size = pm.size;
    msg.destroy = [](light::core::LightMessage &self) {
      delete[] static_cast<char *>(self.data);
    };
    memset(this, 0, sizeof(*this));
  }
};

class Station : public light::core::MessageHandler {

  typedef uint32_t rpc_size_t;

  typedef std::shared_ptr<light::service::NetworkService> network_service_ptr_t;

public:
  Station(uint8_t station_id, const light::network::INetEndPoint &endpoint,
          network_service_ptr_t network_service);
  ~Station();

  light::utils::ErrorCode init() override;
  void on_install() override;
  void post_message(light::core::light_message_ptr_t msg) override;
  void on_unstall() override;
  void fini() override;

  // for test
  void connect_to_station(const light::network::INetEndPoint &point);

  static const char *name;

private:
  void handle_socket_message(light::core::light_message_ptr_t msg);

  void add_partial_message(light::core::light_message_ptr_t msg);
  void register_to_station(uint32_t target_handle, bool need_back);
  void handle_local_message(light::core::light_message_ptr_t msg);
  void handle_inter_station_message(light::core::light_message_ptr_t msg,
                                    uint32_t handle);

  light::service::CommonPacket
  generate_network_packet(light::core::mq_handler_id_t to,
                          light::core::mq_handler_id_t from, uint8_t type,
                          uint32_t handle, flatbuffers::FlatBufferBuilder &fbb);

private:
  uint8_t station_id_;
  light::network::INetEndPoint endpoint_;
  network_service_ptr_t network_service_;

  std::map<uint8_t, std::tuple<uint32_t, bool>> other_station_conn_;
  std::map<uint32_t, uint8_t> other_conn_station_;

  std::map<light::core::mq_handler_id_t, PartialPacketMessage>
      other_station_data_;
};

} /* handler */
} /* light */
