INCLUDE(CheckIncludeFiles)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckLibraryExists)
INCLUDE(CheckSymbolExists)
INCLUDE(CheckTypeSize)
INCLUDE(CheckCSourceCompiles)
INCLUDE(CheckCXXSourceCompiles)

CHECK_CXX_SOURCE_COMPILES (
	"extern thread_local int x;
	thread_local int * ptr = 0;
	int foo() { ptr = &x; return x; }
	thread_local int x = 1;

	int main()
	{
	x = 2;
	foo();
	return 0;
	}"
	HAVE_CXX11_THREAD_LOCAL
	)

IF (NOT HAVE_CXX11_THREAD_LOCAL)
	CHECK_CXX_SOURCE_COMPILES (
		"extern __thread int x;
		__thread int * ptr = 0;
		int foo() { ptr = &x; return x; }
		__thread int x = 1;

		int main()
		{
		x = 2;
		foo();
		return 0;
		}"
		HAVE_GNU_THREAD_LOCAL
		)
	ADD_DEFINITIONS(-Dthread_local=__thread)
ENDIF()

IF (NOT HAVE_CXX11_THREAD_LOCAL AND NOTHAVE_GNU_THREAD_LOCAL)
	MESSAGE (FATAL_ERROR "you compiler doesn't support thread_local or thread")
ENDIF()

CHECK_INCLUDE_FILES("sys/epoll.h" HAVE_EPOLL)
IF (HAVE_EPOLL)
	ADD_DEFINITIONS(-DHAVE_EPOLL)
ENDIF()
CHECK_INCLUDE_FILES("sys/timerfd.h" HAVE_TIMERFD)
IF (HAVE_TIMERFD)
	ADD_DEFINITIONS(-DHAVE_TIMERFD)
ENDIF ()
CHECK_INCLUDE_FILES("sys/eventfd.h" HAVE_EVENTFD)
IF (HAVE_EVENTFD)
	ADD_DEFINITIONS(-DHAVE_EVENTFD)
ENDIF()

CHECK_INCLUDE_FILES("sys/event.h" HAVE_KQUEUE)

IF (HAVE_KQUEUE)
	ADD_DEFINITIONS(-DHAVE_KQUEUE)
ENDIF()
