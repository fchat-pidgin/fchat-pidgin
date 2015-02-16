
#include "f-list_json.h"

#define FLIST_WEB_REQUEST_TIMEOUT 30

struct FListWebRequestData_ {
    PurpleUtilFetchUrlData *url_data;
    FListWebCallback cb;
    gpointer user_data;
    guint timer;
};

GHashTable* requests; //TODO: Replace this with a simple list, perhaps?

void flist_web_request_cancel(FListWebRequestData *req_data) {
    g_return_if_fail(req_data != NULL);
    purple_util_fetch_url_cancel(req_data->url_data);
    g_hash_table_remove(requests, req_data);
    purple_timeout_remove(req_data->timer);
    g_free(req_data);
}

void flist_web_request_cb(PurpleUtilFetchUrlData *url_data, gpointer user_data, const gchar *url_text, gsize len, const gchar *error_message) {
    FListWebRequestData *req_data = user_data;

    if(!url_text) {
        purple_debug_warning(FLIST_DEBUG, "Web Request failed with error message: %s\n", error_message);
        req_data->cb(req_data, req_data->user_data, NULL, error_message);
    } else {
        JsonParser *parser;
        JsonNode *root;
        GError *err = NULL;

        purple_debug_info(FLIST_DEBUG, "Web Request JSON Received: %s\n", url_text);

        parser = json_parser_new();
        json_parser_load_from_data(parser, url_text, len, &err);

        if(err) { /* not valid json */
            purple_debug_warning(FLIST_DEBUG, "Expected JSON Object, but did not parse with error message: %s\n", err->message);
            purple_debug_warning(FLIST_DEBUG, "Raw JSON: %s\n", url_text);
            req_data->cb(req_data, req_data->user_data, NULL, "Invalid JSON.");
            g_error_free(err);
        } else {
            root = json_parser_get_root(parser);
            if(json_node_get_node_type(root) != JSON_NODE_OBJECT) {
                purple_debug_warning(FLIST_DEBUG, "Expected JSON Object, but received a different node.\n");
                purple_debug_warning(FLIST_DEBUG, "Raw JSON: %s\n", url_text);
                req_data->cb(req_data, req_data->user_data, NULL, "Invalid JSON.");
            } else {
                req_data->cb(req_data, req_data->user_data, json_node_get_object(root), NULL);
            }
        }
        g_object_unref(parser);
    }

    purple_timeout_remove(req_data->timer);
    g_free(req_data);
}

gboolean flist_web_request_timeout(gpointer data) {
    FListWebRequestData *req_data = data;
    g_return_val_if_fail(req_data != NULL, FALSE);
    purple_util_fetch_url_cancel(req_data->url_data);
    req_data->cb(req_data, req_data->user_data, NULL, "Web Request timed out.");
    g_hash_table_remove(requests, req_data);
    g_free(req_data);
    return FALSE;
}

FListWebRequestData* flist_web_request(const gchar* url, GHashTable* args, GHashTable *cookies, gboolean post, gboolean secure, FListWebCallback cb, gpointer data) {
    gchar *full_url = g_strdup_printf("%s%s", secure ? "https://" : "http://", url);
    gchar *http = http_request(full_url, TRUE, post, USER_AGENT, args, cookies);
    FListWebRequestData *ret = g_new0(FListWebRequestData, 1);
    PurpleUtilFetchUrlData *url_data = purple_util_fetch_url_request(full_url, FALSE, USER_AGENT, FALSE, http, FALSE, flist_web_request_cb, ret);
    ret->url_data = url_data;
    ret->cb = cb;
    ret->user_data = data;
    ret->timer = purple_timeout_add_seconds(FLIST_WEB_REQUEST_TIMEOUT, (GSourceFunc) flist_web_request_timeout, ret);
    g_hash_table_insert(requests, ret, ret);
    g_free(http);
    g_free(full_url);
    return ret;
}

void flist_web_requests_init() {
    requests = g_hash_table_new_full(NULL, NULL, NULL, NULL);
}

GHashTable *flist_web_request_args(FListAccount *fla) {
    GHashTable *ret = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);
    const gchar *ticket = flist_get_ticket(fla);
    if(fla->username) g_hash_table_insert(ret, "account", g_strdup(fla->username));
    if(ticket) g_hash_table_insert(ret, "ticket", g_strdup(ticket));
    return ret;
}
