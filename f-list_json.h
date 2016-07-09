#ifndef F_LIST_JSON_H
#define	F_LIST_JSON_H

#include "f-list.h"

#define JSON_GET_TICKET         "https://www.f-list.net/json/getApiTicket.php"
#define JSON_BOOKMARK_ADD       "https://www.f-list.net/json/api/bookmark-add.php"
#define JSON_BOOKMARK_REMOVE    "https://www.f-list.net/json/api/bookmark-remove.php"
#define JSON_FRIENDS_REQUEST    "https://www.f-list.net/json/api/request-send.php"
#define JSON_FRIENDS_REMOVE     "https://www.f-list.net/json/api/friend-remove.php"
#define JSON_FRIENDS_ACCEPT     "https://www.f-list.net/json/api/request-accept.php"
#define JSON_FRIENDS_DENY       "https://www.f-list.net/json/api/request-deny.php"
#define JSON_FRIENDS_CANCEL     "https://www.f-list.net/json/api/request-cancel.php"
#define JSON_FRIENDS            "https://www.f-list.net/json/api/friend-bookmark-lists.php"
#define JSON_CHARACTER_INFO     "https://www.f-list.net/json/api/character-info.php"

#define JSON_INFO_LIST          "https://www.f-list.net/json/api/info-list.php"
#define JSON_KINK_LIST          "https://www.f-list.net/json/api/kink-list.php"

#define JSON_UPLOAD_LOG         "https://www.f-list.net/json/api/report-submit.php"

typedef void            (*FListWebCallback)       (FListWebRequestData*, gpointer data, JsonObject *, const gchar *error);

FListWebRequestData* flist_web_request(const gchar*, GHashTable*, GHashTable* cookies, gboolean post, FListWebCallback, gpointer data);
void flist_web_request_cancel(FListWebRequestData*);

GHashTable *flist_web_request_args(FListAccount*);

void flist_web_requests_init();

#endif	/* F_LIST_JSON_H */
