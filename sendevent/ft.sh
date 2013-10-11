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

# remove test file
rm /storage/sdcard0/avatar.zip
# stop, start and wait ftpcafe to finish loading
am start -S -W com.ftpcafe.trial/.Login
# touch connect
touchev 479 512
usleep 1500000
# touch avatar.zip
touchev 320 454
usleep 800000
# touch download
touchev 626 280
# stop, start and wait openexplorer to finish loading
am start -S -W org.brandroid.openmanager/.activities.OpenExplorer
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
