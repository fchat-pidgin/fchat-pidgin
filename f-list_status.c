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
#include "f-list_status.h"

//This file is currently mostly unnecessary, but more will likely be added later.

void flist_update_server_status(FListAccount *fla) {
    JsonObject *json = json_object_new();
    const gchar *status = purple_account_get_string(fla->pa, "_status", "online");
    const gchar *status_message = purple_account_get_string(fla->pa, "_status_message", "");
    json_object_set_string_member(json, "status", status);
    json_object_set_string_member(json, "statusmsg", status_message);
    flist_request(fla->pc, FLIST_SET_STATUS, json);
    
    json_object_unref(json);
}

void flist_set_status(FListAccount *fla, FListStatus status, const gchar *status_message) {
    purple_account_set_string(fla->pa, "_status_message", status_message);
    purple_account_set_string(fla->pa, "_status", flist_internal_status(status));
}

FListStatus flist_get_status(FListAccount *fla) {
    return flist_parse_status(purple_account_get_string(fla->pa, "_status", "online"));
}

const gchar *flist_get_status_message(FListAccount *fla) {
    return purple_account_get_string(fla->pa, "_status_message", "");
}
