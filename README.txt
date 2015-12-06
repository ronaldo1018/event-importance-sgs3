Prerequisites:
1. Android NDK r10e
   Assumed it's installed in /opt/android-ndk
2. Rooted phone

How to build:
1. git clone https://github.com/taka-no-me/android-cmake /opt/android-cmake
2. cd build
3. cmake -DCMAKE_TOOLCHAIN_FILE=/opt/android-cmake/android.toolchain.cmake -DANDROID_NDK=/opt/android-ndk -DCMAKE_BUILD_TYPE=Release -DANDROID_ABI="armeabi-v7a with NEON" -DANDROID_NATIVE_API_LEVEL=android-19 ..
4. make

How to run:
1. make push
2. adb shell
3. cd /data/local/tmp
4. ./importance

Problems:
Cannot create netlink socket: protocol not supported
    Enable connector in kernel config
