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

FListPermissionMask flist_get_permissions(FListAccount *fla, const gchar *character, const gchar *channel)
{
    FListPermissionMask ret = FLIST_PERMISSION_NONE;
    FListChannel *fchannel = channel ? flist_channel_find(fla, channel) : NULL;

    if(channel && !fchannel) {
        purple_debug_error(FLIST_DEBUG, "Flags requested for %s in channel %s, but no channel was found.\n", character, channel);
        return ret;
    }

    if(fchannel && fchannel->owner && !purple_utf8_strcasecmp(fchannel->owner, character))
        ret |= FLIST_PERMISSION_CHANNEL_OWNER;

    if(fchannel && g_list_find_custom(fchannel->operators, character, (GCompareFunc)purple_utf8_strcasecmp))
        ret |= FLIST_PERMISSION_CHANNEL_OP;

    if(g_hash_table_lookup(fla->global_ops, character) != NULL)
        ret |= FLIST_PERMISSION_GLOBAL_OP;

    return ret;
}

PurpleConvChatBuddyFlags flist_permissions_to_purple(FListPermissionMask permission)
{
    PurpleConvChatBuddyFlags flags = 0;

    if (FLIST_HAS_PERMISSION(permission, FLIST_PERMISSION_CHANNEL_OWNER))
        flags |= PURPLE_CBFLAGS_FOUNDER;

    if (FLIST_HAS_PERMISSION(permission, FLIST_PERMISSION_CHANNEL_OP))
        flags |= PURPLE_CBFLAGS_HALFOP;

    if (FLIST_HAS_PERMISSION(permission, FLIST_PERMISSION_GLOBAL_OP))
        flags |= PURPLE_CBFLAGS_OP;

    return flags;
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

        // We use g_uri_escape_string instead of purple_url_encode 
        // because the latter only supports strings up to 2048 bytes
        // (depending on BUF_LEN)
        gchar *encoded_key = g_uri_escape_string(key, NULL, FALSE);
        g_string_append_printf(str, "%s", encoded_key);
        g_free(encoded_key);

        g_string_append(str, "=");

        gchar *encoded_value = g_uri_escape_string(value, NULL, FALSE);
        g_string_append_printf(str, "%s", encoded_value);
        g_free(encoded_value);

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
