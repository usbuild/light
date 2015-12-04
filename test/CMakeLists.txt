INCLUDE (ExternalProject)
INCLUDE (CTest)

ExternalProject_Add(gtest_gmock_src
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG 71a26ac68b28c61686b8941730a6b2577912e239
	CONFIGURE_COMMAND "${CMAKE_COMMAND}" <SOURCE_DIR>/googlemock  -G ${CMAKE_GENERATOR}
	INSTALL_COMMAND cp -p <BINARY_DIR>/${CMAKE_CFG_INTDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gmock${CMAKE_STATIC_LIBRARY_SUFFIX} ${CMAKE_BINARY_DIR}/deps/lib &&
									cp -p <BINARY_DIR>/${CMAKE_CFG_INTDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gmock_main${CMAKE_STATIC_LIBRARY_SUFFIX} ${CMAKE_BINARY_DIR}/deps/lib &&
									cp -p <BINARY_DIR>/gtest/${CMAKE_CFG_INTDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX} ${CMAKE_BINARY_DIR}/deps/lib &&
									cp -p <BINARY_DIR>/gtest/${CMAKE_CFG_INTDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX} ${CMAKE_BINARY_DIR}/deps/lib &&
								  cp -p -r <SOURCE_DIR>/googletest/include/gtest  ${CMAKE_BINARY_DIR}/deps/include &&
								  cp -p -r <SOURCE_DIR>/googlemock/include/gmock  ${CMAKE_BINARY_DIR}/deps/include

	)


FILE (GLOB_RECURSE ls_test_SRC "*.h" "*.cpp")
ADD_EXECUTABLE (unit_test ${ls_test_SRC})
IF(NOT WIN32)
SET(PTHREAD pthread)
ENDIF()
TARGET_LINK_LIBRARIES (unit_test gtest gtest_main gmock gmock_main lightserver_common enet ${PTHREAD})

ADD_DEPENDENCIES (unit_test gtest_gmock_src ${LIB_DEP})

IF(UNIX)
	FIND_PROGRAM(CTEST_MEMORYCHECK_COMMAND NAMES valgrind)
	SET(CTEST_MEMORYCHECK_COMMAND_OPTIONS, "--trace-children=yes --leak-check=full --track-origins=yes")
ENDIF()

ADD_TEST(all ${CMAKE_BINARY_DIR}/test/unit_test${CMAKE_EXECUTABLE_SUFFIX})

IF(NOT WIN32)
SET_TARGET_PROPERTIES(unit_test PROPERTIES
	COMPILE_FLAGS "-Wno-unused-variable")
ENDIF()
