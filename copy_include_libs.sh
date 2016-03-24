set -e
set -x

ANDROID_ROOT=$1
KERNEL_ROOT=$2

rm -f lib/*.so
rm -rvf include/frameworks/native/include
rm -rvf include/system/core/include
rm -f include/{connector.h,cn_proc.h}
rm -f include/importance_data.h
rm -f IImportance.{cpp,h}

mkdir -p lib
mkdir -p include/{frameworks/native,system/core}

cp $ANDROID_ROOT/out/target/product/*/system/lib/{libbinder.so,libutils.so} lib
cp -rv $ANDROID_ROOT/frameworks/native/include include/frameworks/native/
cp -rv $ANDROID_ROOT/system/core/include include/system/core/
cp $ANDROID_ROOT/gecko/hal/importance_data.h include/
cp $ANDROID_ROOT/gecko/hal/gonk/IImportance.{cpp,h} .
cp $KERNEL_ROOT/include/linux/{connector.h,cn_proc.h} include/
