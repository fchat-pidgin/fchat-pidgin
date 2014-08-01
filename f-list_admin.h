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
#ifndef FLIST_ADMIN_H
#define	FLIST_ADMIN_H

#include "f-list.h"

void flist_create_public_channel_action(PurplePluginAction *);
void flist_delete_public_channel_action(PurplePluginAction *);
void flist_add_global_operator_action(PurplePluginAction *);
void flist_remove_global_operator_action(PurplePluginAction *);
void flist_broadcast_action(PurplePluginAction *);

PurpleCmdRet flist_admin_op_deop_cmd(PurpleConversation *, const gchar *, gchar **, gchar **, void *);
PurpleCmdRet flist_global_kick_ban_unban_cmd(PurpleConversation *, const gchar *, gchar **, gchar **, void *);
PurpleCmdRet flist_create_kill_channel_cmd(PurpleConversation *, const gchar *, gchar **, gchar **, void *);
PurpleCmdRet flist_broadcast_cmd(PurpleConversation *, const gchar *, gchar **, gchar **, void *);
PurpleCmdRet flist_timeout_cmd(PurpleConversation *, const gchar *, gchar **, gchar **, void *);
PurpleCmdRet flist_reward_cmd(PurpleConversation *, const gchar *, gchar **, gchar **, void *);

void flist_send_sfc_confirm(PurpleConnection *, const gchar *callid);
gboolean flist_process_SFC(PurpleConnection *, JsonObject *);

#endif	/* F_LIST_ADMIN_H */
