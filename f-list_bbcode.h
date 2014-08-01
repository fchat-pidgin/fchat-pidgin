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
#ifndef FLIST_BBCODE_H
#define	FLIST_BBCODE_H

#include "f-list.h"

gchar *flist_bbcode_to_html(FListAccount *, PurpleConversation *, const gchar *);
gchar *flist_bbcode_strip(const gchar *);
gchar *flist_strip_crlf(const gchar *);
void flist_bbcode_init();

#endif	/* F_LIST_BBCODE_H */

