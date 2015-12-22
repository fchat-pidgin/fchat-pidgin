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

#ifndef FLIST_UTIL_H
#define FLIST_UTIL_H

#include "f-list.h"

#define FLIST_PERMISSION_NONE               0x00
#define FLIST_PERMISSION_CHANNEL_OP         0x01
#define FLIST_PERMISSION_CHANNEL_OWNER      0x02
#define FLIST_PERMISSION_GLOBAL_OP          0x04
#define FLIST_PERMISSION_ADMINISTRATOR      0x08
#define FLIST_HAS_PERMISSION(bitmask, permission) ((bitmask) & (permission))
#define FLIST_HAS_MIN_PERMISSION(bitmask, permission) ((bitmask) >= (permission))
#define FLIST_GET_PURPLE_PERMISSIONS(fla, character, channel) (flist_permissions_to_purple(flist_get_permissions((fla), (character), (channel))))

typedef unsigned char FListPermissionMask;

FListPermissionMask flist_get_permissions(FListAccount *fla, const gchar *character, const gchar *channel);
PurpleConvChatBuddyFlags flist_permissions_to_purple(FListPermissionMask permission);

gchar *http_request(const gchar *url, gboolean http11, gboolean post, const gchar *user_agent, GHashTable *req_table, GHashTable *cookie_table);

#endif
