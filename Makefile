#Customisable stuff here
LINUX_COMPILER = gcc

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
.PHONY:	all clean install

all: 	flist.so

clean:
	rm -f flist.so

install:
	cp flist.so ${PIDGIN_DIR}

flist.so:	${FLIST_SOURCES}
	${LINUX_COMPILER} -Wall -I. -g -O2 -pipe ${FLIST_SOURCES} -o $@ -shared -fPIC ${LIBPURPLE_CFLAGS} ${PIDGIN_CFLAGS} ${GLIB_CFLAGS} ${FLIST_ADDITIONAL_CFLAGS}


