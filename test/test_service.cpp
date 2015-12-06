#include <gtest/gtest.h>
#include "service/network_service.h"
#include "core/default_context_loader.h"
#include <thread>

using namespace light::core;
using namespace light::service;
using namespace light::network;

class Shuttle : public MessageHandler {
public:
  Shuttle(Context &ctx, std::shared_ptr<NetworkService> ns) : MessageHandler(), ns_(ns) {
	}

  virtual light::utils::ErrorCode init() {
    DLOG(INFO) << "shuttle inited!";
    return LS_OK_ERROR();
  }


  virtual void on_install() {
    DLOG(INFO) << "shuttle installed!";
#if 1
    ns_->post<NetworkService>(
        &NetworkService::create_udp_server, INetEndPoint(protocol::v4(), 8889),
        5, 16, ctx_->get_looper().wrap([this](light::utils::ErrorCode ec,
                                              uint32_t handle) {
          UNUSED(handle);
          DLOG(INFO) << "create udp server " << ec.message();
          ns_->post<NetworkService>(
              &NetworkService::create_udp_stub, 10, 16,
              ctx_->get_looper().wrap([this](light::utils::ErrorCode ec,
                                             uint32_t stub_id) {
                DLOG(INFO) << "create udp client stub " << ec.message() << " "
                           << stub_id;
                ns_->post<NetworkService>(
                    &NetworkService::connect_udp_server,
                    INetEndPoint("127.0.0.1", 8889), 5000000LL, stub_id, 8,
                    ctx_->get_looper().wrap([this](light::utils::ErrorCode ec,
                                                   uint32_t handle) {
                      DLOG(INFO) << "connect_udp_server " << ec.message();
                      CommonPacket pkt;
                      pkt.handle = handle;
                      char *data = new char[100]{0};
                      ::strcpy(data, "Hello UDP client");
                      pkt.data = data;
                      pkt.size = 100;
                      pkt.destroy = [data] { delete[] data; };
                      DLOG(INFO) << "send udp packet " << handle;
                      ns_->post<NetworkService>(
                          &NetworkService::send_common_packet, pkt, true, 0);
                      light::utils::ErrorCode ec2;
                      ns_->get_looper().add_timer(
                          ec2, 1000000LL, 0, [this, handle] {
                            ns_->post<NetworkService>(&NetworkService::close,
                                                      handle);
                          });
                    }),
                    id_);
              }),
              id_);
        }),
        id_);
#endif

#if 1
    ns_->post<NetworkService>(
        &NetworkService::create_tcp_server, INetEndPoint(protocol::v4(), 8890),
        5, ctx_->get_looper().wrap([this](light::utils::ErrorCode ec,
                                          uint32_t handle) {
          UNUSED(handle);
          DLOG(INFO) << "create tcp server " << ec.message();
          ns_->post<NetworkService>(
              &NetworkService::connect_tcp_server,
              INetEndPoint("127.0.0.1", 8890), 5000000LL,
              ctx_->get_looper().wrap([this](light::utils::ErrorCode ec,
                                             uint32_t handle) {
                DLOG(INFO) << "connect_tcp_server " << ec.message() << " handle " << handle;
                CommonPacket pkt;
                pkt.handle = handle;
                char *data = new char[100]{0};
                ::strcpy(data, "Hello World");
                pkt.data = data;
                pkt.size = 100;
                pkt.destroy = [data] {
                  delete[] data;
                  DLOG(INFO) << "packet sent";
                };
                DLOG(INFO) << "send packet " << handle;
                ns_->post<NetworkService>(&NetworkService::send_common_packet,
                                          pkt, true, 0);
              }),
              id_);
        }),
        id_);
#endif
  }

  virtual void post_message(light_message_ptr_t msg) {
    DLOG(INFO) << "GET_MESSAGE!";
    auto *nsm = static_cast<NetworkServiceMessage *>(msg->data);
    DLOG(INFO) << "from: " << nsm->endpoint.to_string() << ":"
               << nsm->endpoint.get_port() << " type: " << nsm->type
               << " size: " << nsm->packet.size;
    switch (nsm->type) {
    case NetworkServiceMessageType::NET_MSG_TYPE_DATA:
      DLOG(INFO) << " content " << static_cast<char *>(nsm->packet.data);
      break;
    case NetworkServiceMessageType::NET_MSG_TYPE_CONNECT:
      DLOG(INFO) << " connect~! ";
      break;
    case NetworkServiceMessageType::NET_MSG_TYPE_CLOSE:
      DLOG(INFO) << " close~! ";
      break;
    case NetworkServiceMessageType::NET_MSG_TYPE_EXECPTION:
      DLOG(INFO) << " error_code: " << nsm->ec.message();
      break;
    default:
      break;
    }
  }
  virtual void on_unstall() { DLOG(INFO) << "shuttle uninstalled!"; }
  virtual void fini() {
    ns_->fini();
    DLOG(INFO) << "shuttle finalize!";
  }

private:
  std::shared_ptr<NetworkService> ns_;
};

TEST(Service, demo) {
  Context ctx;

  Shuttle shuttle(ctx, DefaultContextLoader::instance().require_service<NetworkService>(ctx, "network", "shuttle-network", ctx, 0));
  ctx.install_handler(shuttle);
  shuttle.init();

  light::utils::ErrorCode ec;
  ctx.get_looper().add_timer(ec, 3000000LL, 0,
                             [&ctx] { ctx.get_looper().stop(); });
	ctx.get_looper().loop();

	/*
  const int main_thread_num = 4;
  std::thread *ths = new std::thread[main_thread_num];
  for (int i = 0; i < main_thread_num; ++i) {
    ths[i] = std::thread([&ctx] { ctx.get_looper().loop(); });
  }
  for (int i = 0; i < main_thread_num; ++i) {
    ths[i].join();
  }
  delete[] ths;
	*/
}
