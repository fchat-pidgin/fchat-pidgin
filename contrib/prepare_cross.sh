#/bin/sh
# This script prepares the cross-compiling environment with all required dependancies

set -e

FAIL=false
for command in unzip wget tar ; do
    if ! command -v $command >/dev/null 2>/dev/null ; then
        echo "Please install $command"
        FAIL=true
    fi
done
$FAIL && exit 1

if ! command -v i686-w64-mingw32-gcc >/dev/null 2>/dev/null ; then
    echo "You need to install the MinGW-w64 compiler :"
    echo "    aptitude install mingw-w64"
    exit 2
fi

PIDGIN_VERSION=2.10.12

WIN32_DEV_DIR=win32/
mkdir -p $WIN32_DEV_DIR
GTK_BUNDLE_URL=http://ftp.acc.umu.se/pub/gnome/binaries/win32/gtk+/2.16/gtk+-bundle_2.16.6-20100912_win32.zip
GTK_BUNDLE_FILE=${WIN32_DEV_DIR}/gtk+-bundle_2.16.6-20100912_win32.zip
GTK_BUNDLE_DIR=${WIN32_DEV_DIR}/gtk_2_0-2.16
PIDGIN_SRC_URL=http://sourceforge.net/projects/pidgin/files/Pidgin/${PIDGIN_VERSION}/pidgin-${PIDGIN_VERSION}.tar.gz/download
PIDGIN_SRC_FILE=${WIN32_DEV_DIR}/pidgin-${PIDGIN_VERSION}.tar.gz
PIDGIN_SRC_DIR=${WIN32_DEV_DIR}/pidgin-${PIDGIN_VERSION}
PIDGIN_BIN_URL=http://sourceforge.net/projects/pidgin/files/Pidgin/${PIDGIN_VERSION}/pidgin-${PIDGIN_VERSION}-win32-bin.zip/download
PIDGIN_BIN_FILE=${WIN32_DEV_DIR}/pidgin-${PIDGIN_VERSION}-win32-bin.zip
PIDGIN_BIN_DIR=${WIN32_DEV_DIR}/pidgin-${PIDGIN_VERSION}-win32bin
JSON_SRC_URL=http://ftp.gnome.org/pub/GNOME/sources/json-glib/0.12/json-glib-0.12.6.tar.xz
JSON_SRC_FILE=${WIN32_DEV_DIR}/json-glib-0.12.6.tar.xz
JSON_SRC_DIR=${WIN32_DEV_DIR}/json-glib-0.12.6/

# Fetch files
for component in GTK_BUNDLE PIDGIN_SRC PIDGIN_BIN JSON_SRC ; do
    FILE=$(eval echo '$'${component}_FILE)
    URL=$(eval echo '$'${component}_URL)
    DIR=$(eval echo '$'${component}_DIR)
    OPTS=$(eval echo '$'${component}_OPTIONS)

    echo "* Retrieving $component"
    if [ -r "$FILE" ] ; then
        echo "* $component already retrieved"
    else
        wget -qO "$FILE" "$URL"
    fi

    echo "* Extracting $component"
    if [ -d "$DIR" ] ; then
        echo "* $component already extracted"
    else
        mkdir -p "$DIR"
        if [ "$FILE" != "${FILE%.tar.?z}" ]; then
            tar xf "$FILE" --strip-components 1 -C "$DIR" $OPTS
        elif [ "$FILE" != "${FILE%.zip}" ] ; then
            unzip -q $OPTS "$FILE" -d "$DIR"
        else
            cp "$FILE" "$DIR"
        fi
    fi
done

### Last steps
# Prepare json-version.h
if ! [ -f "$JSON_SRC_DIR"/json-glib/json-version.h ] ; then
echo "* Configuring json-glib"
cd "$JSON_SRC_DIR"
./configure
cd -
fi

# Put Pidgin windows binaries back in the right directory
mv ${PIDGIN_BIN_DIR}/pidgin-*/* ${PIDGIN_BIN_DIR} 2>/dev/null && true

echo ""
echo "All good, run"
echo "    make libflist.dll"
echo "to cross-compile"
