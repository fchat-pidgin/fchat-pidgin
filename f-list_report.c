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

void flist_report_alert_staff(FListReport *flr) {
    gchar *report_message = g_strdup_printf("Current Tab/Channel: %s | Reporting User: %s | %s", flr->channel_pretty, flr->fla->proper_character, flr->reason);

    // log uploaded succesfully, let's alert staff now.
    JsonObject *json = json_object_new();
    json_object_set_string_member(json, "action", "report");
    json_object_set_string_member(json, "report", report_message);
    json_object_set_string_member(json, "character", flr->character);

    // Append log_id if we uploaded a log for ths report
    if (flr->log_id)
        json_object_set_string_member(json, "logid", flr->log_id);

    flist_request(flr->fla->pc, FLIST_ALERT_STAFF, json);
    json_object_unref(json);
    g_free(report_message);

    purple_debug_info(FLIST_DEBUG, "Staff alert sent!\n");

    gchar *message;
    if (flr->log_id)
        message = g_strdup_printf("Your report against '%s' has been sent (ID %s), staff has been alerted.", flr->character, flr->log_id);
    else
        message = g_strdup_printf("Your report against '%s' has been sent, Staff has been alerted.", flr->character);

    purple_conversation_write(flr->convo, NULL, message, PURPLE_MESSAGE_SYSTEM, time(NULL));
    g_free(message);
}

static void flist_report_upload_log_cb(FListWebRequestData* req_data, gpointer user_data,
        JsonObject *root, const gchar *error_message) {

    FListReport *flr = (FListReport*)user_data;

    if (!root) {
         purple_debug_warning(FLIST_DEBUG, "Log upload failed! Error Message: %s\n", error_message);
         flist_report_free(flr);
         return;
    }

    const gchar *error = json_object_get_string_member(root, "error");
    if (error && strlen(error))
    {
         purple_debug_warning(FLIST_DEBUG, "Log upload failed! Server error Message: %s\n", error_message);
         flist_report_free(flr);
         return;
    }

    const gchar *log_id = json_object_get_string_member(root, "log_id");
    if (log_id && strlen(log_id))
    {
        purple_debug_info(FLIST_DEBUG, "Log upload succesful (id: %s), now alerting staff.\n", log_id);
        flr->log_id = g_strdup(log_id);
        flist_report_alert_staff(flr);
        flist_report_free(user_data);
        return;
    }

    // This should only happen with a broken server implmenetation
    purple_debug_warning(FLIST_DEBUG, "Log upload failed, server did not respond with a log id!\n");
    flist_report_free(flr);
}

void flist_report_send(FListReport *flr) {
    GList *logs, *current_log;
    PurpleLogType log_type = flr->convo->type == PURPLE_CONV_TYPE_CHAT ? PURPLE_LOG_CHAT : PURPLE_LOG_IM;

    // Fetch a chronologically list of logs belonging to the conversation
    logs = purple_log_get_logs(log_type, flr->channel_handle, flr->fla->pa);

    if (!purple_conversation_is_logging(flr->convo) || g_list_length(logs) == 0)
    {
        purple_notify_info(purple_conversation_get_gc(flr->convo), "No logs found!", "Could not find logs for this conversation. Make sure you have logging enabled before using /report!", NULL);
        g_list_free(logs);
        flist_report_free(flr);
        return;
    }

    // The newest (current) log is always the first one
    current_log = g_list_first(logs);

    // Read its contents and strip HTML tags
    gchar *log_text = purple_log_read(current_log->data, 0);
    flr->log_text = purple_markup_strip_html(log_text);
    g_free(log_text);

    purple_debug_info(FLIST_DEBUG, "User filed a report against '%s': '%s'\n------------- LOG -------------\n%s\n-----------------------------\n", flr->character, flr->reason, flr->log_text);

    // Fire web request to upload our log
    GHashTable *args = flist_web_request_args(flr->fla);
    g_hash_table_insert(args, "character", g_strdup(flr->fla->proper_character));
    g_hash_table_insert(args, "log", g_strdup(flr->log_text));
    g_hash_table_insert(args, "reportText", g_strdup(flr->reason));
    g_hash_table_insert(args, "reportUser", g_strdup(flr->character));
    g_hash_table_insert(args, "channel", g_strdup(flr->channel_pretty));
    flist_web_request(JSON_UPLOAD_LOG, args, flr->fla->cookies, TRUE, flr->fla->secure, flist_report_upload_log_cb, flr);

    g_hash_table_destroy(args);
    g_list_free(logs);

}

static void flist_report_ui_ok_cb(gpointer user_data, PurpleRequestFields *fields) {
    FListReport *flr = user_data;
    flr->reason = g_strdup(purple_request_fields_get_string(fields, "reason"));
    flr->character = g_strdup(purple_request_fields_get_string(fields, "character"));

    flist_report_send(flr);
}

static void flist_report_ui_cancel_cb(gpointer user_data) {
    FListReport *flr = user_data;
    flist_report_free(flr);
}

void flist_report_free(FListReport *flr)
{
    g_free(flr->channel_pretty);
    g_free(flr->channel_handle);
    g_free(flr->character);
    g_free(flr->reason);
    g_free(flr->log_text);
    g_free(flr->log_id);
    g_free(flr);
}

FListReport *flist_report_new(FListAccount *fla, PurpleConversation *convo, const gchar *reported_character, const gchar* reason) {
    FListReport *flr = g_new0(FListReport, 1);
    flr->fla = fla;
    flr->convo = convo;
    flr->character = g_strdup(reported_character);
    flr->reason = g_strdup(reason);

    // Conversation is a channel
    if (convo->type == PURPLE_CONV_TYPE_CHAT)
    {
        const gchar *convo_name = purple_conversation_get_name(convo);
        FListChannel *fchannel = flist_channel_find(fla, convo_name);
        flr->channel_handle = g_strdup(fchannel->name);

        // Only add the internal handle if there is one and it is different from the name
        if (fchannel->title && g_strcmp0(fchannel->title, fchannel->name) != 0)
            flr->channel_pretty = g_strdup_printf("%s (%s)", fchannel->title, fchannel->name);
        else
            flr->channel_pretty = g_strdup(fchannel->name);
    }
    // Conversastion is a private message
    else if (convo->type == PURPLE_CONV_TYPE_IM)
    {
        flr->channel_handle = g_strdup(purple_conversation_get_name(convo));
        flr->channel_pretty = g_strdup(flr->channel_handle);
    }

    return flr;
}

void flist_report_display_ui(FListReport *flr) {
    // Set up report UI
    PurpleRequestFields *fields;
    PurpleRequestFieldGroup *group;
    PurpleRequestField *field;

    group = purple_request_field_group_new("Options");
    fields = purple_request_fields_new();
    purple_request_fields_add_group(fields, group);

    field = purple_request_field_string_new("reason", "What's your problem? Be brief.", flr->reason, TRUE);
    purple_request_field_set_required(field, TRUE);
    purple_request_field_group_add_field(group, field);

    field = purple_request_field_string_new("character", "Reported character", flr->character, FALSE);
    purple_request_field_set_required(field, TRUE);
    purple_request_field_group_add_field(group, field);
    
    purple_request_fields(flr->fla->pc, _("Alert Staff"), _("Before you alert the moderators, PLEASE READ:"), _("If you're just having personal trouble with someone, right-click their name and ignore them. Moderators enforce the rules of the chat. If what you're reporting is not a violation of the rules, NOBODY WILL DO A THING."),
        fields,
        _("OK"), G_CALLBACK(flist_report_ui_ok_cb),
        _("Cancel"), G_CALLBACK(flist_report_ui_cancel_cb),
        flr->fla->pa, NULL, NULL, flr);
}

PurpleCmdRet flist_report_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;

    if (convo->type != PURPLE_CONV_TYPE_CHAT && convo->type != PURPLE_CONV_TYPE_IM)
    {
        *error = g_strdup("You can use /report only in channels and private conversations!");
        return PURPLE_CMD_RET_FAILED;
    }

    gchar *character = NULL, *reason = NULL;

    gchar **split = g_strsplit(args[0], ",", 2);
    guint count = g_strv_length(split);

    // We got character and reason
    if (count == 2)
    {
        character = split[0];
        reason = g_strchug(split[1]);
    }
    // We only got characte
    else if (count == 1)
    {
        character = split[0];
    }
    // We got something we did not expect
    else if (count >= 3)
    {
        g_strfreev(split);
        *error = g_strdup(_("Either use /report &lt;character&rt;,&lt;reason&rt; to directly send report or use /report &lt;character&r or just /report to open a dialog."));
        return PURPLE_CMD_RET_FAILED;
    }

    FListReport *flr = flist_report_new(fla, convo, character, reason);

    // When both character and reason are set we don't need to display the UI
    if (character && reason)
    {
        flist_report_send(flr);
    }
    else
    {
        flist_report_display_ui(flr);
    }

    g_strfreev(split);
    return PURPLE_CMD_RET_OK;
}
