FILE (GLOB server_SRC "server/*.cpp" "server/*.h")
ADD_EXECUTABLE (lightserver ${server_SRC})

IF (strict)
	SET_TARGET_PROPERTIES(lightserver PROPERTIES
		COMPILE_FLAGS ${STRICT_CXX_FLAGS})
ENDIF ()
