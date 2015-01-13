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
				${FLIST_ADDITIONAL_SOURCES}

#Standard stuff here
.PHONY: all clean install prepare_cross

all:	flist.so

clean:
	rm -f flist.so
	rm -f flist.dll
	rm -rf win32

install:
	cp flist.so ${PIDGIN_DIR}

flist.so:	${FLIST_SOURCES}
	${LINUX_COMPILER} -Wall -I. -g -O2 -pipe ${FLIST_SOURCES} -o $@ -shared -fPIC ${LIBPURPLE_CFLAGS} ${PIDGIN_CFLAGS} ${GLIB_CFLAGS} ${FLIST_ADDITIONAL_CFLAGS}

prepare_cross:
	./contrib/prepare_cross.sh

flist.dll: ${FLIST_SOURCES} 
	${WIN32_COMPILER} -Wall -I. -g -O2 -pipe ${FLIST_SOURCES} -o $@ -shared ${WIN32_CFLAGS} ${WIN32_LIBS}
