#include "utils/helpers.h"
#include "network/looper.h"
#include "network/acceptor.h"

int main(int argc, const char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);
	light::utils::SocketGlobalInitialize();
	light::network::Looper looper;
	light::network::Acceptor acceptor(looper);
	light::network::INetEndPoint endpoint(light::network::protocol::v4(), 8888);
	acceptor.open(endpoint.get_protocol());
	acceptor.set_reuseaddr(1);
	acceptor.bind(endpoint);
	acceptor.listen();

	acceptor.set_accept_handler([&looper](light::utils::ErrorCode &ec, int fd) {
		UNUSED(ec);
		DLOG(INFO) << "get client: " << fd;
	});

	looper.loop();
	light::utils::SocketGlobalFinitialize();
	return 0;
}
