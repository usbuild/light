INCLUDE (ExternalProject)

ExternalProject_Add(enet_proj
	GIT_REPOSITORY https://github.com/lsalzman/enet.git
	GIT_TAG 5f476546edabdf37509cd3448d1a616f5eca535d
	PATCH_COMMAND git apply ${CMAKE_SOURCE_DIR}/src/enet.diff
	INSTALL_COMMAND cp -p <BINARY_DIR>/${CMAKE_CFG_INTDIR}/libenet.a ${CMAKE_BINARY_DIR}/deps/lib &&
									cp -r -p <SOURCE_DIR>/include/enet ${CMAKE_BINARY_DIR}/deps/include
	)


ExternalProject_Add(flatbuffer_proj
	GIT_REPOSITORY https://github.com/google/flatbuffers.git
	GIT_TAG v1.2.0
	INSTALL_COMMAND cp -p <BINARY_DIR>/${CMAKE_CFG_INTDIR}/flatc ${CMAKE_BINARY_DIR}/deps/bin &&
									cp -p <BINARY_DIR>/${CMAKE_CFG_INTDIR}/flathash ${CMAKE_BINARY_DIR}/deps/bin &&
									cp -p <BINARY_DIR>/${CMAKE_CFG_INTDIR}/libflatbuffers.a ${CMAKE_BINARY_DIR}/deps/bin &&
								  cp -p -r <SOURCE_DIR>/include/flatbuffers ${CMAKE_BINARY_DIR}/deps/include
	)

IF (APPLE)
	SET(luatgt "macosx")
ELSE()
	SET(luatgt "linux")
ENDIF()

ExternalProject_Add(lua_proj
	URL http://www.lua.org/ftp/lua-5.3.1.tar.gz
	CONFIGURE_COMMAND ""
	BUILD_COMMAND make ${luatgt} -C <SOURCE_DIR>
	INSTALL_COMMAND cp -p <SOURCE_DIR>/src/liblua.a ${CMAKE_BINARY_DIR}/deps/lib &&
									sh -c "cp <SOURCE_DIR>/src/*.h ${CMAKE_BINARY_DIR}/deps/include"
	)

SET(PROTOC ${CMAKE_BINARY_DIR}/deps/bin/flatc PARENT_SCOPE)

SET(LIB_DEP ${LIB_DEP} enet_proj)
SET(LIB_DEP ${LIB_DEP} lua_proj)

SET(LIB_DEP ${LIB_DEP} PARENT_SCOPE)
