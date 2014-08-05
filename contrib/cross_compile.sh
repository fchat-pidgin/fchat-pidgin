#!/bin/bash

# Script to cross-compile Pidgin plugins on Linux for Windows.
# Tested to work on Ubuntu 12.04

PLUGIN_SRC_FILES=$@
PLUGIN_NAME=flist
PLUGIN_NAME="${PLUGIN_NAME%.*}"
PLUGIN_DLL="lib${PLUGIN_NAME}.dll"

set -e
set -u

BASE_DIR=$(pwd)
BUILD_DIR="$BASE_DIR/win32"
TMP_DIR="$BUILD_DIR/tmp"
META_DIR="${BUILD_DIR}/.meta"

mkdir -p $BUILD_DIR
mkdir -p $TMP_DIR

PIDGIN_VERSION="2.10.9"
PIDGIN_SRC="pidgin-${PIDGIN_VERSION}"
WIN32_SRC="${BUILD_DIR}/win32-dev"
MINGW_SRC="${WIN32_SRC}/mingw"

echo "Installing required packages ..."
#sudo apt-get install mingw32 mingw32-binutils mingw32-runtime

if [ ! -f "${TMP_DIR}/${PIDGIN_SRC}.tar.bz2" ]; then
  echo "Getting Pidgin source ..."
  wget --quiet "http://sourceforge.net/projects/pidgin/files/Pidgin/${PIDGIN_VERSION}/${PIDGIN_SRC}.tar.bz2/download" -O "${TMP_DIR}/${PIDGIN_SRC}.tar.bz2"
  tar xj --overwrite -f ${TMP_DIR}/${PIDGIN_SRC}.tar.bz2 -C $BUILD_DIR
else
  echo "Pidgin source is already present."
fi

cat > "${BUILD_DIR}/${PIDGIN_SRC}/local.mak" << EOF
SHELL := /bin/bash
CC := /usr/bin/i686-w64-mingw32-gcc
GMSGFMT := msgfmt
MAKENSIS := /usr/bin/makensis
WINDRES := /usr/bin/i686-w64-mingw32-windres
STRIP := /usr/bin/i686-w64-mingw32-strip
INTLTOOL_MERGE := /usr/bin/intltool-merge

INCLUDE_PATHS := -I/usr/i686-w64-mingw32/include
#LIB_PATHS := -L$(readlink -f "$WIN32_SRC")/mingw/lib/gcc/mingw32/4.7.2
LIB_PATHS := -L/usr/lib/gcc/i686-w64-mingw32/4.9.1/ -L$BASE_DIR
EOF

mkdir -p "${MINGW_SRC}"

function is_installed {
  mkdir -p ${META_DIR}
  [ -f "${META_DIR}/$1" ] && return 0
  return 1
}

function set_installed {
  mkdir -p ${META_DIR}
  touch "${META_DIR}/$1"
}

function get_dep {
  echo " * $1"
  is_installed $1 && return
  if [[ $# == 2 ]]; then
    TAR_FLAGS="x --lzma"
  else
    TAR_FLAGS=$3
  fi

  if [[ $# != 4 ]]; then
    TAR_TARGET="${MINGW_SRC}"
  else
    TAR_TARGET=$4
  fi

  wget --quiet $2  -O - | tar $TAR_FLAGS -C "${TAR_TARGET}"
  set_installed $1 
}

function get_dep_zip {
  echo " * $1"
  is_installed $1 && return

  if [[ $# != 4 ]]; then
    ZIP_TARGET="${WIN32_SRC}/$2"
  else
    ZIP_TARGET=$4
  fi

  wget --quiet $3 -O ${TMP_DIR}/`basename $3`
  unzip -qq ${TMP_DIR}/`basename $3` -d "${ZIP_TARGET}"
  rm ${TMP_DIR}/`basename $3`
  set_installed $1
}

echo "Getting dependencies ..."
get_dep binutils "http://sourceforge.net/projects/mingw/files/MinGW/Base/binutils/binutils-2.23.1/binutils-2.23.1-1-mingw32-bin.tar.lzma/download"
get_dep mingwrt "http://sourceforge.net/projects/mingw/files/MinGW/Base/mingw-rt/mingwrt-3.20/mingwrt-3.20-2-mingw32-dev.tar.lzma/download"
get_dep mingwrt-dll "http://sourceforge.net/projects/mingw/files/MinGW/Base/mingw-rt/mingwrt-3.20/mingwrt-3.20-2-mingw32-dll.tar.lzma/download"
get_dep w32api "http://sourceforge.net/projects/mingw/files/MinGW/Base/w32api/w32api-3.17/w32api-3.17-2-mingw32-dev.tar.lzma/"
get_dep mpc "http://sourceforge.net/projects/mingw/files/MinGW/Base/mpc/mpc-0.8.1-1/mpc-0.8.1-1-mingw32-dev.tar.lzma/"
get_dep mpc-dll "http://sourceforge.net/projects/mingw/files/MinGW/Base/mpc/mpc-0.8.1-1/libmpc-0.8.1-1-mingw32-dll-2.tar.lzma/"
get_dep mpcfr "http://sourceforge.net/projects/mingw/files/MinGW/Base/mpfr/mpfr-2.4.1-1/mpfr-2.4.1-1-mingw32-dev.tar.lzma/"
get_dep mpfr-dll "http://sourceforge.net/projects/mingw/files/MinGW/Base/mpfr/mpfr-2.4.1-1/libmpfr-2.4.1-1-mingw32-dll-1.tar.lzma/"
get_dep gmp "http://sourceforge.net/projects/mingw/files/MinGW/Base/gmp/gmp-5.0.1-1/gmp-5.0.1-1-mingw32-dev.tar.lzma/"
get_dep gmp-dll "http://sourceforge.net/projects/mingw/files/MinGW/Base/gmp/gmp-5.0.1-1/libgmp-5.0.1-1-mingw32-dll-10.tar.lzma/"
get_dep pthreads "http://sourceforge.net/projects/mingw/files/MinGW/Base/pthreads-w32/pthreads-w32-2.9.0-pre-20110507-2/pthreads-w32-2.9.0-mingw32-pre-20110507-2-dev.tar.lzma/"
get_dep pthread-dll "http://sourceforge.net/projects/mingw/files/MinGW/Base/pthreads-w32/pthreads-w32-2.9.0-pre-20110507-2/libpthreadgc-2.9.0-mingw32-pre-20110507-2-dll-2.tar.lzma/"
get_dep libiconv "http://sourceforge.net/projects/mingw/files/MinGW/Base/libiconv/libiconv-1.14-2/libiconv-1.14-2-mingw32-dev.tar.lzma/"
get_dep libiconv-dll "http://sourceforge.net/projects/mingw/files/MinGW/Base/libiconv/libiconv-1.14-2/libiconv-1.14-2-mingw32-dll-2.tar.lzma/"
get_dep libintl-dll "http://sourceforge.net/projects/mingw/files/MinGW/Base/gettext/gettext-0.18.1.1-2/libintl-0.18.1.1-2-mingw32-dll-8.tar.lzma/"
get_dep libgomp-dll "http://sourceforge.net/projects/mingw/files/MinGW/Base/gcc/Version4/gcc-4.7.2-1/libgomp-4.7.2-1-mingw32-dll-1.tar.lzma/"
get_dep libssp-dll "http://sourceforge.net/projects/mingw/files/MinGW/Base/gcc/Version4/gcc-4.7.2-1/libssp-4.7.2-1-mingw32-dll-0.tar.lzma/"
get_dep libquadmath-dll "http://sourceforge.net/projects/mingw/files/MinGW/Base/gcc/Version4/gcc-4.7.2-1/libquadmath-4.7.2-1-mingw32-dll-0.tar.lzma/"
get_dep gcc-core "http://sourceforge.net/projects/mingw/files/MinGW/Base/gcc/Version4/gcc-4.7.2-1/gcc-core-4.7.2-1-mingw32-bin.tar.lzma/"
get_dep libgcc-dll "http://sourceforge.net/projects/mingw/files/MinGW/Base/gcc/Version4/gcc-4.7.2-1/libgcc-4.7.2-1-mingw32-dll-1.tar.lzma/"
get_dep zlib-dll "http://sourceforge.net/projects/mingw/files/MinGW/Base/zlib/zlib-1.2.8/zlib-1.2.8-1-mingw32-dll.tar.lzma/download"
get_dep zlib "http://sourceforge.net/projects/mingw/files/MinGW/Base/zlib/zlib-1.2.8/zlib-1.2.8-1-mingw32-dev.tar.lzma/download"

get_dep_zip gtk gtk_2_0-2.14 "http://ftp.gnome.org/pub/gnome/binaries/win32/gtk+/2.14/gtk+-bundle_2.14.7-20090119_win32.zip"
get_dep_zip gettext-tools gettext-0.17 "http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/gettext-tools-0.17.zip"
get_dep_zip gettext-runtime gettext-0.17 "http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/gettext-runtime-0.17-1.zip"
get_dep_zip libxml2 libxml2-2.9.0 "http://ftp.gnome.org/pub/GNOME/binaries/win32/dependencies/libxml2-dev_2.9.0-1_win32.zip"
get_dep_zip libxml2-dll libxml2-2.9.0 "http://ftp.gnome.org/pub/GNOME/binaries/win32/dependencies/libxml2_2.9.0-1_win32.zip"
get_dep_zip intltool intltool_0.40.4-1_win32 "http://ftp.acc.umu.se/pub/GNOME/binaries/win32/intltool/0.40/intltool_0.40.4-1_win32.zip"
get_dep_zip enchant enchant_1.6.0_win32 "https://developer.pidgin.im/static/win32/enchant_1.6.0_win32.zip"

# meanwhile extracts to meanwhile-1.0.2_daa3/meanwhile-1.0.2_daa3/... if not downloaded this way
get_dep_zip meanwhile meanwhile-1.0.2_daa3 "https://developer.pidgin.im/static/win32/meanwhile-1.0.2_daa3-win32.zip" "$WIN32_SRC"

get_dep perl "https://developer.pidgin.im/static/win32/perl_5-10-0.tar.gz" "xz" "$WIN32_SRC"
get_dep tcl "https://developer.pidgin.im/static/win32/tcl-8.4.5.tar.gz" "xz" "$WIN32_SRC"
get_dep gtkspell "https://developer.pidgin.im/static/win32/gtkspell-2.0.16.tar.bz2" "xj" "$WIN32_SRC"
get_dep nss "https://developer.pidgin.im/static/win32/nss-3.16-nspr-4.10.4.tar.gz" "xz" "$WIN32_SRC"
get_dep silc "https://developer.pidgin.im/static/win32/silc-toolkit-1.1.10.tar.gz" "xz" "$WIN32_SRC"
get_dep cyrus-sasl "https://developer.pidgin.im/static/win32/cyrus-sasl-2.1.25.tar.gz" "xz" "$WIN32_SRC"
get_dep pidgin-deps "https://developer.pidgin.im/static/win32/pidgin-inst-deps-20130214.tar.gz" "xz" "${WIN32_SRC}/intltool_0.40.4-1_win32"
get_dep json-glib "https://download.gnome.org/sources/json-glib/0.12/json-glib-0.12.6.tar.bz2" "xj" "$WIN32_SRC"

# We don't have Bonjour libraries installed so we exclude that protocol from the build
sed -i "s/bonjour//g" "${BUILD_DIR}/${PIDGIN_SRC}/libpurple/protocols/Makefile.mingw"

# Disable mxit and myspace, they fucked up markup.c and break compilation
sed -i "s/mxit//g" "${BUILD_DIR}/${PIDGIN_SRC}/libpurple/protocols/Makefile.mingw"
sed -i "s/myspace//g" "${BUILD_DIR}/${PIDGIN_SRC}/libpurple/protocols/Makefile.mingw"

# Disable SSL plugin as it requires Mozilla NSS library that cannot be compiled easily (see: https://developer.pidgin.im/wiki/BuildingWinNSS)
sed -i "s/.*SSL.*//g" "${BUILD_DIR}/${PIDGIN_SRC}/libpurple/plugins/Makefile.mingw"

# Run configure in json-glib to generate json-version.h
pushd "${WIN32_SRC}/json-glib-0.12.6" > /dev/null
./configure > /dev/null
popd > /dev/null

cd "${BUILD_DIR}/${PIDGIN_SRC}"
echo "Building ..."
make -j8 -f Makefile.mingw

echo
echo
echo "***********************************************************************"
echo "********** Done building win32 cross compiling environment. ***********"
echo "***********************************************************************"
