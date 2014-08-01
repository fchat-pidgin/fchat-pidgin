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
#include "f-list_admin.h"

static void flist_admin_request_cancel_string_cb(gpointer user_data, const gchar *name) {
    PurpleConnection *pc = user_data;
    FListAccount *fla = pc->proto_data;

    g_return_if_fail(fla);

    fla->input_request = FALSE;
}

static void flist_create_public_channel_cb(gpointer user_data, const gchar *name) {
    PurpleConnection *pc = user_data;
    FListAccount *fla = pc->proto_data;
    JsonObject *json;

    g_return_if_fail(fla);

    json = json_object_new();
    json_object_set_string_member(json, "channel", name);
    flist_request(pc, FLIST_PUBLIC_CHANNEL_CREATE, json);
    json_object_unref(json);

    fla->input_request = FALSE;
}

void flist_create_public_channel_action(PurplePluginAction *action) {
    PurpleConnection *pc = action->context;
    PurpleAccount *pa = purple_connection_get_account(pc);
    FListAccount *fla = pc->proto_data;
    g_return_if_fail(fla);
    if(fla->input_request) return;

    purple_request_input(pc, _("Create Public Channel"), _("Create a public channel on the F-List server."),
        _("Please enter a title for your new channel."), "",
        FALSE, FALSE, NULL,
        _("OK"), G_CALLBACK(flist_create_public_channel_cb),
        _("Cancel"), G_CALLBACK(flist_admin_request_cancel_string_cb),
        pa, NULL, NULL, pc);
    fla->input_request = TRUE;
}

static void flist_delete_public_channel_cb(gpointer user_data, const gchar *name) {
    PurpleConnection *pc = user_data;
    FListAccount *fla = pc->proto_data;
    JsonObject *json;

    g_return_if_fail(fla);
    json = json_object_new();
    json_object_set_string_member(json, "channel", name);
    flist_request(pc, FLIST_PUBLIC_CHANNEL_DELETE, json);
    json_object_unref(json);

    fla->input_request = FALSE;
}

void flist_delete_public_channel_action(PurplePluginAction *action) {
    PurpleConnection *pc = action->context;
    PurpleAccount *pa = purple_connection_get_account(pc);
    FListAccount *fla = pc->proto_data;
    g_return_if_fail(fla);
    if(fla->input_request) return;

    purple_request_input(pc, _("Delete Public Channel"), _("Delete a public channel from the F-List server."),
        _("Please enter the title of the channel to delete."), "",
        FALSE, FALSE, NULL,
        _("OK"), G_CALLBACK(flist_delete_public_channel_cb),
        _("Cancel"), G_CALLBACK(flist_admin_request_cancel_string_cb),
        pa, NULL, NULL, pc); //TODO: give a list of public channels instead!
    fla->input_request = TRUE;
}

static void flist_add_global_operator_cb(gpointer user_data, const gchar *name) {
    PurpleConnection *pc = user_data;
    FListAccount *fla = pc->proto_data;
    JsonObject *json;

    g_return_if_fail(fla);

    json = json_object_new();
    json_object_set_string_member(json, "character", name);
    flist_request(pc, FLIST_ADD_GLOBAL_OPERATOR, json);
    json_object_unref(json);

    fla->input_request = FALSE;
}

void flist_add_global_operator_action(PurplePluginAction *action) {
    PurpleConnection *pc = action->context;
    PurpleAccount *pa = purple_connection_get_account(pc);
    FListAccount *fla = pc->proto_data;
    g_return_if_fail(fla);
    if(fla->input_request) return;

    purple_request_input(pc, _("Add Global Operator"), _("Add a global operator to the F-List server."),
        _("Please enter the name of the character to promote."), "",
        FALSE, FALSE, NULL,
        _("OK"), G_CALLBACK(flist_add_global_operator_cb),
        _("Cancel"), G_CALLBACK(flist_admin_request_cancel_string_cb),
        pa, NULL, NULL, pc);
    fla->input_request = TRUE;
}

static void flist_remove_global_operator_cb(gpointer user_data, const gchar *name) {
    PurpleConnection *pc = user_data;
    FListAccount *fla = pc->proto_data;
    JsonObject *json;

    g_return_if_fail(fla);

    json = json_object_new();
    json_object_set_string_member(json, "character", name);
    flist_request(pc, FLIST_REMOVE_GLOBAL_OPERATOR, json);
    json_object_unref(json);

    fla->input_request = FALSE;
}

void flist_remove_global_operator_action(PurplePluginAction *action) {
    PurpleConnection *pc = action->context;
    PurpleAccount *pa = purple_connection_get_account(pc);
    FListAccount *fla = pc->proto_data;
    g_return_if_fail(fla);
    if(fla->input_request) return;

    purple_request_input(pc, _("Remove Global Operator"), _("Remove a global operator from the F-List server."),
        _("Please enter the name of the character to demote."), "",
        FALSE, FALSE, NULL,
        _("OK"), G_CALLBACK(flist_remove_global_operator_cb),
        _("Cancel"), G_CALLBACK(flist_admin_request_cancel_string_cb),
        pa, NULL, NULL, pc); //TODO: give a list of global operators instead!
    fla->input_request = TRUE;
}

static void flist_broadcast_action_cb(gpointer user_data, const gchar *message) {
    PurpleConnection *pc = user_data;
    FListAccount *fla = pc->proto_data;
    JsonObject *json;

    g_return_if_fail(fla);

    json = json_object_new();
    json_object_set_string_member(json, "message", message);
    flist_request(pc, FLIST_BROADCAST, json);
    json_object_unref(json);

    fla->input_request = FALSE;
}

void flist_broadcast_action(PurplePluginAction *action) {
    PurpleConnection *pc = action->context;
    PurpleAccount *pa = purple_connection_get_account(pc);
    FListAccount *fla = pc->proto_data;
    g_return_if_fail(fla);
    if(fla->input_request) return;

    purple_request_input(pc, _("Broadcast"), _("Send a broadcast to the entire F-List server."),
        _("Please enter the message you want to broadcast."), "",
        FALSE, FALSE, NULL,
        _("OK"), G_CALLBACK(flist_broadcast_action_cb),
        _("Cancel"), G_CALLBACK(flist_admin_request_cancel_string_cb),
        pa, NULL, NULL, pc);
    fla->input_request = TRUE;
}

PurpleCmdRet flist_admin_op_deop_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    const gchar *character, *code;
    JsonObject *json;
    FListFlags flags;

    flags = flist_get_flags(fla, NULL, fla->proper_character);
    if(!(flags & FLIST_FLAG_ADMIN)) {
        *error = g_strdup(_("You must be an administrator to add or remove global operators."));
        return PURPLE_CMD_STATUS_FAILED;
    }

    if(!purple_utf8_strcasecmp(cmd, "op")) code = FLIST_CHANNEL_BAN;
    if(!purple_utf8_strcasecmp(cmd, "deop")) code = FLIST_CHANNEL_UNBAN;
    if(!code) return PURPLE_CMD_STATUS_NOT_FOUND;

    character = args[0];

    json = json_object_new();
    json_object_set_string_member(json, "character", character);
    flist_request(pc, code, json);
    json_object_unref(json);

    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_global_kick_ban_unban_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    const gchar *character, *code;
    JsonObject *json;
    FListFlags flags;

    flags = flist_get_flags(fla, NULL, fla->proper_character);
    if(!(flags & (FLIST_FLAG_ADMIN | FLIST_FLAG_GLOBAL_OP))) {
        *error = g_strdup(_("You must be a global operator to globally kick, ban, or unban."));
        return PURPLE_CMD_STATUS_FAILED;
    }

    if(!purple_utf8_strcasecmp(cmd, "gkick")) code = FLIST_GLOBAL_KICK;
    if(!purple_utf8_strcasecmp(cmd, "ipban")) code = FLIST_GLOBAL_IP_BAN;
    if(!purple_utf8_strcasecmp(cmd, "accountban")) code = FLIST_GLOBAL_ACCOUNT_BAN;
    if(!purple_utf8_strcasecmp(cmd, "gunban")) code = FLIST_GLOBAL_UNBAN;
    if(!code) return PURPLE_CMD_STATUS_NOT_FOUND;

    character = args[0];

    json = json_object_new();
    json_object_set_string_member(json, "character", character);
    flist_request(pc, code, json);
    json_object_unref(json);

    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_create_kill_channel_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    const gchar *channel, *code;
    JsonObject *json;
    FListFlags flags;

    flags = flist_get_flags(fla, NULL, fla->proper_character);
    if(!(flags & (FLIST_FLAG_ADMIN | FLIST_FLAG_GLOBAL_OP))) {
        *error = g_strdup(_("You must be a global operator to create or delete public channels."));
        return PURPLE_CMD_STATUS_FAILED;
    }

    if(!purple_utf8_strcasecmp(cmd, "createchannel")) code = FLIST_PUBLIC_CHANNEL_CREATE;
    if(!purple_utf8_strcasecmp(cmd, "killchannel")) code = FLIST_PUBLIC_CHANNEL_DELETE;
    if(!code) return PURPLE_CMD_STATUS_NOT_FOUND;

    channel = args[0];

    json = json_object_new();
    json_object_set_string_member(json, "channel", channel);
    flist_request(pc, code, json);
    json_object_unref(json);

    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_broadcast_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    const gchar *message;
    JsonObject *json;
    FListFlags flags;

    flags = flist_get_flags(fla, NULL, fla->proper_character);
    if(!(flags & (FLIST_FLAG_ADMIN))) {
        *error = g_strdup(_("You must be an administrator to send a global broadcast."));
        return PURPLE_CMD_STATUS_FAILED;
    }

    message = args[0];

    json = json_object_new();
    json_object_set_string_member(json, "message", message);
    flist_request(pc, FLIST_BROADCAST, json);
    json_object_unref(json);

    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_timeout_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    gchar **split; guint count;
    const gchar *character, *time, *reason;
    gulong time_parsed; gchar *endptr;
    JsonObject *json;
    FListFlags flags;

    flags = flist_get_flags(fla, NULL, fla->proper_character);
    if(!(flags & (FLIST_FLAG_ADMIN | FLIST_FLAG_GLOBAL_OP))) {
        *error = g_strdup(_("You must be a global operator to timeban."));
        return PURPLE_CMD_STATUS_FAILED;
    }

    split = g_strsplit(args[0], ",", 3);
    count = g_strv_length(split);

    if(count < 3) {
        g_strfreev(split);
        *error = g_strdup(_("You must enter a character, a time, and a reason."));
        return PURPLE_CMD_STATUS_WRONG_ARGS;
    }

    character = split[0];
    time = g_strchug(split[1]);
    reason = g_strchug(split[2]);

    time_parsed = strtoul(time, &endptr, 10);
    if(time_parsed == 0 || endptr != time + strlen(time)) {
        g_strfreev(split);
        *error = g_strdup(_("You must enter a valid length of time."));
        return PURPLE_CMD_STATUS_WRONG_ARGS;
    }
    
    json = json_object_new();
    json_object_set_string_member(json, "character", character);
    json_object_set_string_member(json, "reason", reason);
    json_object_set_int_member(json, "time", time_parsed);
    flist_request(pc, FLIST_GLOBAL_TIMEOUT, json);
    json_object_unref(json);
    g_strfreev(split);
    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_reward_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    const gchar *character;
    JsonObject *json;
    FListFlags flags;

    flags = flist_get_flags(fla, NULL, fla->proper_character);
    if(!(flags & (FLIST_FLAG_ADMIN | FLIST_FLAG_GLOBAL_OP))) {
        *error = g_strdup(_("You must be a global operator to reward a user."));
        return PURPLE_CMD_STATUS_FAILED;
    }

    character = args[0];

    json = json_object_new();
    json_object_set_string_member(json, "character", character);
    flist_request(pc, FLIST_REWARD, json);
    json_object_unref(json);

    return PURPLE_CMD_STATUS_OK;
}

void flist_send_sfc_confirm(PurpleConnection *pc, const gchar *callid) {
    FListAccount *fla = pc->proto_data;
    JsonObject *json;

    json = json_object_new();
    json_object_set_string_member(json, "action", "confirm");
    json_object_set_string_member(json, "moderator", fla->proper_character);
    json_object_set_string_member(json, "callid", callid);
    flist_request(pc, "SFC", json);
    json_object_unref(json);
}

static void flist_sfc_report(PurpleConnection *pc, JsonObject *root) {
    PurpleAccount *pa = purple_connection_get_account(pc);
    const gchar *callid, *reporter, *report;
    gchar *s, *escaped_reporter, *escaped_report, *message;
    GString *message_str;
    gdouble timestamp;
    gint logid; gboolean has_logid;

    callid = json_object_get_string_member(root, "callid");
    reporter = json_object_get_string_member(root, "character");
    report = json_object_get_string_member(root, "report");
    logid = json_object_get_parse_int_member(root, "logid", &has_logid);
    timestamp = json_object_get_double_member(root, "timestamp");

    g_return_if_fail(callid);
    g_return_if_fail(reporter);
    g_return_if_fail(report);

    message_str = g_string_new(NULL);
    s = g_strdup(purple_url_encode(flist_serialize_account(pa)));
    escaped_reporter = purple_markup_escape_text(reporter, -1);
    escaped_report = purple_markup_escape_text(report, -1);

    g_string_append_printf(message_str, "Moderator Alert. %s writes:\n", escaped_reporter);
    g_string_append_printf(message_str, "%s\n", escaped_report);
    g_string_append_printf(message_str, "<a href=\"flistsfc://%s/%s\">Confirm Alert</a>", s, purple_url_encode(callid));
    g_string_append(message_str, ", ");
    if(has_logid) {
        g_string_append_printf(message_str, "<a href=\"http://www.f-list.net/fchat/getLog.php?log=%d\">View Log</a>", logid);
    } else {
        g_string_append_printf(message_str, "(No Log)");
    }

    message = g_string_free(message_str, FALSE);
    serv_got_im(pc, GLOBAL_NAME, message, PURPLE_MESSAGE_RECV, time(NULL));

    g_free(escaped_report);
    g_free(escaped_reporter);
    g_free(message);
    g_free(s);
}

static void flist_sfc_confirm(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    const gchar *moderator, *reporter;
    gchar *message, *escaped_message, *bbcode_message;
    gdouble timestamp;

    moderator = json_object_get_string_member(root, "moderator");
    reporter = json_object_get_string_member(root, "character");
    timestamp = json_object_get_double_member(root, "timestamp");

    g_return_if_fail(moderator);
    g_return_if_fail(reporter);

    message = g_strdup_printf("Alert Confirmed. [b]%s[/b] is handling [b]%s[/b]'s report.", moderator, reporter);
    escaped_message = purple_markup_escape_text(message, -1);
    bbcode_message = flist_bbcode_to_html(fla, NULL, escaped_message);
    serv_got_im(pc, GLOBAL_NAME, bbcode_message, PURPLE_MESSAGE_RECV, time(NULL));
    g_free(bbcode_message);
    g_free(escaped_message);
    g_free(message);
}

gboolean flist_process_SFC(PurpleConnection *pc, JsonObject *root) {
    const gchar *action;

    action = json_object_get_string_member(root, "action");
    g_return_val_if_fail(action, TRUE);

    if(flist_str_equal(action, "report")) {
        flist_sfc_report(pc, root);
    }

    if(flist_str_equal(action, "confirm")) {
        flist_sfc_confirm(pc, root);
    }

    return TRUE;
}
