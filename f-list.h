/*
 * F-List Pidgin - a libpurple protocol plugin for F-Chat
 *
 * Copyright 2011 F-List Pidgin developers.
 *
 * This file is part of F-List Pidgin.
 *
 * F-List Pidgin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * F-List Pidgin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with F-List Pidgin.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef FLIST_H
#define FLIST_H

#include <glib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef _WIN32
#    include "win32dep.h"
#    define dlopen(a,b) LoadLibrary(a)
#    define RTLD_LAZY
#    define dlsym(a,b) GetProcAddress(a,b)
#    define dlclose(a) FreeLibrary(a)
#else
#    include <arpa/inet.h>
#    include <dlfcn.h>
#    include <netinet/in.h>
#    include <sys/socket.h>
#endif

//json-glib sources
#include <json-glib/json-glib.h>

//libpurple sources
#include "util.h"
#include "debug.h"
#include "cmds.h"
#include "request.h"
#include "smiley.h"
#include "savedstatuses.h"


typedef enum FListFlags_ FListFlags;
typedef enum FListGender_ FListGender;
typedef enum FListStatus_ FListStatus;
typedef enum FListChannelMode_ FListChannelMode;
typedef enum FListFriendStatus_ FListFriendStatus;
typedef enum FListFriendsRequestType_ FListFriendsRequestType;
typedef enum FListConnectionStatus_ FListConnectionStatus;

typedef struct FListCharacter_ FListCharacter;
typedef struct FListAccount_ FListAccount;
typedef struct FListRoomlistChannel_ FListRoomlistChannel;

typedef struct FListKinks_ FListKinks;
typedef struct FListProfiles_ FListProfiles;
typedef struct FListWebRequestData_ FListWebRequestData;
typedef struct FListFriends_ FListFriends;

//gboolean flist_account_is_operator(PurpleConnection *pc, const gchar *name);
//void flist_account_set_operator(PurpleConnection *pc, const gchar *name, gboolean operator);

#define FLIST_CLIENT_NAME "F-List Pidgin"
#define FLIST_PLUGIN_VERSION    "0.3.1"
#define USER_AGENT              "Pidgin F-Chat 3.1"
#define FLIST_PLUGIN_ID         "prpl-flist"
#define FLIST_PORT              9722
#define FLIST_PORT_SECURE       9799
#define GLOBAL_NAME             "#FList"
#define FLIST_DEBUG             "flist"

#define CONVO_SHOW_CHAT         "CONVO_SHOW_CHAT"
#define CONVO_SHOW_ADS          "CONVO_SHOW_ADS"

#define CHANNEL_JOIN_TIMEOUT (5*(G_USEC_PER_SEC))
#define FLIST_STATUS_MESSAGE_KEY "message"

#define CHANNEL_COMPONENTS_NAME                 "channel"

#define FLIST_CHANNEL_INVITE                    "CIU"
#define FLIST_REQUEST_CHANNEL_BANLIST           "CBL"
#define FLIST_REQUEST_CHANNEL_LIST              "CHA"
#define FLIST_REQUEST_PRIVATE_CHANNEL_LIST      "ORS"
#define FLIST_REQUEST_CHANNEL_OPS               "COL"
#define FLIST_CHANNEL_BAN                       "CBU"
#define FLIST_CHANNEL_UNBAN                     "CUB"
#define FLIST_CHANNEL_KICK                      "CKU"
#define FLIST_CHANNEL_ADD_OP                    "COA"
#define FLIST_CHANNEL_REMOVE_OP                 "COR"
#define FLIST_CHANNEL_CREATE                    "CCR"
#define FLIST_SET_CHANNEL_DESCRIPTION           "CDS"
#define FLIST_SET_CHANNEL_STATUS                "RST"
#define FLIST_SET_STATUS                        "STA"
#define FLIST_CHANNEL_JOIN                      "JCH"
#define FLIST_ROLL_DICE                         "RLL"
#define FLIST_CHANNEL_LEAVE                     "LCH"
#define FLIST_REQUEST_CHANNEL_MESSAGE           "MSG"
#define FLIST_CHANNEL_ADVERSTISEMENT            "LRP"
#define FLIST_KINK_SEARCH                       "FKS"
#define FLIST_REQUEST_PRIVATE_MESSAGE           "PRI"
#define FLIST_REQUEST_CHARACTER_KINKS           "KIN"
#define FLIST_REQUEST_PROFILE                   "PRO"
#define FLIST_PING                              "PIN"
#define FLIST_NOTIFY_TYPING                     "TPN"
#define FLIST_CHANNEL_GET_BANLIST               "CBL"

/* admin commands */
#define FLIST_ADD_GLOBAL_OPERATOR               "AOP"
#define FLIST_REMOVE_GLOBAL_OPERATOR            "DOP"
#define FLIST_BROADCAST                         "BRO"
#define FLIST_PUBLIC_CHANNEL_CREATE             "CRC"
#define FLIST_PUBLIC_CHANNEL_DELETE             "KIC"
/* more */
#define FLIST_GLOBAL_KICK                       "KIK"
#define FLIST_GLOBAL_ACCOUNT_BAN                "ACB"
#define FLIST_GLOBAL_IP_BAN                     "IPB"
#define FLIST_GLOBAL_UNBAN                      "UBN"
#define FLIST_GLOBAL_TIMEOUT                    "TMO"
#define FLIST_REWARD                            "RWD"

#define LOGIN_TIMEOUT 5000


enum FListConnectionStatus_ {
    FLIST_OFFLINE,
    FLIST_CONNECT,
    FLIST_HANDSHAKE,
    FLIST_IDENTIFY,
    FLIST_ONLINE
};

enum FListFlags_ {
    FLIST_FLAG_GLOBAL_OP = 0x01,
    FLIST_FLAG_CHANNEL_OP = 0x02,
    FLIST_FLAG_CHANNEL_FOUNDER = 0x04,
    FLIST_FLAG_ADMIN = 0x08
};

enum FListStatus_ {
    FLIST_STATUS_AVAILABLE,
    FLIST_STATUS_LOOKING,
    FLIST_STATUS_BUSY,
    FLIST_STATUS_DND,
    FLIST_STATUS_CROWN,
    FLIST_STATUS_AWAY,
    FLIST_STATUS_IDLE,
    FLIST_STATUS_OFFLINE,
    FLIST_STATUS_UNKNOWN /* if we don't recognize the status string */
};

enum FListGender_ { /* flags make for quick comparisons */
    FLIST_GENDER_NONE = 0x1,
    FLIST_GENDER_MALE = 0x2,
    FLIST_GENDER_FEMALE = 0x4,
    FLIST_GENDER_HERM = 0x8,
    FLIST_GENDER_TRANSGENDER = 0x10,
    FLIST_GENDER_SHEMALE = 0x20,
    FLIST_GENDER_MALEHERM = 0x40,
    FLIST_GENDER_CUNTBOY = 0x80,
    FLIST_GENDER_UNKNOWN = 0x100
};

enum FListChannelMode_ {
    CHANNEL_MODE_BOTH = 0,
    CHANNEL_MODE_ADS_ONLY = 1,
    CHANNEL_MODE_CHAT_ONLY = 2,
    CHANNEL_MODE_UNKNOWN = 16
};

enum FListFriendStatus_ {
    FLIST_NOT_FRIEND            = 0x0, /* no friend status */
    FLIST_PENDING_OUT_FRIEND    = 0x1, /* awaiting his authorization */
    FLIST_PENDING_IN_FRIEND     = 0x2, /* needs your authorization */
    FLIST_MUTUAL_FRIEND         = 0x4, /* mutual friend */
    FLIST_FRIEND_STATUS_UNKNOWN = 0x10
};

enum FListFriendsRequestType_ {
    FLIST_FRIEND_REQUEST,
    FLIST_FRIEND_CANCEL,
    FLIST_FRIEND_REMOVE,
    FLIST_BOOKMARK_ADD,
    FLIST_BOOKMARK_REMOVE,
    FLIST_FRIEND_AUTHORIZE,
    FLIST_FRIEND_DENY,
    FLIST_FRIENDS_UPDATE
};

/* gender conversion */
GSList *flist_get_gender_list();
FListGender flist_parse_gender(const gchar *gender_string);
const gchar *flist_format_gender(FListGender gender);
const gchar *flist_format_gender_color(FListGender gender);
const gchar *flist_gender_color(FListGender gender);

/* status conversion */
GSList *flist_get_status_list();
FListStatus flist_parse_status(const gchar *status_string);
const gchar *flist_format_status(FListStatus status);
const gchar *flist_internal_status(FListStatus status);

/* channel mode conversion */
FListChannelMode flist_parse_channel_mode(const gchar *);

/* friend status conversion */
const gchar *flist_format_friend_status(FListFriendStatus);

/* groups */
PurpleGroup *flist_get_filter_group(FListAccount*);
PurpleGroup *flist_get_bookmarks_group(FListAccount*);
PurpleGroup *flist_get_friends_group(FListAccount*);
PurpleGroup *flist_get_chat_group(FListAccount*);

PurpleTypingState flist_typing_state(const gchar *state);
const gchar *flist_typing_state_string(PurpleTypingState state);

void flist_g_list_free(GList *to_free);
void flist_g_list_free_full(GList *to_free, GDestroyNotify f);
void flist_g_slist_free_full(GSList *to_free, GDestroyNotify f);
GSList *flist_g_slist_intersect_and_free(GSList *list1, GSList *list2);

guint flist_str_hash(const char *);
gboolean flist_str_equal(const char *, const char *);
gint flist_strcmp(const char *nick1, const char *nick2);
const char *flist_normalize(const PurpleAccount *, const char *);

PurpleGroup *flist_get_chat_group(FListAccount*);
PurpleChat *flist_get_chat(FListAccount*, const gchar*);
void flist_remove_chat(FListAccount*, const gchar*);

gint json_object_get_parse_int_member(JsonObject *, const gchar *, gboolean *success);
gint json_array_get_parse_int_element(JsonArray *, guint, gboolean *success);

FListCharacter *flist_get_character(FListAccount *, const gchar *identity);
GSList *flist_get_all_characters(FListAccount *);

const gchar *flist_serialize_account(PurpleAccount *);
PurpleAccount *flist_deserialize_account(const gchar*);

gboolean flist_get_channel_show_ads(FListAccount *, const gchar *);
gboolean flist_get_channel_show_chat(FListAccount *, const gchar *);
void flist_set_channel_show_ads(FListAccount *, const gchar *, gboolean);
void flist_set_channel_show_chat(FListAccount *, const gchar *, gboolean);
void flist_channel_show_message(FListAccount *, const gchar *);


struct FListGenderStruct_ {
    FListGender gender;
    const gchar *name;
    const gchar *display_name;
    const gchar *color;
    const gchar *colored_name;
};

struct FListCharacter_ {
    gchar* name;
    FListGender gender;
    FListStatus status;
    gchar* status_message; /* this is stored html-escaped */
};

struct FListRoomlistChannel_ {
    gchar *name;
    gchar *title;
    gboolean is_public;
    guint users;
};

struct FListAccount_ {
    PurpleAccount *pa;
    PurpleConnection *pc;
    
    GHashTable *global_ops; //hash table of global operators
    GHashTable *all_characters; //hash table of FListCharacter, all that are online

    gint characters_remaining;
    gboolean online; //whether or not we've set pidgin to say we're online
    guint32 character_count; //total number of characters online, should be count(all_characters)
    gboolean secure; //whether or not we use ssl/tls for the socket, and https for web requests
    gchar *username;
    gchar *character;
    gchar *password;
    gchar *proper_character;
    
    PurpleUtilFetchUrlData *url_request;
    gchar *fls_cookie;
    FListConnectionStatus connection_status;
    
    /* for tickets */
    guint ticket_timer;
    FListWebRequestData *ticket_request;
    
    /* connection data */
    PurpleSslConnection *ssl_con;
    gchar *rx_buf;
    gsize rx_len;
    int fd;
    int input_handle;
    GByteArray *frame_buffer;
    
    PurpleRoomlist *roomlist;
    gboolean input_request;

    guint ping_timeout_handle;
    
    /* for the channel subsystem */
    GHashTable *chat_table; /* a hash table of open PurpleConvChat */
    GHashTable *chat_timestamp; /* the last time we attempted to join the chat */

    /* for the icon subsystem */
    GSList *icon_requests;
    GSList *icon_request_queue;

    /* connection options */
    gchar *server_address;
    gint server_port;
    gboolean use_websocket_handshake; /* enable to use handshake instead of WSH */

    /* filter subsystem */
    gchar *filter_channel;
    gboolean filter_looking;
    //gint filter_gender; //TODO: implement

    /* kinks subsystem */
    FListKinks *flist_kinks;

    /* profile subsystem */
    FListProfiles *flist_profiles;
    
    /* friends subsystem */
    gboolean sync_bookmarks;
    gboolean sync_friends;
    FListFriends *flist_friends;
    
    /* other options */
    gboolean debug_mode;
};

//f-list sources
#include "f-list_callbacks.h"
#include "f-list_commands.h"
#include "f-list_autobuddy.h"
#include "f-list_connection.h"
#include "f-list_icon.h"
#include "f-list_kinks.h"
#include "f-list_profile.h"
#include "f-list_bbcode.h"
#include "f-list_admin.h"
#include "f-list_channels.h"
#include "f-list_json.h"
#include "f-list_friends.h"
#include "f-list_status.h"
#include "f-list_rtb.h"
#include "f-list_pidgin.h" //TODO: maybe not include this ...

#endif
