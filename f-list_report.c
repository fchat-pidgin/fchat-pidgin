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

#include "f-list_report.h"

static void flist_upload_log_cb(FListWebRequestData* req_data, gpointer user_data,
        JsonObject *root, const gchar *error_message) {

    if (!root) {
         purple_debug_warning(FLIST_DEBUG, "Log upload failed! Error Message: %s\n", error_message);
         flist_report_free(user_data);
         return;
    }

    const gchar *error = json_object_get_string_member(root, "error");
    if (error && strlen(error))
    {
         purple_debug_warning(FLIST_DEBUG, "Log upload failed! Server error Message: %s\n", error_message);
         flist_report_free(user_data);
         return;
    }

    const gchar *log_id = json_object_get_string_member(root, "log_id");
    if (log_id && strlen(log_id))
    {
        purple_debug_info(FLIST_DEBUG, "Log upload succesful, id: %s\n", log_id);
        flist_report_free(user_data);
    }
}

static void flist_report_cb(gpointer user_data, PurpleRequestFields *fields) {
    FListReport *flr = user_data;
    flr->reason = g_strdup(purple_request_fields_get_string(fields, "reason"));
    flr->character = g_strdup(purple_request_fields_get_string(fields, "character"));

    GList *logs, *current_log;
    PurpleLogType log_type = flr->convo->type == PURPLE_CONV_TYPE_CHAT ? PURPLE_LOG_CHAT : PURPLE_LOG_IM;

    logs = purple_log_get_logs(log_type, flr->channel, flr->fla->pa);

    if (!purple_conversation_is_logging(flr->convo) || g_list_length(logs) == 0)
    {
        purple_notify_info(purple_conversation_get_gc(flr->convo), "No logs found!", "Could not find logs for this conversation. Make sure you have logging enabled before using /report!", NULL);
        g_list_free(logs);
        flist_report_free(flr);
        return;
    }

    /*GList *elem;
    for(elem = logs; elem != NULL; elem = elem->next)
    {
        PurpleLog *log = (PurpleLog*)elem->data;
        purple_debug_info(FLIST_DEBUG, "Log: %s\n", log->name);
        gchar *log_text = purple_log_read(log, NULL);
        if (log_text && *log_text)
        {
            purple_debug_info(FLIST_DEBUG, "-------------------------------------\n");
            purple_debug_info(FLIST_DEBUG, "%s\n", log_text);
            purple_debug_info(FLIST_DEBUG, "-------------------------------------\n");
        }
        g_free(log_text);
    }*/

    current_log = g_list_first(logs);
    gchar *log_text = purple_log_read(current_log->data, 0);
    flr->log = purple_markup_strip_html(log_text);

    purple_debug_info(FLIST_DEBUG, "User filed a report against '%s': '%s'\n------------- LOG -------------\n%s\n-----------------------------\n", flr->character, flr->reason, flr->log);

    GHashTable *args = flist_web_request_args(flr->fla);
    g_hash_table_insert(args, "character", g_strdup(flr->fla->proper_character));
    g_hash_table_insert(args, "log", g_strdup(flr->log));
    g_hash_table_insert(args, "reportText", g_strdup(flr->reason));
    g_hash_table_insert(args, "reportUser", g_strdup(flr->character));
    g_hash_table_insert(args, "channel", g_strdup(flr->channel));

    flist_web_request(JSON_UPLOAD_LOG, args, flr->fla->cookies, TRUE, flr->fla->secure, flist_upload_log_cb, flr);

    g_hash_table_destroy(args);
}

static void flist_report_cancel_cb(gpointer user_data) {
    FListReport *flr = user_data;
    flist_report_free(flr);
}

void flist_report_free(FListReport *flr)
{
    g_free(flr->channel);
    g_free(flr->character);
    g_free(flr->reason);
    g_free(flr->log);
    g_free(flr);
}

PurpleCmdRet flist_report_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    gchar *character = args[0];
    const gchar *channel = NULL;

    if (convo->type == PURPLE_CONV_TYPE_CHAT)
    {
        const gchar *convo_name = purple_conversation_get_name(convo);
        FListChannel *fchannel = flist_channel_find(fla, convo_name);
        channel = fchannel->name;
    }
    else if (convo->type == PURPLE_CONV_TYPE_IM)
    {
        channel = purple_conversation_get_name(convo);
    }
    else
    {
        *error = g_strdup("You can use /report only in channels and private conversations!");
        return PURPLE_CMD_RET_FAILED;
    }

    FListReport *flr = g_new0(FListReport, 1);
    flr->fla = fla;
    flr->channel = g_strdup(channel);
    flr->convo = convo;

    // Set up report UI
    PurpleRequestFields *fields;
    PurpleRequestFieldGroup *group;
    PurpleRequestField *field;

    group = purple_request_field_group_new("Options");
    fields = purple_request_fields_new();
    purple_request_fields_add_group(fields, group);

    field = purple_request_field_string_new("reason", "What's your problem? Be brief.", "", TRUE);
    purple_request_field_set_required(field, TRUE);
    purple_request_field_group_add_field(group, field);

    field = purple_request_field_string_new("character", "Reported character", character, FALSE);
    purple_request_field_set_required(field, TRUE);
    purple_request_field_group_add_field(group, field);
    
    purple_request_fields(fla->pc, _("Alert Staff"), _("Before you alert the moderators, PLEASE READ:"), _("If you're just having personal trouble with someone, right-click their name and ignore them. Moderators enforce the rules of the chat. If what you're reporting is not a violation of the rules, NOBODY WILL DO A THING."),
        fields,
        _("OK"), G_CALLBACK(flist_report_cb),
        _("Cancel"), G_CALLBACK(flist_report_cancel_cb),
        fla->pa, NULL, NULL, flr);

    return PURPLE_CMD_RET_OK;
}
