#!/system/bin/sh

function touchev()
{
	sendevent /dev/input/event2 3 57 29
	sendevent /dev/input/event2 3 53 $1
	sendevent /dev/input/event2 3 54 $2
	sendevent /dev/input/event2 3 48 29
	sendevent /dev/input/event2 3 58 2
	sendevent /dev/input/event2 0 0 0
	sendevent /dev/input/event2 3 57 4294967295
	sendevent /dev/input/event2 0 0 0
}

am start -S -W org.openintents.filemanager/.FileManagerActivity
input keyevent 82
usleep 500000
touchev 306 1035
usleep 500000
input keyevent 4
usleep 500000
input keyevent 82
usleep 500000
touchev 306 1035
usleep 500000
input keyevent 4
usleep 500000
input keyevent 82
usleep 500000
touchev 306 1035
usleep 500000
input keyevent 4
usleep 500000
input keyevent 82
usleep 500000
touchev 306 1035
usleep 500000
input keyevent 4
usleep 500000
input keyevent 82
usleep 500000
touchev 306 1035
usleep 500000
input keyevent 4
usleep 500000
input keyevent 82
usleep 500000
touchev 306 1035
usleep 500000
input keyevent 4
usleep 500000
input keyevent 82
usleep 500000
touchev 306 1035
usleep 500000
input keyevent 4
usleep 500000
input keyevent 82
usleep 500000
touchev 306 1035
usleep 500000
input keyevent 4
usleep 500000
input keyevent 82
usleep 500000
touchev 306 1035
usleep 500000
input keyevent 4
usleep 500000
input keyevent 82
usleep 500000
touchev 306 1035
usleep 500000
input keyevent 4
usleep 500000
