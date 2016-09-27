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
    gchar *url;
    gchar *character;
} FListFetchIcon;

static void flist_fetch_icon_real(FListAccount *, FListFetchIcon* fli);

static void flist_fetch_icon_cb(PurpleUtilFetchUrlData *url_data, gpointer data, const gchar *b, size_t len, const gchar *err) {
    FListFetchIcon *fli = data;
    FListAccount *fla = fli->fla;
    const gchar *checksum;

    checksum = "0"; /* what the fuck is this checksum supposed to be?? */

    if(err) {
        if(fli->convo) {
            PurpleConversation *convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_ANY, fli->convo, fla->pa);
            if(convo) {
                purple_conv_custom_smiley_close(convo, fli->smiley);
            }
        }
        purple_debug_warning(FLIST_DEBUG, "Failed to fetch an icon. (URL: %s) (Convo: %s) (Error Message: %s)\n",
            fli->url, fli->convo ? fli->convo : "none", err);
    } else {
        purple_debug_info(FLIST_DEBUG, "Received icon. (URL: %s) (Convo: %s)\n",
                fli->url, fli->convo ? fli->convo : "none");
        if(fli->convo) {
            PurpleConversation *convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_ANY, fli->convo, fla->pa);
            if(convo) {
                purple_debug_info(FLIST_DEBUG, "Writing icon to convo... (Smiley: %s)\n", fli->smiley);
                purple_conv_custom_smiley_write(convo, fli->smiley, (const guchar *) b, len);
                purple_conv_custom_smiley_close(convo, fli->smiley);
            }
        } else {
            if (fli->character)
            {
                // check if the icon is for my account ...
                if (!purple_utf8_strcasecmp(fli->character, fla->character))
                {
                    purple_buddy_icons_set_account_icon(fla->pa, g_memdup(b, len), len);
                }
                // ... or a buddy
                else
                {
                    purple_buddy_icons_set_for_user(fla->pa, fli->character, g_memdup(b, len), len, checksum);
                }
            }
        }
    }

    fla->icon_requests = g_slist_remove(fla->icon_requests, fli);

    if(fli->convo) g_free(fli->convo);
    if(fli->smiley) g_free(fli->smiley);
    g_free(fli->character);
    g_free(fli->url);
    g_free(fli);

    gboolean done = FALSE;
    while(fla->icon_request_queue && !done) {
        FListFetchIcon *new_fli = fla->icon_request_queue->data;
        fla->icon_request_queue = g_slist_delete_link(fla->icon_request_queue, fla->icon_request_queue);

        // TODO why? and how? and ... what?
        // Are there icon requests without convo nor character? Where do they come from?
        if(new_fli->convo || (new_fli->character && purple_find_buddy(fla->pa, new_fli->character))) {
            flist_fetch_icon_real(fla, new_fli);
            done = TRUE;
        }
    }
}

static void flist_fetch_icon_real(FListAccount *fla, FListFetchIcon *fli) {
    purple_debug_info(FLIST_DEBUG, "Fetching icon... (URL: %s) (Convo: %s)\n",
        fli->url, fli->convo ? fli->convo : "none");

    fli->url_data = purple_util_fetch_url_request(fli->url, TRUE, USER_AGENT, TRUE, NULL, FALSE, flist_fetch_icon_cb, fli);
    fla->icon_requests = g_slist_prepend(fla->icon_requests, fli);
}

void flist_fetch_account_icon(FListAccount *fla)
{
    flist_fetch_icon(fla, fla->character);
}

void flist_fetch_icon(FListAccount *fla, const gchar *character) {
    FListFetchIcon *fli = g_new0(FListFetchIcon, 1);
    int len;

    gchar *character_lower = g_utf8_strdown(character, -1);
    fli->character = g_strdup(character);
    fli->url = g_strdup_printf(ICON_AVATAR_URL, purple_url_encode(character_lower));
    fli->fla = fla;

    g_free(character_lower);

    len = g_slist_length(fla->icon_requests);
    if(len < FLIST_MAX_ICON_REQUESTS) {
        flist_fetch_icon_real(fla, fli);
    } else {
        fla->icon_request_queue = g_slist_append(fla->icon_request_queue, fli);
    }
}

void flist_fetch_avatar(FListAccount *fla, const gchar *smiley, const gchar *character, PurpleConversation *convo) {
    FListFetchIcon *fli;
    int len;

    if(!purple_conv_custom_smiley_add(convo, smiley, "none", "0", TRUE)) {
        return; /* for some reason or another we can't add it */
    }

    gchar *character_lower = g_utf8_strdown(character, -1);

    fli = g_new0(FListFetchIcon, 1);
    fli->character = g_strdup(character);
    fli->url = g_strdup_printf(ICON_AVATAR_URL, purple_url_encode(character_lower));
    fli->fla = fla;
    fli->convo = g_strdup(purple_conversation_get_name(convo));
    fli->smiley = g_strdup(smiley);

    g_free(character_lower);

    len = g_slist_length(fla->icon_requests);
    if(len < FLIST_MAX_ICON_REQUESTS) {
        flist_fetch_icon_real(fla, fli);
    } else {
        fla->icon_request_queue = g_slist_append(fla->icon_request_queue, fli);
    }
}

void flist_fetch_eicon(FListAccount *fla, const gchar *smiley, const gchar *name, PurpleConversation *convo) {
    FListFetchIcon *fli;
    int len;

    if(!purple_conv_custom_smiley_add(convo, smiley, "none", "0", TRUE)) {
        return; /* for some reason or another we can't add it */
    }

    gchar *name_lower = g_utf8_strdown(name, -1);

    fli = g_new0(FListFetchIcon, 1);
    fli->character = NULL;
    fli->url = g_strdup_printf(ICON_EICON_URL, purple_url_encode(name_lower));
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
        g_free(fli->url);
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
