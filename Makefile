#Customisable stuff here
LINUX_COMPILER = gcc
WIN32_COMPILER = i686-w64-mingw32-gcc

WIN32_DEV_DIR=win32
WIN32_GTK_DEV_DIR=${WIN32_DEV_DIR}/gtk_2_0-2.14
WIN32_PIDGIN_DIR=${WIN32_DEV_DIR}/pidgin-2.10.11
WIN32_CFLAGS = \
				-DENABLE_NLS \
				-DHAVE_ZLIB \
				-DPURPLE_PLUGINS \
				-I${WIN32_GTK_DEV_DIR}/include/glib-2.0 \
				-I${WIN32_PIDGIN_DIR}/pidgin \
				-I${WIN32_PIDGIN_DIR}/libpurple \
				-I${WIN32_PIDGIN_DIR}/libpurple/win32 \
				-I${WIN32_GTK_DEV_DIR}/include \
				-I${WIN32_GTK_DEV_DIR}/include/gtk-2.0 \
				-I${WIN32_GTK_DEV_DIR}/include/cairo \
				-I${WIN32_GTK_DEV_DIR}/include/pango-1.0 \
				-I${WIN32_GTK_DEV_DIR}/include/atk-1.0 \
				-I${WIN32_GTK_DEV_DIR}/lib/glib-2.0/include \
				-I${WIN32_GTK_DEV_DIR}/lib/gtk-2.0/include \
				-I${WIN32_DEV_DIR}/json-glib-0.12.6 \
				-Wno-format

WIN32_LIBS = \
				-L${WIN32_GTK_DEV_DIR}/lib \
				-L${WIN32_PIDGIN_DIR}-win32bin \
				-lglib-2.0 \
				-lgobject-2.0 \
				-lintl \
				-lpurple \
				-lpidgin \
				-lws2_32 \
				-L. \
				-ljson-glib-1.0 \
				-lz

LIBPURPLE_CFLAGS = -DPURPLE_PLUGINS -DENABLE_NLS -DHAVE_ZLIB
GLIB_CFLAGS = `pkg-config glib-2.0 json-glib-1.0 --cflags --libs`

ifdef FLIST_PURPLE_ONLY
FLIST_ADDITIONAL_CFLAGS = -DFLIST_PURPLE_ONLY
PIDGIN_CFLAGS = `pkg-config purple --cflags --libs`
else
FLIST_ADDITIONAL_SOURCES = f-list_pidgin.c
PIDGIN_CFLAGS = `pkg-config pidgin --cflags --libs`
endif

PIDGIN_DIR = `pkg-config --variable=plugindir purple`
PIDGIN_DIR = ~/.purple/plugins/

FLIST_SOURCES = \
				f-list.c \
				f-list_admin.c \
				f-list_autobuddy.c \
				f-list_bbcode.c \
				f-list_callbacks.c \
				f-list_channels.c \
				f-list_commands.c \
				f-list_connection.c \
				f-list_icon.c \
				f-list_kinks.c \
				f-list_profile.c \
				f-list_json.c \
				f-list_friends.c \
				f-list_status.c \
				f-list_rtb.c \
				f-list_ignore.c \
				${FLIST_ADDITIONAL_SOURCES}

TARGET = flist.so
WIN32_TARGET = flist.dll

TEST_PURPLE_DIR = purple
TEST_PURPLE_PLUGINS_DIR = ${TEST_PURPLE_DIR}/plugins

#Standard stuff here
.PHONY: all clean install prepare_cross

all:	${TARGET}

clean:
	rm -f ${TARGET}
	rm -f ${WIN32_TARGET}
	rm -rf ${WIN32_DEV_DIR}
	rm -rf ${TEST_PURPLE_DIR}

prepare_test_linux: ${TARGET}
	mkdir -p ${TEST_PURPLE_PLUGINS_DIR}
	cp ${TARGET} ${TEST_PURPLE_PLUGINS_DIR}

test_linux: prepare_test_linux
	pidgin -m -c ${TEST_PURPLE_DIR} -d 2>&1 | tee pidgin.log

valgrind_linux: prepare_test_linux
	valgrind --tool=memcheck --leak-check=yes --leak-resolution=high --num-callers=20 --trace-children=no --child-silent-after-fork=yes --track-fds=yes --track-origins=yes pidgin -m -c ${TEST_PURPLE_DIR} -d 2>&1 | tee valgrind.log

install:
	cp flist.so ${PIDGIN_DIR}

${TARGET}:	${FLIST_SOURCES}
	${LINUX_COMPILER} -Wall -I. -g -std=c99 -O2 -pipe ${FLIST_SOURCES} -o $@ -shared -fPIC ${LIBPURPLE_CFLAGS} ${PIDGIN_CFLAGS} ${GLIB_CFLAGS} ${FLIST_ADDITIONAL_CFLAGS}

prepare_cross:
	./contrib/prepare_cross.sh

${WIN32_TARGET}: ${FLIST_SOURCES} 
	${WIN32_COMPILER} -Wall -I. -g -O2 -std=c99 -pipe ${FLIST_SOURCES} -o $@ -shared ${WIN32_CFLAGS} ${WIN32_LIBS}
