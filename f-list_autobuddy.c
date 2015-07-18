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
#include "f-list_autobuddy.h"

void flist_apply_filter(FListAccount *fla, GSList *candidates) {
    PurpleGroup *filter_group = NULL;
    GSList *buddies, *cur;
    GSList *removed = NULL;

    filter_group = flist_get_filter_group(fla);

    buddies = purple_find_buddies(fla->pa, NULL);
    cur = buddies;
    while(cur) {
        PurpleBuddy *b = cur->data; cur = g_slist_next(cur);
        const gchar *name = purple_buddy_get_name(b);
        if(purple_buddy_get_group(b) == filter_group) {
            if(!g_slist_find_custom(candidates, name, (GCompareFunc) flist_strcmp)) { //TODO: this is extremely inefficient, fix it!
                removed = g_slist_prepend(removed, b);
            }
        }
    }
    g_slist_free(buddies);

    cur = removed;
    while(cur) {
        PurpleBuddy *b = cur->data; cur = g_slist_next(cur);
        purple_blist_remove_buddy(b);
    }
    g_slist_free(removed);

    cur = candidates;
    while(cur) {
        const gchar *name = cur->data; cur = g_slist_next(cur);
        purple_debug_info(FLIST_DEBUG, "adding %s\n", name);
        PurpleBuddy *b = purple_find_buddy(fla->pa, name);
        if(!b) { /* we're adding a new buddy */
            b = purple_buddy_new(fla->pa, name, NULL);
            purple_blist_add_buddy(b, NULL, filter_group, NULL);
            purple_account_add_buddy(fla->pa, b);
        } /* do nothing if this is already a buddy */
    }
}

void flist_clear_filter(FListAccount *fla) {
    flist_apply_filter(fla, NULL);
}

void flist_convo_closed(PurpleConnection *pc, const char *who) {
    FListAccount *fla = pc->proto_data;
    PurpleGroup *im_group = flist_get_im_group(fla);

    /* Try to find if this was a temporary IM, and if it was, remove it */
    PurpleBuddy *buddy = purple_find_buddy_in_group(fla->pa, who, im_group);
    if (buddy) {
        purple_debug_info(FLIST_DEBUG, "Removing temporary IM buddy %s\n", who);
        purple_blist_remove_buddy(buddy);
    }
}

void flist_temp_im_check(FListAccount *fla, const char *character) {
    PurpleBuddy *buddy = purple_find_buddy(fla->pa, character);
    if (!buddy) {
        purple_debug_info(FLIST_DEBUG, "Adding temporary IM buddy %s\n", character);
        PurpleGroup *im_group = flist_get_im_group(fla);
        PurpleBuddy *b = purple_buddy_new(fla->pa, character, NULL);
        purple_blist_add_buddy(b, NULL, im_group, NULL);
        purple_account_add_buddy(fla->pa, b);
    }
}

