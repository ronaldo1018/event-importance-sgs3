cmake_minimum_required(VERSION 2.8)
PROJECT(IMPORTANCE)

ADD_CUSTOM_TARGET(push adb push bin/importance /data/local/bin/)
ADD_CUSTOM_TARGET(exec adb shell /data/local/bin/importance | tee logs/imp-`date +%G%m%d%H%M`.txt)
ADD_CUSTOM_TARGET(timer adb shell insmod /data/local/bin/timer_connector.ko)

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

SET(MY_C_FLAGS "-Wall")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MY_C_FLAGS}")

INCLUDE_DIRECTORIES(${IMPORTANCE_SOURCE_DIR}/include)
ADD_EXECUTABLE(importance ${IMPORTANCE_SRC})

