#ifndef F_LIST_JSON_H
#define	F_LIST_JSON_H

#include "f-list.h"

#define JSON_GET_TICKET         "www.f-list.net/json/getApiTicket.php"
#define JSON_BOOKMARK_ADD       "www.f-list.net/json/api/bookmark-add.php"
#define JSON_BOOKMARK_REMOVE    "www.f-list.net/json/api/bookmark-remove.php"
#define JSON_FRIENDS_REQUEST    "www.f-list.net/json/api/request-send.php"
#define JSON_FRIENDS_REMOVE     "www.f-list.net/json/api/friend-remove.php"
#define JSON_FRIENDS_ACCEPT     "www.f-list.net/json/api/request-accept.php"
#define JSON_FRIENDS_DENY       "www.f-list.net/json/api/request-deny.php"
#define JSON_FRIENDS_CANCEL     "www.f-list.net/json/api/request-cancel.php"
#define JSON_FRIENDS            "www.f-list.net/json/api/friend-bookmark-lists.php"
#define JSON_CHARACTER_INFO     "www.f-list.net/json/api/character-info.php"

#define JSON_INFO_LIST          "www.f-list.net/json/api/info-list.php"
#define JSON_KINK_LIST          "www.f-list.net/json/api/kink-list.php"

typedef void            (*FListWebCallback)       (FListWebRequestData*, gpointer data, JsonObject *, const gchar *error);

FListWebRequestData* flist_web_request(const gchar*, GHashTable*, gboolean post, gboolean secure, FListWebCallback, gpointer data);
void flist_web_request_cancel(FListWebRequestData*);

GHashTable *flist_web_request_args(FListAccount*);

void flist_web_requests_init();

#endif	/* F_LIST_JSON_H */
