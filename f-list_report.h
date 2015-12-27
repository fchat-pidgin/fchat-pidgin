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

#ifndef FLIST_REPORT_H
#define FLIST_REPORT_H

#include "f-list.h"

struct FListReport_ {
    FListAccount* fla;
    PurpleConversation *convo;
    gchar *channel_handle;
    gchar *channel_pretty;
    gchar *character;
    gchar *reason;
    gchar *log_text;
    gchar *log_id;
};

typedef struct FListReport_ FListReport;

PurpleCmdRet flist_report_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data);
FListReport *flist_report_new(FListAccount *fla, PurpleConversation *convo, const gchar *reported_character, const gchar* reason);
void flist_report_free(FListReport *flr);
void flist_report_display_ui(FListReport *flr);

#endif /* FLIST_REPORT_H */
