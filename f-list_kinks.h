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
#ifndef FLIST_KINKS_H
#define	FLIST_KINKS_H

#include "f-list.h"

#define FLIST_GLOBAL_KINKS_URL "http://www.f-list.net/api/get/kinklist/?mode=extended"

gboolean flist_process_FKS(PurpleConnection *, JsonObject *);

void flist_filter_action(PurplePluginAction *action);

void flist_global_kinks_load(PurpleConnection *);
void flist_global_kinks_unload(PurpleConnection *);

PurpleCmdRet flist_filter_cmd(PurpleConversation *, const gchar *, gchar **, gchar **, void *);

#endif	/* KINKS_H */

