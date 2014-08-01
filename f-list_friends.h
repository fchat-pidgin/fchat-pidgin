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
#ifndef FLIST_FRIENDS_H
#define	FLIST_FRIENDS_H

#include "f-list.h"

void flist_blist_node_action(PurpleBlistNode*, gpointer);

FListFriendStatus flist_friends_get_friend_status(FListAccount *fla, const gchar *character);
gboolean flist_friends_is_bookmarked(FListAccount *fla, const gchar *character);

//void flist_friends_add_buddy(FListAccount *fla, const gchar* character);
//void flist_friends_remove_buddy(FListAccount *fla, const gchar* character);

void flist_friends_login(FListAccount*);
void flist_friends_load(FListAccount*);
void flist_friends_unload(FListAccount*);

/* These tell the Friends List API to update. */

//This is received when we receive a friend invitation.
void flist_friends_received_request(FListAccount*);
//This is received when someone accepts your friend invitation.
void flist_friends_added_friend(FListAccount*);
//This is received when you remove a friend or a friend removes you.
void flist_friends_removed_friend(FListAccount*);

#endif /* FLIST_FRIENDS_H */
