#include <gtest/gtest.h>
#include <network/socket.h>

int main(int argc, char **argv) {
  light::utils::SocketGlobalInitialize();
  ::testing::InitGoogleTest(&argc, argv);
  int ret = RUN_ALL_TESTS();
  light::utils::SocketGlobalFinitialize();
  return ret;
}
