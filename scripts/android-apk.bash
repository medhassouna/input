#!/bin/bash

# Needed env variables:
# QT_BASE, e.g. /opt/Qt/<version>/android
# SOURCE_DIR, path to input root source dir
# ANDROID_NDK_ROOT, e.g. /opt/Android/sdk/ndk/<version>
# ANDROID_NDK_HOST, e.g. darwin_x86_64 (on mac)
# ANDROID_NDK_PLATFORM=android-24
# ANDROIDAPI=24
# ARCH=armeabi-v7a or arm640v8a
# INPUTKEYSTORE_STOREPASS
# make sure you have JAVA_HOME env set to your java installation where JAVA_HOME/lib/tools.jar exists

# optional:
# CORES, number of cores to use

BUILD_DIR=`pwd`/build-input
INSTALL_DIR=${BUILD_DIR}/out

set -e

# TODO take from input-sdk?
# export ANDROIDAPI=23
if [ "X${ARCH}" == "Xarmeabi-v7a" ]; then
  export TOOLCHAIN_SHORT_PREFIX=arm-linux-androideabi
  export TOOLCHAIN_PREFIX=arm-linux-androideabi
  export QT_ARCH_PREFIX=armv7
elif [ "X${ARCH}" == "Xarm64-v8a" ]; then
  export TOOLCHAIN_SHORT_PREFIX=aarch64-linux-android
  export TOOLCHAIN_PREFIX=aarch64-linux-android
  export QT_ARCH_PREFIX=arm64 # watch out when changing this, openssl depends on it
else
  echo "Error: Please report issue to enable support for arch (${ARCH})."
  exit 1
fi

#####
# PRINT ENV

echo "INSTALL_DIR: ${INSTALL_DIR}"
echo "SOURCE_DIR: ${SOURCE_DIR}"
echo "BUILD_DIR: ${BUILD_DIR}"
echo "ARCH: ${ARCH}"
echo "NDK: ${ANDROID_NDK_ROOT}"
echo "API: $ANDROIDAPI"
echo "QT BASE: ${QT_BASE}"

######################
# Input

# see https://bugreports.qt.io/browse/QTBUG-80756
# export ANDROID_TARGET_ARCH=${ARCH}

mkdir -p ${BUILD_DIR}/.gradle

pushd ${BUILD_DIR}

${QT_BASE}/bin/qmake -spec android-clang ANDROID_ABIS="${ARCH}" ${SOURCE_DIR}/app/input.pro
ls ${ANDROID_NDK_ROOT}/prebuilt/
${ANDROID_NDK_ROOT}/prebuilt/${ANDROID_NDK_HOST}/bin/make qmake_all
make -j ${CORES}

make install INSTALL_ROOT=${INSTALL_DIR}

if [ -f ${SOURCE_DIR}/Input_keystore.keystore ]; then
    echo "building release"
    ${QT_BASE}/bin/androiddeployqt \
        --sign ${SOURCE_DIR}/Input_keystore.keystore input \
        --storepass ${INPUTKEYSTORE_STOREPASS} \
        --keypass ${INPUTKEYSTORE_STOREPASS} \
        --input ${BUILD_DIR}/android-Input-deployment-settings.json \
        --output ${INSTALL_DIR} \
        --deployment bundled \
        --gradle
else
    echo "building debug"
    ${QT_BASE}/bin/androiddeployqt \
        --input ${BUILD_DIR}/android-Input-deployment-settings.json \
        --output ${INSTALL_DIR} \
        --deployment bundled \
        --gradle
fi
