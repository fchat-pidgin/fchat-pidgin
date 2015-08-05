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

void flist_purple_set_status(PurpleAccount *account, PurpleStatus *status) {
    g_return_if_fail(account);
    g_return_if_fail(status);

    PurpleConnection *pc = purple_account_get_connection(account);
    FListAccount *fla = pc->proto_data;
    g_return_if_fail(fla);

    if (fla->sync_status) {
        purple_debug_info(FLIST_DEBUG, "Processing purple status change to %s with message %s\n", purple_status_get_id(status), purple_status_get_attr_string(status, FLIST_STATUS_MESSAGE_KEY));
        flist_set_internal_status_from_purple_status(fla, status);
        flist_update_server_status(fla);
    }
}

void flist_set_internal_status_from_purple_status(FListAccount *fla, PurpleStatus *status) {
    PurpleStatusType *statusType = purple_status_get_type(status);
    GList *statusTypes = flist_status_types(fla->pa);
    GList *cur = statusTypes;
    FListStatus fStatus = FLIST_STATUS_UNKNOWN;
    gchar *message = NULL;
    
    // The status isn't active! bail!
    if (!purple_status_is_active(status))
        return;

    
    message = purple_unescape_html(purple_status_get_attr_string(status, FLIST_STATUS_MESSAGE_KEY));
    // First, get the presence. If it's idle, we default to idle.
    PurplePresence *presence = purple_status_get_presence(status);
    if(purple_presence_is_idle(presence)) {
        flist_set_status(fla, FLIST_STATUS_IDLE, message);
    }
    
    // Alright, not idle. Next, compare StatusType IDs. If it's a match, use that.
    while(cur) {
        PurpleStatusType *type = cur->data;
        if(strcmp(purple_status_type_get_id(type), purple_status_type_get_id(statusType)) == 0){
            fStatus = flist_parse_status(purple_status_type_get_id(statusType));
            break;
        } else {
            cur = g_list_next(cur);
        }
    }
    // Found a matching F-list Status. Use it!
    if(fStatus != FLIST_STATUS_UNKNOWN) {
        flist_set_status(fla, fStatus, 	message);
    } else {
        // Alright, seems the status we chose isn't an F-list one. Let's convert to the next best primitive.
        switch (purple_status_type_get_primitive(statusType)) {
            case PURPLE_STATUS_AWAY:
            case PURPLE_STATUS_EXTENDED_AWAY:
                flist_set_status(fla, FLIST_STATUS_AWAY, message);
                break;
            case PURPLE_STATUS_UNAVAILABLE:
                flist_set_status(fla, FLIST_STATUS_DND, message);
                break;
                
                // Assume AVAILABLE by default if it's not an AWAY/DND status
            default:
                flist_set_status(fla, FLIST_STATUS_AVAILABLE, message);
        }
    }
    g_list_free(statusTypes);
    g_free(message);
}
