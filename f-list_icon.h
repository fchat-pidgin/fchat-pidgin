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
#ifndef FLIST_ICON_H
#define	FLIST_ICON_H

#include "f-list.h"

#define ICON_AVATAR_URL "https://static.f-list.net/images/avatar/%s.png"
#define ICON_EICON_URL "https://static.f-list.net/images/eicon/%s.gif"

void flist_fetch_icon(FListAccount *, const gchar *who);
void flist_fetch_account_icon(FListAccount *fla);
void flist_fetch_avatar(FListAccount *, const gchar *smiley, const gchar *who, PurpleConversation *convo);
void flist_fetch_eicon(FListAccount *, const gchar *smiley, const gchar *name, PurpleConversation *convo);
void flist_fetch_icon_cancel_all(FListAccount *);

#endif	/* F_LIST_ICON_H */
