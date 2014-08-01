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
#ifndef FLIST_CHANNELS_H
#define	FLIST_CHANNELS_H

#include "f-list.h"

typedef struct FListChannel_ FListChannel;

struct FListChannel_ {
    gchar *name;
    gchar *title;
    GHashTable *users;
    gchar *owner;
    GList *operators;
    FListChannelMode mode;
    gchar *topic;
};

FListFlags flist_get_flags(FListAccount *, const gchar *channel, const gchar *identity);

void flist_channel_print_error(PurpleConversation *convo, const gchar *message);

void flist_update_user_chats_offline(PurpleConnection *, const gchar *);
void flist_update_user_chats_rank(PurpleConnection *, const gchar *);
void flist_update_users_chats_rank(PurpleConnection *, GList *);

void flist_got_channel_joined(FListAccount *, const gchar *);
void flist_got_channel_left(FListAccount *, const gchar *);
void flist_got_channel_oplist(FListAccount *, const gchar *channel, GList *oplist);
void flist_got_channel_userlist(FListAccount *, const gchar *channel, GList *users);
void flist_got_channel_user_joined(FListAccount *, const gchar *, const gchar *);
void flist_got_channel_user_left(FListAccount *, const gchar *, const gchar *, const gchar *);


FListChannel *flist_channel_find(FListAccount *, const gchar *channel);
FListChannel *flist_channel_new(FListAccount *, const gchar *channel);
const gchar *flist_channel_get_title(FListChannel *);
GList *flist_channel_list_names(FListAccount *);
GList *flist_channel_list_all(FListAccount *);

void flist_channel_subsystem_load(FListAccount*);
void flist_channel_subsystem_unload(FListAccount*);

PurpleCmdRet flist_channel_code_cmd(PurpleConversation *, const gchar *, gchar **args, gchar **error, void *);
PurpleCmdRet flist_channel_oplist_cmd(PurpleConversation *, const gchar *, gchar **args, gchar **error, void *);
PurpleCmdRet flist_channel_op_deop_cmd(PurpleConversation *, const gchar *, gchar **args, gchar **error, void *);
PurpleCmdRet flist_channel_join_cmd(PurpleConversation *, const gchar *, gchar **args, gchar **error, void *);
PurpleCmdRet flist_channel_banlist_cmd(PurpleConversation *, const gchar *, gchar **args, gchar **error, void *);
PurpleCmdRet flist_channel_who_cmd(PurpleConversation *, const gchar *, gchar **args, gchar **error, void *);
PurpleCmdRet flist_channel_open_cmd(PurpleConversation *, const gchar *, gchar **args, gchar **error, void *);
PurpleCmdRet flist_channel_close_cmd(PurpleConversation *, const gchar *, gchar **args, gchar **error, void *);
PurpleCmdRet flist_channel_show_topic_cmd(PurpleConversation *, const gchar *, gchar **args, gchar **error, void *);
PurpleCmdRet flist_channel_show_raw_topic_cmd(PurpleConversation *, const gchar *, gchar **args, gchar **error, void *);
PurpleCmdRet flist_channel_set_topic_cmd(PurpleConversation *, const gchar *, gchar **args, gchar **error, void *);
PurpleCmdRet flist_channel_kick_ban_unban_cmd(PurpleConversation *, const gchar *, gchar **args, gchar **error, void *);
PurpleCmdRet flist_channel_invite_cmd(PurpleConversation *, const gchar *, gchar **args, gchar **error, void *);
PurpleCmdRet flist_channel_make_cmd(PurpleConversation *, const gchar *, gchar **args, gchar **error, void *);

gboolean flist_process_COL(PurpleConnection *, JsonObject *);
gboolean flist_process_JCH(PurpleConnection *, JsonObject *);
gboolean flist_process_LCH(PurpleConnection *, JsonObject *);
gboolean flist_process_ICH(PurpleConnection *, JsonObject *);
gboolean flist_process_CKU(PurpleConnection *, JsonObject *);
gboolean flist_process_CBU(PurpleConnection *, JsonObject *);
gboolean flist_process_CDS(PurpleConnection *, JsonObject *);
gboolean flist_process_CIU(PurpleConnection *, JsonObject *);

#endif	/* F_LIST_CHANNELS_H */

