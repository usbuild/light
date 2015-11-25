#include <gtest/gtest.h>
#include "service/network_service.h"
#include "handler/station.h"
#include "service/lua_service.h"
#include "core/default_context_loader.h"

using namespace light::core;
using namespace light::network;
using namespace light::utils;
using namespace light::handler;
using namespace light::service;
TEST(Handler, test) {
	Context ctx;

	//Shuttle shuttle(*dynamic_cast<NetworkService*>(DefaultContextLoader::instance().require_service("network", "shuttle-network", ctx)));
	auto network_service = DefaultContextLoader::instance().require_service<NetworkService>("network", "station-network", ctx);
	int station_id = 5;
	auto station_handler = DefaultContextLoader::instance().require_handler<Station>("station", "station1", station_id, INetEndPoint(protocol::v4(), 8889), network_service);
	ctx.install_handler(*station_handler);

	auto station_id2 = 6;
	auto station_handler2 = DefaultContextLoader::instance().require_handler<Station>("station", "station2", station_id2, INetEndPoint(protocol::v4(), 8890), network_service);
	ctx.install_handler(*station_handler2);

	auto ec = LS_OK_ERROR();
	ctx.get_looper().add_timer(ec, 1000000LL, 0, [station_handler2](){
		station_handler2->connect_to_station(INetEndPoint("127.0.0.1", 8889));
		});
	auto lua_service = DefaultContextLoader::instance().require_service<LuaService>("luaservice", "luaservice", ctx);

	//lua_service->post
	lua_service->install_new_handler(LUA_TEST_DIR"test.lua", "aluahandler", "");

	ctx.get_looper().add_timer(ec, 5000000LL, 0, [&ctx] {
		ctx.get_looper().stop();
	});

    ctx.get_looper().loop();
	
	const int main_thread_num = std::thread::hardware_concurrency();
	std::thread *ths = new std::thread[main_thread_num];

	for (int i = 0; i < main_thread_num; ++i) {
		ths[i] = std::thread([&ctx]{ctx.get_looper().loop();});
	}
	for (int i = 0; i < main_thread_num; ++i) {
		ths[i].join();
	}
	delete []ths;
}
