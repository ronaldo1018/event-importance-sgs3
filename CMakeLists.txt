SET(IMPORTANCE_SRC
	importance.c
	netlink.c
	thread.c
	core.c
	activity.c
	touch.c
	timer.c
	vector.c
	parse.c
	debug.c
)
INCLUDE_DIRECTORIES(${IMPORTANCE_SOURCE_DIR}/include)
ADD_EXECUTABLE(importance ${IMPORTANCE_SRC})

