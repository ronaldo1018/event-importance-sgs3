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
rm /storage/sdcard0/AVideo/avatar.mp4.zip
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
# stop, start and wait openmanager to finish loading
am start -S -W org.brandroid.openmanager/.activities.OpenExplorer
# touch AVideo
touchev 551 565
usleep 800000
# touch avatar.mp4
touchev 688 155
usleep 800000
# touch zip
touchev 544 1247
usleep 800000
# touch yes
touchev 512 935
# start launcher
am start com.sec.android.app.launcher/com.android.launcher2.Launcher
