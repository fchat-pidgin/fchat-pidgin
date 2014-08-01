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
#include "f-list_commands.h"

static gboolean is_empty_status(FListStatus status) {
    return status == FLIST_STATUS_AVAILABLE || status == FLIST_STATUS_OFFLINE;
}
void flist_update_friend(PurpleConnection *pc, const gchar *name, gboolean update_icon, gboolean new_buddy) {
    PurpleAccount *pa = purple_connection_get_account(pc);
    FListAccount *fla = pc->proto_data;
    FListCharacter *character = flist_get_character(fla, name);
    if(!new_buddy && !purple_find_buddy(pa, name)) return;

    if(character) {
        purple_prpl_got_user_status(pa, name, flist_internal_status(character->status),
                FLIST_STATUS_MESSAGE_KEY, character->status_message, NULL);
    } else {
        purple_prpl_got_user_status(pa, name, "offline", NULL);
    }
    if(update_icon) {
        flist_fetch_icon(fla, name);
    }
}

GList *flist_buddy_menu(PurpleBuddy *b) {
    PurpleAccount *pa = purple_buddy_get_account(b);
    PurpleConnection *pc;
    FListAccount *fla;
    const gchar *name = purple_buddy_get_name(b);
    gboolean bookmarked;
    FListFriendStatus friend_status;
    GList *ret = NULL;
    PurpleMenuAction *act;
    
    g_return_val_if_fail((pc = purple_account_get_connection(pa)), NULL);
    g_return_val_if_fail((fla = pc->proto_data), NULL);
    
    friend_status = flist_friends_get_friend_status(fla, name);
    bookmarked = flist_friends_is_bookmarked(fla, name);
    
    if(bookmarked) {
        act = purple_menu_action_new("Remove Bookmark", PURPLE_CALLBACK(flist_blist_node_action), GINT_TO_POINTER(FLIST_BOOKMARK_REMOVE), NULL);
        ret = g_list_prepend(ret, act);
    } else {
        act = purple_menu_action_new("Add Bookmark", PURPLE_CALLBACK(flist_blist_node_action), GINT_TO_POINTER(FLIST_BOOKMARK_ADD), NULL);
        ret = g_list_prepend(ret, act);
    }
    
    switch(friend_status) {
    case FLIST_MUTUAL_FRIEND:
        act = purple_menu_action_new("Remove Friend", PURPLE_CALLBACK(flist_blist_node_action), GINT_TO_POINTER(FLIST_FRIEND_REMOVE), NULL);
        ret = g_list_prepend(ret, act);
        break;
    case FLIST_NOT_FRIEND:
        act = purple_menu_action_new("Make Friend Request", PURPLE_CALLBACK(flist_blist_node_action), GINT_TO_POINTER(FLIST_FRIEND_REQUEST), NULL);
        ret = g_list_prepend(ret, act);
        break;
    case FLIST_PENDING_OUT_FRIEND:
        act = purple_menu_action_new("Cancel Friend Request", PURPLE_CALLBACK(flist_blist_node_action), GINT_TO_POINTER(FLIST_FRIEND_CANCEL), NULL);
        ret = g_list_prepend(ret, act);
        break;
    case FLIST_PENDING_IN_FRIEND:
        act = purple_menu_action_new("Deny Friend Request", PURPLE_CALLBACK(flist_blist_node_action), GINT_TO_POINTER(FLIST_FRIEND_DENY), NULL);
        ret = g_list_prepend(ret, act);
        act = purple_menu_action_new("Approve Friend Request", PURPLE_CALLBACK(flist_blist_node_action), GINT_TO_POINTER(FLIST_FRIEND_AUTHORIZE), NULL);
        ret = g_list_prepend(ret, act);
        break;
    default: break;
    }
    
    return ret;
}

GList *flist_blist_node_menu(PurpleBlistNode *node) {
    if(PURPLE_BLIST_NODE_IS_BUDDY(node)) {
        return flist_buddy_menu((PurpleBuddy *) node);
    }
    return NULL;
}

void flist_get_tooltip(PurpleBuddy *buddy, PurpleNotifyUserInfo *user_info, gboolean full) {
    PurpleAccount *pa = buddy->account;
    PurpleConnection *pc = purple_account_get_connection(pa);
    FListAccount *fla;
    FListCharacter *character;
    const gchar *identity = flist_normalize(pa, purple_buddy_get_name(buddy));
    FListFriendStatus friend_status;
    gboolean bookmarked;
    
    g_return_if_fail((fla = pc->proto_data));
    character = flist_get_character(fla, identity);
    
    bookmarked = flist_friends_is_bookmarked(fla, identity);
    friend_status = flist_friends_get_friend_status(fla, identity);
    
    if(character) {
        purple_notify_user_info_add_pair(user_info, "Status", flist_format_status(character->status));
        purple_notify_user_info_add_pair(user_info, "Gender", flist_format_gender(character->gender));
        if(strlen(character->status_message)) {
            purple_notify_user_info_add_pair(user_info, "Message", character->status_message);
        }
    }
    
    purple_notify_user_info_add_pair(user_info, "Friend", flist_format_friend_status(friend_status));
    purple_notify_user_info_add_pair(user_info, "Bookmarked", bookmarked ? "Yes" : "No");
}

gchar *flist_get_status_text(PurpleBuddy *buddy) {
    PurpleAccount *pa = buddy->account;
    PurpleConnection *pc = purple_account_get_connection(pa);
    FListAccount *fla;
    FListCharacter *character;
    GString *ret;
    gboolean empty_status = FALSE;
    gboolean empty;
    
    g_return_val_if_fail(pc, NULL);
    g_return_val_if_fail((fla = pc->proto_data), NULL);
    
    character = flist_get_character(fla, flist_normalize(pa, buddy->name));
    if(!character) return NULL; /* user is offline, no problem here */
    
    empty_status = is_empty_status(character->status);
    empty = empty_status && strlen(character->status_message) == 0;
    
    ret = g_string_new(NULL);
    g_string_append_printf(ret, "(%s)%s", flist_format_gender(character->gender), empty ? "" : " ");
    if(!empty_status) {
        g_string_append(ret, flist_format_status(character->status));
    }
    if(strlen(character->status_message) > 0) {
        if(!empty_status) g_string_append(ret, " - ");
        g_string_append(ret, character->status_message);
    }
    return g_string_free(ret, FALSE);
}

void flist_pidgin_add_buddy(PurpleConnection *pc, PurpleBuddy *buddy, PurpleGroup *group) {
    PurpleAccount *pa = buddy->account;
    FListAccount *fla = pc->proto_data;
    const gchar *identity;
    PurpleBuddy *old_buddy;
    GSList *buddies, *cur;
    
    g_return_if_fail(fla);
    
    identity = purple_buddy_get_name(buddy);
    buddies = purple_find_buddies(pa, identity);
    cur = buddies;
    while(cur) {
        old_buddy = cur->data;
        if(old_buddy != buddy) {
            purple_blist_remove_buddy(old_buddy); //no duplicate buddies, please
        }
        cur = g_slist_next(cur);
    }
    g_slist_free(buddies);

    flist_update_friend(pc, identity, TRUE, TRUE);
}

void flist_pidgin_remove_buddy(PurpleConnection *pc, PurpleBuddy *buddy, PurpleGroup *group) {
    FListAccount *fla = pc->proto_data;
    const gchar *identity;
    
    g_return_if_fail(fla);

    identity = purple_buddy_get_name(buddy);
}

PurpleRoomlist *flist_get_roomlist(PurpleConnection *pc) {
    PurpleAccount *pa = purple_connection_get_account(pc);
    FListAccount *fla = pc->proto_data;
    PurpleRoomlistField *f;
    GList *fields = NULL;

    g_return_val_if_fail(fla, NULL);

    if(fla->roomlist) purple_roomlist_unref(fla->roomlist);
    
    fla->roomlist = purple_roomlist_new(pa);
    
    f = purple_roomlist_field_new(PURPLE_ROOMLIST_FIELD_STRING, "", "channel", TRUE);
    fields = g_list_append(fields, f);
    
    f = purple_roomlist_field_new(PURPLE_ROOMLIST_FIELD_STRING, _("Status"), "Status", FALSE);
    fields = g_list_append(fields, f);
    
    f = purple_roomlist_field_new(PURPLE_ROOMLIST_FIELD_INT, _("Users"), "users", FALSE);
    fields = g_list_append(fields, f);
    
    purple_roomlist_set_fields(fla->roomlist, fields);
    
    flist_request(pc, FLIST_REQUEST_CHANNEL_LIST, NULL);
    
    return fla->roomlist;
}

guint flist_send_typing(PurpleConnection *pc, const char *name, PurpleTypingState state) {
    FListAccount *fla = pc->proto_data;
    const gchar *state_string = flist_typing_state_string(state);
    JsonObject *json;

    g_return_val_if_fail(fla, 0);

    json = json_object_new();
    json_object_set_string_member(json, "character", name);
    json_object_set_string_member(json, "status", state_string);
    flist_request(pc, FLIST_NOTIFY_TYPING, json);
    json_object_unref(json);
    
    return 0; //we don't need to send again
}

void flist_input_request_cancel_cb(gpointer user_data, const gchar *name) {
    PurpleConnection *pc = user_data;
    FListAccount *fla = pc->proto_data;
    
    g_return_if_fail(fla);
    
    fla->input_request = FALSE;
}

void flist_create_private_channel_action_cb(gpointer user_data, const gchar *name) {
    PurpleConnection *pc = user_data;
    FListAccount *fla = pc->proto_data;
    JsonObject *json;
    
    g_return_if_fail(fla);

    json = json_object_new();
    
    json_object_set_string_member(json, "channel", name);
    flist_request(pc, "CCR", json);
    json_object_unref(json);
    
    fla->input_request = FALSE;
}

void flist_create_private_channel_action(PurplePluginAction *action) {
    PurpleConnection *pc = action->context;
    PurpleAccount *pa = purple_connection_get_account(pc);
    FListAccount *fla;
    
    g_return_if_fail(pc);
    g_return_if_fail((fla = pc->proto_data));
    if(fla->input_request) return;
    
    purple_request_input(pc, _("Create Private Channel"), _("Create a private channel on the F-List server."),
        _("Please enter a title for your new channel."), "",
        FALSE, FALSE, NULL,
        _("OK"), G_CALLBACK(flist_create_private_channel_action_cb),
        _("Cancel"), G_CALLBACK(flist_input_request_cancel_cb),
        pa, NULL, NULL, pc);
    
    fla->input_request = TRUE;
}


void flist_set_status_action_cb(gpointer user_data, PurpleRequestFields *fields) {
    PurpleConnection *pc = user_data;
    FListAccount *fla = pc->proto_data;
    GSList *status_list = flist_get_status_list();
    FListStatus status;
    const gchar *status_message;

    g_return_if_fail(fla);

    status = flist_parse_status(g_slist_nth_data(status_list, purple_request_fields_get_choice(fields, "status")));
    status_message = purple_request_fields_get_string(fields, "message");
    if(status_message == NULL) status_message = "";

    flist_set_status(fla, status, status_message);
    flist_update_server_status(fla);
    
    fla->input_request = FALSE;
}

static void flist_set_status_dialog(FListAccount *fla) {
    PurpleRequestFields *fields;
    PurpleRequestFieldGroup *group;
    PurpleRequestField *field;
    GSList *status_list;
    int default_value, i;
    FListStatus old_status = flist_get_status(fla);
    
    if(fla->input_request) return;
    
    group = purple_request_field_group_new("Options");
    fields = purple_request_fields_new();
    purple_request_fields_add_group(fields, group);

    default_value = 0; /* we need to find the default value first to work around a Pidgin bug */
    i = 0;
    status_list = flist_get_status_list();
    while(status_list) {
        if(flist_parse_status(status_list->data) == old_status) {
            default_value = i;
        }
        status_list = g_slist_next(status_list);
        i++;
    }

    status_list = flist_get_status_list();
    field = purple_request_field_choice_new("status", "Status", default_value);
    while(status_list) {
        purple_request_field_choice_add(field, flist_format_status(flist_parse_status(status_list->data)));
        status_list = g_slist_next(status_list);
    }
    purple_request_field_group_add_field(group, field);

    field = purple_request_field_string_new("message", "Status Message", flist_get_status_message(fla), FALSE);
    purple_request_field_group_add_field(group, field);
    
    purple_request_fields(fla->pc, _("Set Status"), _("Set your status on the F-List server."), _("Select your status and enter your status message."),
        fields,
        _("OK"), G_CALLBACK(flist_set_status_action_cb),
        _("Cancel"), G_CALLBACK(flist_input_request_cancel_cb),
        fla->pa, NULL, NULL, fla->pc);     
    
    fla->input_request = TRUE;
    
}

void flist_set_status_action(PurplePluginAction *action) {
    PurpleConnection *pc = action->context;
    FListAccount *fla;

    g_return_if_fail(pc);
    g_return_if_fail((fla = pc->proto_data));
    
    flist_set_status_dialog(fla);
}

static guint64 flist_now() {
    GTimeVal time;
    g_get_current_time(&time);
    return (((guint64) time.tv_sec) * G_USEC_PER_SEC) + time.tv_usec;
}
void flist_join_channel(PurpleConnection *pc, GHashTable *components) {
    PurpleAccount *pa = purple_connection_get_account(pc);
    FListAccount *fla = pc->proto_data;
    const gchar *channel;
    JsonObject *json;
    PurpleConversation *convo;
    guint64 *last_ptr, last, now;

    g_return_if_fail(fla);
    
    channel = g_hash_table_lookup(components, CHANNEL_COMPONENTS_NAME);
    g_return_if_fail(channel);

    /* Pidgin can sometimes try to automatically join the channel twice at the same time. */
    /* Don't let this happen! */
    last_ptr = g_hash_table_lookup(fla->chat_timestamp, channel);
    last = last_ptr ? *last_ptr : 0;
    now = flist_now();
    if(last <= now && now - last < CHANNEL_JOIN_TIMEOUT) return;

    convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, channel, pa);
    if(convo && !purple_conv_chat_has_left(PURPLE_CONV_CHAT(convo))) { 
        return; //don't double-join the channel, we don't want the error message
    }

    last_ptr = g_malloc(sizeof(guint64));
    *last_ptr = now;
    g_hash_table_replace(fla->chat_timestamp, g_strdup(channel), last_ptr);

    json = json_object_new();
    json_object_set_string_member(json, "channel", channel);
    flist_request(pc, FLIST_CHANNEL_JOIN, json);
    json_object_unref(json);
}

void flist_leave_channel(PurpleConnection *pc, int id) {
    FListAccount *fla = pc->proto_data;
    const gchar *channel;
    PurpleConversation *convo = purple_find_chat(pc, id);
    JsonObject *json;
    
    g_return_if_fail(fla);

    if (!convo)
        return;
    
    json = json_object_new();
    channel = purple_conversation_get_name(convo);
    json_object_set_string_member(json, "channel", channel);
    flist_request(pc, FLIST_CHANNEL_LEAVE, json);
    json_object_unref(json);

    flist_got_channel_left(fla, channel);
    serv_got_chat_left(pc, id);
}

void flist_cancel_roomlist(PurpleRoomlist *list) {
    PurpleConnection *pc = purple_account_get_connection(list->account);
    FListAccount *fla;

    g_return_if_fail(pc);
    g_return_if_fail((fla = pc->proto_data));

    purple_roomlist_set_in_progress(list, FALSE);
    
    if (fla->roomlist == list) {
        fla->roomlist = NULL;
        purple_roomlist_unref(list);
    }
}

//Hopefully, this will never be used.
#define PURPLE_MESSAGE_FLIST 0x04000000
int flist_send_message(PurpleConnection *pc, const gchar *who, const gchar *message, PurpleMessageFlags flags) {
    FListAccount *fla = pc->proto_data;
    JsonObject *json;
    PurpleConvIm *im;
    gchar *stripped_message, *escaped_message, *local_message, *bbcode_message;
    int ret;

    g_return_val_if_fail(fla, 0);

    purple_debug_info(FLIST_DEBUG, "Sending message... (From: %s) (To: %s) (Message: %s) (Flags: %x)\n",
        fla->character, who, message, flags);
    
    im = PURPLE_CONV_IM(purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, who, fla->pa));

    stripped_message = purple_markup_strip_html(message); /* strip out formatting */
    escaped_message = purple_unescape_html(stripped_message); /* escape the html entities that are left */
    local_message = purple_markup_escape_text(stripped_message, -1); /* re-escape the html entities */
    bbcode_message = flist_bbcode_to_html(fla, purple_conv_im_get_conversation(im), local_message); /* convert the bbcode to html to display locally */

    json = json_object_new();
    json_object_set_string_member(json, "recipient", who);
    json_object_set_string_member(json, "message", escaped_message);
    flist_request(pc, FLIST_REQUEST_PRIVATE_MESSAGE, json);
    json_object_unref(json);

    if(im) { /* This should always be the case. */
        purple_conv_im_write(im, NULL, bbcode_message, flags, time(NULL));
        ret = 0; //we've already displayed it
    } else {
        purple_debug_warning("flist", "Sent message, but convo not found. (From: %s) (To: %s)\n",
            fla->character, who);
        ret = 1; //display it now
    }
    
    g_free(stripped_message);
    g_free(escaped_message);
    g_free(bbcode_message);
    g_free(local_message);

    return ret;
}

/* This function is essentially copied from libpurple. */
static gchar *flist_fix_newlines(const char *html) {
    GString *ret;
    const char *c = html;
    if (html == NULL)
        return NULL;
    
    ret = g_string_new("");
    while (*c) {
        if (*c == '\n') {
            g_string_append(ret, "<br>");
            c++;
        } else if (*c == '\r') {
            c++;
        } else {
            g_string_append_c(ret, *c);
            c++;
        }
    }

    return g_string_free(ret, FALSE);
}

static void flist_send_channel_message_real(FListAccount *fla, PurpleConversation *convo, const gchar *message, gboolean ad) {
    JsonObject *json = json_object_new();
    gchar *stripped_message, *escaped_message, *local_message, *bbcode_message;
    const gchar *channel = purple_conversation_get_name(convo);
    purple_debug_info("flist", "Sending message to channel... (Character: %s) (Channel: %s) (Message: %s) (Ad: %s)\n", 
        fla->character, channel, message, ad ? "yes" : "no");
    
    stripped_message = purple_markup_strip_html(message); /* strip out formatting */
    escaped_message = purple_unescape_html(stripped_message); /* unescape the html entities that are left */
    local_message = purple_markup_escape_text(stripped_message, -1); /* re-escape the html entities */
    if(ad) {
        gchar *tmp = g_strdup_printf("[b](Roleplay Ad)[/b] %s", local_message);
        g_free(local_message);
        local_message = tmp;
    }
    bbcode_message = flist_bbcode_to_html(fla, convo, local_message); /* convert the bbcode to html to display locally */
    channel = purple_conversation_get_name(convo);
    json_object_set_string_member(json, "message", escaped_message);
    json_object_set_string_member(json, "channel", channel);
    flist_request(fla->pc, !ad ? FLIST_REQUEST_CHANNEL_MESSAGE : FLIST_CHANNEL_ADVERSTISEMENT, json);
    
    serv_got_chat_in(fla->pc, purple_conv_chat_get_id(PURPLE_CONV_CHAT(convo)), fla->proper_character, PURPLE_MESSAGE_SEND, bbcode_message, time(NULL));

    g_free(escaped_message);
    g_free(stripped_message);
    g_free(bbcode_message);
    g_free(local_message);
}

int flist_send_channel_message(PurpleConnection *pc, int id, const char *message, PurpleMessageFlags flags) {
    FListAccount *fla = pc->proto_data;
    PurpleConversation *convo = purple_find_chat(pc, id);

    g_return_val_if_fail((fla = pc->proto_data), -EINVAL);

    if (!convo) {
        purple_debug(PURPLE_DEBUG_ERROR, "flist", "We want to send a message in channel %d, but we are not in this channel.\n", id);
        return -EINVAL;
    }
    
    flist_send_channel_message_real(fla, convo, message, FALSE);
    return 0;
}

PurpleCmdRet flist_roll_bottle(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    JsonObject *json = json_object_new();
    const gchar *channel = purple_conversation_get_name(convo);
    json_object_set_string_member(json, "channel", channel);
    json_object_set_string_member(json, "dice", "bottle");
    flist_request(pc, FLIST_ROLL_DICE, json);
    json_object_unref(json);
    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_roll_dice(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    JsonObject *json = json_object_new();
    const gchar *channel = purple_conversation_get_name(convo);
    json_object_set_string_member(json, "channel", channel);
    json_object_set_string_member(json, "dice", args[0]);
    flist_request(pc, FLIST_ROLL_DICE, json);
    json_object_unref(json);
    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_priv_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    PurpleAccount *pa = purple_connection_get_account(pc);
    const gchar *character = args[0];
    PurpleConversation *rconvo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, character, pa);
    if(!rconvo) {
        rconvo = purple_conversation_new(PURPLE_CONV_TYPE_IM, pa, character);
    }
    if(rconvo) {
        purple_conversation_present(rconvo);
    }
    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_channel_show_ads_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    const gchar *channel = purple_conversation_get_name(convo);
    flist_set_channel_show_ads(fla, channel, TRUE);
    flist_channel_show_message(fla, channel);
    return PURPLE_CMD_STATUS_OK;
}
PurpleCmdRet flist_channel_hide_ads_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    const gchar *channel = purple_conversation_get_name(convo);
    flist_set_channel_show_ads(fla, channel, FALSE);
    flist_channel_show_message(fla, channel);
    return PURPLE_CMD_STATUS_OK;
}
PurpleCmdRet flist_channel_show_chat_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    const gchar *channel = purple_conversation_get_name(convo);
    flist_set_channel_show_chat(fla, channel, TRUE);
    flist_channel_show_message(fla, channel);
    return PURPLE_CMD_STATUS_OK;
}
PurpleCmdRet flist_channel_hide_chat_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    const gchar *channel = purple_conversation_get_name(convo);
    flist_set_channel_show_chat(fla, channel, FALSE);
    flist_channel_show_message(fla, channel);
    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_channel_send_ad(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    const gchar *message = args[0];
    gchar *fixed_message = flist_fix_newlines(message);
    
    flist_send_channel_message_real(fla, convo, fixed_message, TRUE);
    
    g_free(fixed_message);
    return PURPLE_CMD_STATUS_OK;
}
 
PurpleCmdRet flist_channel_warning(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    const gchar *message = args[0];
    gchar *fixed_message = flist_fix_newlines(message);
    gchar *extended_message = g_strdup_printf("/warn %s", fixed_message);
    
    flist_send_channel_message_real(fla, convo, extended_message, FALSE);
 
    g_free(fixed_message);
    g_free(extended_message);
    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_get_profile_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    const gchar *character = args[0];
    flist_get_profile(pc, character);

    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_channels_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    PurpleAccount *pa = purple_connection_get_account(pc);

    purple_roomlist_show_with_account(pa);

    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_status_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    
    flist_set_status_dialog(fla);
    
    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_whoami_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    gchar *message1, *message2;
    FListStatus status = flist_get_status(fla);
    const gchar *status_message = flist_get_status_message(fla);
    
    message1 = g_strdup_printf("You are %s.", fla->proper_character);
    if(status_message && strlen(status_message)) {
        message2 = g_strdup_printf("Your status is set to %s, with message: %s", flist_format_status(status), status_message);
    } else {
        message2 = g_strdup_printf("Your status is set to %s, with no message.", flist_format_status(status));
    }
    
    purple_conversation_write(convo, NULL, message1, PURPLE_MESSAGE_SYSTEM, time(NULL));
    purple_conversation_write(convo, NULL, message2, PURPLE_MESSAGE_SYSTEM, time(NULL));
    
    g_free(message1);
    g_free(message2);
    
    return PURPLE_CMD_STATUS_OK;
}

void flist_init_commands() {
    PurpleCmdFlag channel_flags = PURPLE_CMD_FLAG_PRPL_ONLY | PURPLE_CMD_FLAG_CHAT;
    PurpleCmdFlag anywhere_flags = PURPLE_CMD_FLAG_PRPL_ONLY | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_IM;

    purple_cmd_register("channels", "", PURPLE_CMD_P_PRPL, anywhere_flags,
        FLIST_PLUGIN_ID, flist_channels_cmd, "channels: Opens the list of channels.", NULL);

    purple_cmd_register("status", "", PURPLE_CMD_P_PRPL, anywhere_flags,
        FLIST_PLUGIN_ID, flist_status_cmd, "status: Opens a dialog box to set your status.", NULL);
    
    purple_cmd_register("whoami", "", PURPLE_CMD_P_PRPL, anywhere_flags,
        FLIST_PLUGIN_ID, flist_whoami_cmd, "whoami: Displays which character you are using.", NULL);
    
    purple_cmd_register("open", "", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_open_cmd, "open: Opens the current private channel.", NULL);
    purple_cmd_register("openroom", "", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_open_cmd, "openroom: Opens the current private channel.", NULL);
    
    purple_cmd_register("close", "", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_close_cmd, "close: Closes the current private channel.", NULL);
    purple_cmd_register("closeroom", "", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_close_cmd, "closeroom: Closes the current private channel.", NULL);

    purple_cmd_register("profile", "s", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_get_profile_cmd, "profile &lt;character&gt;: Shows a user's profile.", NULL);

    purple_cmd_register("invite", "s", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_invite_cmd, "invite &lt;character&gt;: Invites a user to the current channel.", NULL);

    purple_cmd_register("roll", "s", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_roll_dice, "roll &lt;dice&gt;: Rolls dice.", NULL);
    purple_cmd_register("bottle", "", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_roll_bottle, "bottle: Spins a bottle.", NULL);
    
    purple_cmd_register("warn", "s", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_warning, "warn &lt;message&gt;: Sends a warning.", NULL);
    purple_cmd_register("ad", "s", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_send_ad, "ad &lt;message&gt;: Sends a \"looking for role play\" ad.", NULL);
    purple_cmd_register("showads", "", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_show_ads_cmd, "showads: Show roleplay ads in this channel.", NULL);
    purple_cmd_register("hideads", "", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_hide_ads_cmd, "hideads: Hide roleplay ads in this channel.", NULL);
    purple_cmd_register("showchat", "", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_show_chat_cmd, "showchat: Show chat in this channel.", NULL);
    purple_cmd_register("hidechat", "", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_hide_chat_cmd, "hidechat: Hide chat in this channel.", NULL);    
    
    purple_cmd_register("priv", "s", PURPLE_CMD_P_PRPL, anywhere_flags,
        FLIST_PLUGIN_ID, flist_priv_cmd, "priv &lt;character&gt;: Opens a private conversation.", NULL);
    purple_cmd_register("join", "s", PURPLE_CMD_P_PRPL, anywhere_flags,
        FLIST_PLUGIN_ID, flist_channel_join_cmd, "join &lt;channel&gt;: Joins the given channel.", NULL);
    purple_cmd_register("makeroom", "s", PURPLE_CMD_P_PRPL, anywhere_flags,
        FLIST_PLUGIN_ID, flist_channel_make_cmd, "makeroom &lt;channel&gt;: Creates a private channel.", NULL);

    purple_cmd_register("setdescription", "s", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_set_topic_cmd, "setdescription &lt;message&gt: Sets the topic for the current channel.", NULL);
    purple_cmd_register("getdescription", "", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_show_raw_topic_cmd, "getdescripton: Shows the unparsed topic for the current channel.", NULL);
    purple_cmd_register("showdescription", "", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_show_topic_cmd, "getdescripton: Shows the topic for the current channel.", NULL);

    purple_cmd_register("code", "", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_code_cmd, "code: Shows the BBCode to advertise the channel.", NULL);
    purple_cmd_register("coplist", "", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_oplist_cmd, "coplist: Lists the channel operators of the current channel.", NULL);
    purple_cmd_register("cop", "s", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_op_deop_cmd, "cop &lt;character&gt: Adds an operator to the current channel.", NULL);
    purple_cmd_register("cdeop", "s", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_op_deop_cmd, "cdeop &lt;character&gt: Removes an operator from the current channel.", NULL);

    purple_cmd_register("search", "", PURPLE_CMD_P_PRPL, anywhere_flags,
        FLIST_PLUGIN_ID, flist_filter_cmd, "search: Opens the character search form.", NULL);

    purple_cmd_register("who", "", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_who_cmd, "who: Lists the users in the current channel.", NULL);

    purple_cmd_register("kick", "s", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_kick_ban_unban_cmd, "kick &lt;character&gt;: Kicks a user from the channel.", NULL);
    purple_cmd_register("ban", "s", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_kick_ban_unban_cmd, "ban &lt;character&gt;: Bans a user from the channel.", NULL);
    purple_cmd_register("unban", "s", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_kick_ban_unban_cmd, "unban &lt;character&gt;: Unbans a user from the channel.", NULL);

    purple_cmd_register("banlist", "", PURPLE_CMD_P_PRPL, channel_flags,
        FLIST_PLUGIN_ID, flist_channel_banlist_cmd, "banlist: Displays the banlist for the channel.", NULL);

    purple_cmd_register("gkick", "s", PURPLE_CMD_P_PRPL, anywhere_flags,
        FLIST_PLUGIN_ID, flist_global_kick_ban_unban_cmd, "gkick &lt;character&gt;: Kicks a user from the server.", NULL);
    purple_cmd_register("accountban", "s", PURPLE_CMD_P_PRPL, anywhere_flags,
        FLIST_PLUGIN_ID, flist_global_kick_ban_unban_cmd, "accountban &lt;character&gt;: Bans a user's account from the server.", NULL);
    purple_cmd_register("ipban", "s", PURPLE_CMD_P_PRPL, anywhere_flags,
        FLIST_PLUGIN_ID, flist_global_kick_ban_unban_cmd, "ipban &lt;character&gt;: Bans a user's IP from the server.", NULL);
    purple_cmd_register("gunban", "s", PURPLE_CMD_P_PRPL, anywhere_flags,
        FLIST_PLUGIN_ID, flist_global_kick_ban_unban_cmd, "unban &lt;character&gt;: Unbans a user from the server.", NULL);

    purple_cmd_register("timeout", "s", PURPLE_CMD_P_PRPL, anywhere_flags,
        FLIST_PLUGIN_ID, flist_timeout_cmd, "timeout &lt;character&gt;, &lt;time&gt; &lt;reason&gt;: Temporarily bans a user from the server.", NULL);

    purple_cmd_register("reward", "s", PURPLE_CMD_P_PRPL, anywhere_flags,
        FLIST_PLUGIN_ID, flist_reward_cmd, "reward &lt;character&gt;: Sets a user's status to &quot;crown&quot;.", NULL);

    purple_cmd_register("createchannel", "s", PURPLE_CMD_P_PRPL, anywhere_flags,
        FLIST_PLUGIN_ID, flist_create_kill_channel_cmd, "createchannel &lt;channel&gt;: Creates a public channel.", NULL);
    purple_cmd_register("killchannel", "s", PURPLE_CMD_P_PRPL, anywhere_flags,
        FLIST_PLUGIN_ID, flist_create_kill_channel_cmd, "killchannel &lt;channel&gt;: Deletes a public channel.", NULL);

    purple_cmd_register("op", "s", PURPLE_CMD_P_PRPL, anywhere_flags,
        FLIST_PLUGIN_ID, flist_admin_op_deop_cmd, "op &lt;character&gt;: Adds a global operator.", NULL);
    purple_cmd_register("deop", "s", PURPLE_CMD_P_PRPL, anywhere_flags,
        FLIST_PLUGIN_ID, flist_admin_op_deop_cmd, "deop &lt;character&gt;: Removes a global operator.", NULL);

    purple_cmd_register("broadcast", "s", PURPLE_CMD_P_PRPL, anywhere_flags,
        FLIST_PLUGIN_ID, flist_broadcast_cmd, "broadcast &lt;message&gt;: Sends a broadcast to the server.", NULL);
}
