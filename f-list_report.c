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
    gchar *report_message = g_strdup_printf("Current Tab/Channel: %s | Reporting User: %s | %s", flr->channel_pretty, flr->character, flr->reason);

    // log uploaded succesfully, let's alert staff now.
    JsonObject *json = json_object_new();
    json_object_set_string_member(json, "action", "report");
    json_object_set_string_member(json, "report", report_message);
    json_object_set_string_member(json, "character", flr->character);
    json_object_set_string_member(json, "tab", flr->channel_handle);

    // Append logid if we uploaded a log for ths report
    if (flr->log_id)
        json_object_set_string_member(json, "logid", flr->log_id);

    flist_request(flr->fla, FLIST_ALERT_STAFF, json);
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
        purple_debug_info(FLIST_DEBUG, "Log upload successful (id: %s), now alerting staff.\n", log_id);
        flr->log_id = g_strdup(log_id);
        flist_report_alert_staff(flr);
        flist_report_free(flr);
        return;
    }

    // This should only happen with a broken server implmenetation
    purple_debug_warning(FLIST_DEBUG, "Log upload failed, server did not respond with a log id!\n");
    flist_report_free(flr);
}

// TODO Passing a struct hides the required inputs of this function
// Change it to return a gchar* and take everything it needs (convo, channel_handle, ...) as parameters
void flist_report_fetch_text(FListReport *flr) {
    GList *logs, *current_log;
    PurpleLogType log_type = flr->convo->type == PURPLE_CONV_TYPE_CHAT ? PURPLE_LOG_CHAT : PURPLE_LOG_IM;

    logs = purple_log_get_logs(log_type, flr->channel_handle, flr->fla->pa);
    current_log = g_list_nth(logs, flr->start_point);

    GString *report_text = g_string_new(NULL);
    for (gint i = flr->start_point; i >= 0; i--)
    {
        PurpleLog *log = (PurpleLog*)current_log->data;
        g_string_append(report_text, purple_log_read(log, 0));

        current_log = current_log->prev;
    }

    gchar *raw_text = g_string_free(report_text, FALSE);
    // Read its contents and strip HTML tags
    gchar *stripped_text;
    purple_markup_html_to_xhtml(raw_text, NULL, &stripped_text);

    // And finally, escape HTML entities
    flr->log_text = purple_markup_escape_text(stripped_text, -1);
    g_free(stripped_text);

    g_list_free(logs);
}

void flist_report_send(FListReport *flr) {
    purple_debug_info(FLIST_DEBUG, "User filed a report against '%s': '%s'\n------------- LOG -------------\n%s\n-----------------------------\n", flr->character, flr->reason, flr->log_text);

    // Fire web request to upload our log
    GHashTable *args = flist_web_request_args(flr->fla);
    g_hash_table_insert(args, "character", g_strdup(flr->fla->proper_character));
    g_hash_table_insert(args, "log", g_strdup(flr->log_text));
    g_hash_table_insert(args, "reportText", g_strdup(flr->reason));
    g_hash_table_insert(args, "reportUser", g_strdup(flr->character));
    g_hash_table_insert(args, "channel", g_strdup(flr->channel_pretty));
    flist_web_request(JSON_UPLOAD_LOG, args, NULL, TRUE, flist_report_upload_log_cb, flr);

    g_hash_table_unref(args);
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
    flr->start_point = 0;
    flr->display_preview = FALSE;

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
    // Conversation is a private message
    else if (convo->type == PURPLE_CONV_TYPE_IM)
    {
        flr->channel_handle = g_strdup(purple_conversation_get_name(convo));
        flr->channel_pretty = g_strdup(flr->channel_handle);
    }

    return flr;
}

static void flist_report_ui_preview_ok_cb(gpointer user_data, PurpleRequestFields *fields) {
    FListReport *flr = user_data;
    flist_report_send(flr);
}

static void flist_report_ui_ok_cb(gpointer user_data, PurpleRequestFields *fields) {
    FListReport *flr = user_data;
    flr->reason = g_strdup(purple_request_fields_get_string(fields, "reason"));
    flr->character = g_strdup(purple_request_fields_get_string(fields, "character"));

    if (purple_request_fields_exists(fields, "start_point"))
        flr->start_point = purple_request_fields_get_choice(fields, "start_point");

    flist_report_fetch_text(flr);

    flr->display_preview = purple_request_fields_get_bool(fields, "preview");
    if (flr->display_preview)
    {
        gchar *description = g_strdup_printf("Reporting User: %s\nReason: %s", flr->character, flr->reason);
        purple_request_input(flr->fla->pa,                                          /* handle */
                             "Preview Report",                                      /* title */
                             "You can preview what will be sent to the server once you file your report.\nChanging the text does NOT change the report.", /* primary message */
                             description,                                           /* secondary message */
                             flr->log_text,                                         /* default value */
                             TRUE,                                                  /* multiline */
                             FALSE,                                                 /* masked input (e.g. passwords) */
                             "",                                                    /* hint */
                             "Send",                                                /* ok text */
                             PURPLE_CALLBACK(flist_report_ui_preview_ok_cb),        /* ok callback */
                             "Cancel",                                              /* cancel text */
                             PURPLE_CALLBACK(flist_report_display_ui),              /* cancel callback */
                             flr->fla->pa,                                          /* account */
                             NULL,                                                  /* associated buddy */
                             flr->convo,                                            /* associated conversation */
                             flr                                                    /* user data */
                            );

        g_free(description);
    }
    else
        flist_report_send(flr);
}

static void flist_report_ui_cancel_cb(gpointer user_data) {
    FListReport *flr = user_data;
    flist_report_free(flr);
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

    if (purple_conversation_is_logging(flr->convo))
    {
        field = purple_request_field_choice_new("start_point", "Select from which point in time on you want to send logs", flr->start_point);

        // Add the past 5 logs to the list
        GList *logs;
        PurpleLogType log_type = flr->convo->type == PURPLE_CONV_TYPE_CHAT ? PURPLE_LOG_CHAT : PURPLE_LOG_IM;
        logs = purple_log_get_logs(log_type, flr->channel_handle, flr->fla->pa);

        guint32 log_num = 0;
        if (g_list_length(logs) > 0)
        {
            // Use up to 6 logs, because at 6 entries, the choice widget turns into a dropdown field
            // Yay, Pidgin
            while (logs && log_num < 6)
            {
                PurpleLog *log = (PurpleLog*)logs->data;
                const gchar *datetime = purple_utf8_strftime("%d.%m.%Y %H:%M:%S", localtime(&log->time));

                if (log_num == 0)
                {
                    gboolean has_current_conversation = g_list_length(purple_conversation_get_message_history(flr->convo)) > 0;

                    gchar *label = g_strdup_printf("%s (%s conversation)", datetime, has_current_conversation ? "Current" : "Last");
                    purple_request_field_choice_add(field, label);
                    g_free(label);
                }
                else
                    purple_request_field_choice_add(field, datetime);

                log_num++;
                logs = logs->next;
            }
        }
        purple_request_field_group_add_field(group, field);

        if (log_num == 0)
        {
            purple_request_fields_destroy(fields);
            purple_notify_warning(flr->fla->pc, "Report Error", "No logs found!", "The current conversation is empty and there are no logs. Are you sure you want to report this chat?");
            return;
        }
    }

    field = purple_request_field_bool_new("preview", "Display preview of report", flr->display_preview);
    purple_request_field_set_required(field, TRUE);
    purple_request_field_group_add_field(group, field);
    
    purple_request_fields(flr->fla->pc, _("Alert Staff"), _("Before you alert the moderators, PLEASE READ:"), _("If you're just having personal trouble with someone, right-click their name and ignore them. Moderators enforce the rules of the chat. If what you're reporting is not a violation of the rules, NOBODY WILL DO A THING."),
        fields,
        _("OK"), G_CALLBACK(flist_report_ui_ok_cb),
        _("Cancel"), G_CALLBACK(flist_report_ui_cancel_cb),
        flr->fla->pa, NULL, NULL, flr);
}

PurpleCmdRet flist_report_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    FListAccount *fla = flist_get_account_from_conversation(convo);
    g_return_val_if_fail(fla, PURPLE_CMD_RET_FAILED);

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
