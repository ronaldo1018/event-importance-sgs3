Prerequisites:
1. Android NDK r10e
   Assumed it's installed in /opt/android-ndk
2. Rooted phone

How to build:
ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=./Android.mk

How to run:
1. adb push libs/armeabi/importance /data/local/tmp/
2. adb shell
3. cd /data/local/tmp
4. ./importance

Problems:
Cannot create netlink socket: protocol not supported
    Enable connector in kernel config

Shared files:
include/importance_data.h => gecko/hal/importance_data.h
include/IImportance.h => gecko/hal/gonk/IImportance.h
IImportance.cpp => gecko/hal/gonk/IImportance.cpp
include/frameworks/native/include => frameworks/native/include
include/system/core/include/ => system/core/include/
lib/libutils.so => out/target/product/flame/system/lib/libutils.so
lib/libbinder.so => out/target/product/flame/system/lib/libbinder.so
