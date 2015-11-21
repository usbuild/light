INCLUDE (ExternalProject)

ExternalProject_Add(gtest_gmock_src
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG 71a26ac68b28c61686b8941730a6b2577912e239
	CONFIGURE_COMMAND "${CMAKE_COMMAND}" <SOURCE_DIR>/googlemock -G ${CMAKE_GENERATOR}

	INSTALL_COMMAND cp -p <BINARY_DIR>/${CMAKE_CFG_INTDIR}/libgmock.a ${CMAKE_BINARY_DIR}/deps/lib &&
									cp -p <BINARY_DIR>/${CMAKE_CFG_INTDIR}/libgmock_main.a ${CMAKE_BINARY_DIR}/deps/lib &&
									cp -p <BINARY_DIR>/gtest/${CMAKE_CFG_INTDIR}/libgtest.a ${CMAKE_BINARY_DIR}/deps/lib &&
									cp -p <BINARY_DIR>/gtest/${CMAKE_CFG_INTDIR}/libgtest_main.a ${CMAKE_BINARY_DIR}/deps/lib &&
								  cp -p -r <SOURCE_DIR>/googletest/include/gtest  ${CMAKE_BINARY_DIR}/deps/include &&
								  cp -p -r <SOURCE_DIR>/googlemock/include/gmock  ${CMAKE_BINARY_DIR}/deps/include

	)


FILE (GLOB_RECURSE ls_test_SRC "*.h" "*.cpp")
ENABLE_TESTING ()
ADD_EXECUTABLE (unit_test ${ls_test_SRC})
TARGET_LINK_LIBRARIES (unit_test gtest gtest_main gmock gmock_main lightserver_common enet pthread)
ADD_DEPENDENCIES (unit_test gtest_gmock_src ${LIB_DEP})

SET_TARGET_PROPERTIES(unit_test PROPERTIES
	COMPILE_FLAGS "-Wno-unused-variable")
