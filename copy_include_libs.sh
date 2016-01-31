set -e
set -x

ANDROID_ROOT=$1
KERNEL_ROOT=$2

rm -f lib/*.so
rm -rvf include/frameworks/native/include
rm -rvf include/system/core/include
rm -f include/{connector.h,cn_proc.h}

cp $ANDROID_ROOT/out/target/product/*/system/lib/{libbinder.so,libutils.so} lib
cp -rv $ANDROID_ROOT/frameworks/native/include include/frameworks/native/
cp -rv $ANDROID_ROOT/system/core/include include/system/core/
cp $KERNEL_ROOT/include/linux/{connector.h,cn_proc.h} include/
