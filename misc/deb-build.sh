#!/bin/bash

cp=/usr/bin/cp
mv=/usr/bin/mv
ln=/usr/bin/ln
find=/usr/bin/find
gzip=/usr/bin/gzip
make=/usr/bin/make
cmake=/usr/bin/cmake
mkdir=/usr/bin/mkdir
apt=/usr/bin/apt-get
chmod=/usr/bin/chmod
chown=/usr/bin/chown
dpkg_deb=/usr/bin/dpkg-deb
dpkg_scan=/usr/bin/dpkg-scanpackages

PKG_NAME=myrpc
PKG_VERSION=1.0.0
PKG_ARCH=amd64
DIR_BUILD=build/release
DIR_MISC=misc
DIR_REPO=deb

PKG_SERVER_FULL_NAME=$DIR_BUILD/${PKG_NAME}-server_${PKG_VERSION}_${PKG_ARCH}
PKG_CLIENT_FULL_NAME=$DIR_BUILD/${PKG_NAME}-client_${PKG_VERSION}_${PKG_ARCH}
PKG_META_FULL_NAME=$DIR_BUILD/${PKG_NAME}-meta_${PKG_VERSION}_${PKG_ARCH}

$apt update && $apt upgrade && $apt install -y \
    build-essential \
    dpkg-dev \
    debhelper \
    devscripts \
    make \
    cmake \
    ninja-build

if [ $? -ne 0 ]; then
    echo "Failed to install dependencies $?" >&2
    exit 1
fi

cd mysyslog && $make deb
if [ $? -ne 0 ]; then
    echo "Failed to build mysyslog deb packages $?" >&2
    exit 1
fi

cd ..
if ! $cmake --preset=release; then
    echo "Failed to generate myrpc build scripts $?" >&2
    exit 1
fi

if ! $cmake --build --preset=release; then
    echo "Failed to build myrpc binaries $?" >&2
    exit 1
fi

$chmod 755 $($find $DIR_MISC -name control -type f)
$chmod 755 $($find $DIR_MISC -name postinst -type f)

$rm -rf $PKG_SERVER_FULL_NAME 2>/dev/null
$rm -rf $PKG_CLIENT_FULL_NAME 2>/dev/null
$rm -rf $PKG_META_FULL_NAME 2>/dev/null

$cp -r $DIR_MISC/${PKG_NAME}-server $PKG_SERVER_FULL_NAME
$cp -r $DIR_MISC/${PKG_NAME}-client $PKG_CLIENT_FULL_NAME
$cp -r $DIR_MISC/${PKG_NAME}-meta $PKG_META_FULL_NAME

if ! $mkdir -p $PKG_SERVER_FULL_NAME/usr/sbin; then
    echo "Unable to create $PKG_SERVER_FULL_NAME/usr/sbin direcrory" >&2
    exit 1
fi
$cp $DIR_BUILD/$PKG_NAME-server $PKG_SERVER_FULL_NAME/usr/sbin

if ! $mkdir -p $PKG_CLIENT_FULL_NAME/usr/bin; then
    echo "Unable to create $PKG_CLIENT_FULL_NAME/usrsbin direcrory" >&2
    exit 1
fi
$cp $DIR_BUILD/$PKG_NAME-client $PKG_CLIENT_FULL_NAME/usr/bin

$dpkg_deb --root-owner-group --build $PKG_SERVER_FULL_NAME
if [ $? -ne 0 ]; then
    echo "Unable to build $PKG_SERVER_FULL_NAME.deb" >&2
    exit 1
fi

$dpkg_deb --root-owner-group --build $PKG_CLIENT_FULL_NAME
if [ $? -ne 0 ]; then
    echo "Unable to build $PKG_CLIENT_FULL_NAME.deb" >&2
    exit 1
fi

$dpkg_deb --root-owner-group --build $PKG_META_FULL_NAME
if [ $? -ne 0 ]; then
    echo "Unable to build $PKG_META_FULL_NAME.deb" >&2
    exit 1
fi

$mkdir -p $DIR_REPO/packages
$mv $($find . -name "*.deb") $DIR_REPO/packages
cd $DIR_REPO && $dpkg_scan . /dev/null | $gzip -9c > Packages.gz
