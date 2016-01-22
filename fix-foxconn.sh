set -e
set -x

adb shell stop ril-daemon
fb-adb shell sh -c "echo adb > /sys/power/wake_lock"
adb shell cat /sys/power/wake_lock
