CMAKE_MINIMUM_REQUIRED (VERSION 2.8 FATAL_ERROR)

OPTION (test "Build all tests." ON)
OPTION (debug "enable debug." ON)
OPTION (strict "enable strict check." ON)
CMAKE_POLICY (SET CMP0054 OLD)

PROJECT (lightserver CXX C)

FILE (GLOB lightserver_lib_SRC "src/common/**/*.h" "src/common/**/*.cpp")

IF(NOT WIN32)
SET(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Werror -Wno-unused -Wextra")

SET(STRICT_CXX_FLAGS "-Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 -Winit-self \
											-Wmissing-declarations -Woverloaded-virtual \
											-Wredundant-decls -Wshadow -Wsign-promo -Wstrict-overflow=5 -Wswitch-default -Wundef \
											-Wpointer-arith -Wcast-qual -pedantic")
ENDIF()

IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	SET(STRICT_CXX_FLAGS "${STRICT_CXX_FLAGS} -stdlib=libc++")
ELSEIF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	SET(STRICT_CXX_FLAGS "${STRICT_CXX_FLAGS} -Wlogical-op -Wnoexcept -Wstrict-null-sentinel -Wuninitialized")
ELSEIF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	SET(BUILD_SHARED_LIBS OFF)
	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
	SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:MSVCRTD.lib")
	SET(STRICT_CXX_FLAGS "/nologo")
ELSE()

ENDIF ()

IF (debug)
	IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
		SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od")
		SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Zi /Od")
	ELSE()
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
		SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
	ENDIF()
ELSE()
	IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	ELSE()
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
	ENDIF()
ENDIF ()

#platform check
INCLUDE(platform.cmake)

FILE (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/deps/bin)
FILE (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/deps/include)
FILE (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/deps/lib)
FILE (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/gen_header/proto)

ADD_SUBDIRECTORY (3rd)
INCLUDE_DIRECTORIES (${CMAKE_BINARY_DIR}/deps/include)
LINK_DIRECTORIES (${CMAKE_BINARY_DIR}/deps/lib)

FILE (GLOB proto_SRC "src/common/proto/*.fbs")
SET (PROTOC_C_OUT_FLAG  -c)
SET (PROTO_GEN_DIR ${CMAKE_BINARY_DIR}/gen_header/proto)

FOREACH(PROTO_FILE ${proto_SRC})
  GET_FILENAME_COMPONENT(PROTO_NAME ${PROTO_FILE} NAME_WE)
  SET(CUR_PROTO_GEN
     ${PROTO_GEN_DIR}/${PROTO_NAME}_generated.h
  )
  SET(PROTO_GEN
     ${PROTO_GEN}
     ${CUR_PROTO_GEN}
  )
 
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CUR_PROTO_GEN}
		COMMAND ${PROTOC} ${PROTOC_C_OUT_FLAG} --gen-mutable -o ${PROTO_GEN_DIR} -I ${CMAKE_SOURCE_DIR}/src/common/proto/ ${PROTO_FILE}
		DEPENDS ${PROTO_FILE} flatbuffer_proj
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
ENDFOREACH(PROTO_FILE ${PROTO_INPUT})

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/gen_header/)
ADD_LIBRARY (lightserver_common ${lightserver_lib_SRC} ${PROTO_GEN})

IF (strict AND debug)
	SET_TARGET_PROPERTIES(lightserver_common PROPERTIES
		COMPILE_FLAGS ${STRICT_CXX_FLAGS})
ENDIF ()

ADD_DEPENDENCIES(lightserver_common ${LIB_DEP})

IF(NOT WIN32)
SET(COMMLIB dl)
ELSE()
SET(COMMLIB Ws2_32 WINMM)
ENDIF()
TARGET_LINK_LIBRARIES (lightserver_common lua ${COMMLIB})

INCLUDE_DIRECTORIES ("${PROJECT_SOURCE_DIR}/src/common")


ADD_SUBDIRECTORY (src)

IF (test)
	ADD_SUBDIRECTORY (test)
ENDIF ()
