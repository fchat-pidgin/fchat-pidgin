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
#include "f-list_icon.h"

#define FLIST_REQUESTS_PER_SECOND 8 //TODO: implement this!
#define FLIST_MAX_ICON_REQUESTS 4

typedef struct FListFetchIcon_ {
    FListAccount *fla;
    PurpleUtilFetchUrlData *url_data;
    gchar *convo; //TODO: store convo type for rare possible case that chat and im have same name?
    gchar *smiley;
    gchar *character;
} FListFetchIcon;

static void flist_fetch_icon_real(FListAccount *, FListFetchIcon* fli);

static void flist_fetch_icon_cb(PurpleUtilFetchUrlData *url_data, gpointer data, const gchar *b, size_t len, const gchar *err) {
    FListFetchIcon *fli = data;
    FListAccount *fla = fli->fla;
    const gchar *character, *checksum;

    checksum = "0"; /* what the fuck is this checksum supposed to be?? */
    character = fli->character;
    
    if(err) {
        if(fli->convo) {
            PurpleConversation *convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_ANY, fli->convo, fla->pa);
            if(convo) {
                purple_conv_custom_smiley_close(convo, fli->smiley);
            }
        }
        purple_debug_warning(FLIST_DEBUG, "Failed to fetch a character icon. (Character: %s) (Convo: %s) (Secure: %s) (Error Message: %s)\n",
            fli->character, fli->convo ? fli->convo : "none", fla->secure ? "yes" : "no", err);
    } else {
        purple_debug_info(FLIST_DEBUG, "Received character icon. (Character: %s) (Convo: %s) (Secure: %s)\n",
                fli->character, fli->convo ? fli->convo : "none", fla->secure ? "yes" : "no");
        if(fli->convo) {
            PurpleConversation *convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_ANY, fli->convo, fla->pa);
            if(convo) {
                purple_debug_info(FLIST_DEBUG, "Writing character icon to convo... (Smiley: %s)\n", fli->smiley);
                purple_conv_custom_smiley_write(convo, fli->smiley, (const guchar *) b, len);
                purple_conv_custom_smiley_close(convo, fli->smiley);
            }
        } else {
            purple_buddy_icons_set_for_user(fla->pa, character, g_memdup(b, len), len, checksum);
        }
    }

    fla->icon_requests = g_slist_remove(fla->icon_requests, fli);

    if(fli->convo) g_free(fli->convo);
    if(fli->smiley) g_free(fli->smiley);
    g_free(fli->character);
    g_free(fli);

    gboolean done = FALSE;
    while(fla->icon_request_queue && !done) {
        FListFetchIcon *new_fli = fla->icon_request_queue->data;
        fla->icon_request_queue = g_slist_delete_link(fla->icon_request_queue, fla->icon_request_queue);
        if(new_fli->convo || purple_find_buddy(fla->pa, new_fli->character)) {
            flist_fetch_icon_real(fla, new_fli);
            done = TRUE;
        }
    }
}

static void flist_fetch_icon_real(FListAccount *fla, FListFetchIcon *fli) {
    gchar *character_lower;
    gchar *url;
    
    purple_debug_info(FLIST_DEBUG, "Fetching character icon... (Character: %s) (Convo: %s) (Secure: %s)\n", 
        fli->character, fli->convo ? fli->convo : "none", fla->secure ? "yes" : "no");

    character_lower = g_utf8_strdown(fli->character, -1);
    url = g_strdup_printf("%sstatic.f-list.net/images/avatar/%s.png", fla->secure ? "https://" : "http://", purple_url_encode(character_lower));
    fli->url_data = purple_util_fetch_url_request(url, TRUE, USER_AGENT, TRUE, NULL, FALSE, flist_fetch_icon_cb, fli);
    fla->icon_requests = g_slist_prepend(fla->icon_requests, fli);
    
    g_free(url);
    g_free(character_lower);
}

void flist_fetch_icon(FListAccount *fla, const gchar *character) {
    FListFetchIcon *fli = g_new0(FListFetchIcon, 1);
    int len;

    fli->character = g_strdup(character);
    fli->fla = fla;
    
    len = g_slist_length(fla->icon_requests);
    if(len < FLIST_MAX_ICON_REQUESTS) {
        flist_fetch_icon_real(fla, fli);
    } else {
        fla->icon_request_queue = g_slist_append(fla->icon_request_queue, fli);
    }
}

void flist_fetch_emoticon(FListAccount *fla, const gchar *smiley, const gchar *character, PurpleConversation *convo) {
    FListFetchIcon *fli;
    int len;
    
    if(!purple_conv_custom_smiley_add(convo, smiley, "none", "0", TRUE)) {
        return; /* for some reason or another we can't add it */
    }
    
    fli = g_new0(FListFetchIcon, 1);
    fli->character = g_strdup(character);
    fli->fla = fla;
    fli->convo = g_strdup(purple_conversation_get_name(convo));
    fli->smiley = g_strdup(smiley);
    
    len = g_slist_length(fla->icon_requests);
    if(len < FLIST_MAX_ICON_REQUESTS) {
        flist_fetch_icon_real(fla, fli);
    } else {
        fla->icon_request_queue = g_slist_append(fla->icon_request_queue, fli);
    }
}

void flist_fetch_icon_cancel_all(FListAccount *fla) {
    GSList *req;

    req = fla->icon_requests;
    while(req) {
        FListFetchIcon *fli = req->data;
        g_free(fli->character);
        if(fli->convo) {
            /* POSSIBLE WARNING: We do not cancel writing the custom smiley. */
            g_free(fli->convo);
        }
        if(fli->smiley) g_free(fli->smiley);
        purple_util_fetch_url_cancel(fli->url_data);
        g_free(fli);
        req = g_slist_next(req);
    }
    g_slist_free(fla->icon_requests);
    fla->icon_requests = NULL;

    req = fla->icon_request_queue;
    while(req) {
        g_free(req->data);
        req = g_slist_next(req);
    }
    fla->icon_request_queue = NULL;
}
