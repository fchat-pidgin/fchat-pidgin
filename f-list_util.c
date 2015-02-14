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

#include "f-list_util.h"

void flist_parse_cookies_into_hash_table(const gchar *header, gsize len, GHashTable *hash_table)
{
    const static gchar* ident = "Set-Cookie: ";
    gsize ident_len = strlen(ident);

    gchar *start = g_strstr_len(header, len, ident);
    while (start)
    {
        gchar *key_end = g_strstr_len(start, len, "=");
        gchar *value_end = g_strstr_len(start, len, ";");

        if (!key_end || !value_end)
            break;

        gchar *key = g_strndup(start + ident_len, key_end-(start+ident_len));
        gchar *value = g_strndup(key_end + 1, value_end-(key_end+1));
        g_hash_table_insert(hash_table, key, value);
        start = g_strstr_len(start+1, len, ident);
    }
}

//TODO: you're supposed to change spaces to "+" values??
static void g_string_append_cgi(GString *str, GHashTable *table) {
    GHashTableIter iter;
    gpointer key, value;
    gboolean first = TRUE;
    g_hash_table_iter_init(&iter, table);
    while(g_hash_table_iter_next(&iter, &key, &value)) {
        purple_debug_info(FLIST_DEBUG, "cgi writing key, value: %s, %s\n", (gchar *)key, (gchar *)value);
        if(!first) g_string_append(str, "&");
        g_string_append_printf(str, "%s", purple_url_encode(key));
        g_string_append(str, "=");
        g_string_append_printf(str, "%s", purple_url_encode(value));
        first = FALSE;
    }
}

static void g_string_append_cookies(GString *str, GHashTable *table) {
    GHashTableIter iter;
    gpointer key, value;
    gboolean first = TRUE;
    g_hash_table_iter_init(&iter, table);
    while(g_hash_table_iter_next(&iter, &key, &value)) {
        if(!first) g_string_append(str, " ");
        g_string_append_printf(str, "%s", purple_url_encode(key));
        g_string_append(str, "=");
        g_string_append_printf(str, "%s;", purple_url_encode(value));
        first = FALSE;
    }
}

//mostly shamelessly stolen from pidgin's "util.c"
gchar *http_request(const gchar *url, gboolean http11, gboolean post, const gchar *user_agent, GHashTable *req_table, GHashTable *cookie_table) {
    GString *request_str = g_string_new(NULL);
    gchar *address = NULL, *page = NULL, *user = NULL, *password = NULL;
    int port;

    purple_url_parse(url, &address, &port, &page, &user, &password);

    g_string_append_printf(request_str, "%s /%s%s", (post ? "POST" : "GET"), page, (!post && req_table ? "?" : ""));
    if(req_table && !post) g_string_append_cgi(request_str, req_table);
    g_string_append_printf(request_str, " HTTP/%s\r\n", (http11 ? "1.1" : "1.0"));
    g_string_append_printf(request_str, "Connection: close\r\n");
    if(user_agent) g_string_append_printf(request_str, "User-Agent: %s\r\n", user_agent);
    g_string_append_printf(request_str, "Accept: */*\r\n");
    g_string_append_printf(request_str, "Host: %s\r\n", address);

    if(cookie_table) {
        g_string_append(request_str, "Cookie: ");
        g_string_append_cookies(request_str, cookie_table);
        g_string_append(request_str, "\r\n");
    }

    if(post) {
        GString *post_str = g_string_new(NULL);
        gchar *post = NULL;

        if(req_table) g_string_append_cgi(post_str, req_table);

        post = g_string_free(post_str, FALSE);

        purple_debug_info(FLIST_DEBUG, "posting (len: %" G_GSIZE_FORMAT "): %s\n", strlen(post), post);

        g_string_append(request_str, "Content-Type: application/x-www-form-urlencoded\r\n");
        g_string_append_printf(request_str, "Content-Length: %" G_GSIZE_FORMAT "\r\n", strlen(post));
        g_string_append(request_str, "\r\n");

        g_string_append(request_str, post);

        g_free(post);
    } else {
        g_string_append(request_str, "\r\n");
    }

    if(address) g_free(address);
    if(page) g_free(page);
    if(user) g_free(user);
    if(password) g_free(password);

    return g_string_free(request_str, FALSE);
}
