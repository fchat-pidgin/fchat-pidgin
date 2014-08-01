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
#ifndef F_LIST_STATUS_H
#define	F_LIST_STATUS_H

#include "f-list.h"

void flist_update_server_status(FListAccount *fla);
void flist_set_status(FListAccount *fla, FListStatus status, const gchar *status_message);

const gchar *flist_get_status_message(FListAccount *fla);
FListStatus flist_get_status(FListAccount *fla);

#endif	/* F_LIST_STATUS_H */

