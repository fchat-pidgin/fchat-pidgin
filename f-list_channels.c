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
#include "f-list_channels.h"
#define CHAT_SHOW_DISPLAY_STATUS "CHAT_SHOW_DISPLAY_STATUS"

GCompareFunc flist_op_strcmp = (GCompareFunc) flist_strcmp;

FListFlags flist_get_flags(FListAccount *fla, const gchar *channel, const gchar *identity) {
    FListFlags ret = 0;
    FListChannel *fchannel = channel ? flist_channel_find(fla, channel) : NULL;
    
    if(fchannel && fchannel->owner && !flist_op_strcmp(fchannel->owner, identity)) {
        ret |= FLIST_FLAG_CHANNEL_FOUNDER;
    }
    if(fchannel && g_list_find_custom(fchannel->operators, identity, flist_op_strcmp)) {
        ret |= FLIST_FLAG_CHANNEL_OP;
    }
    if(g_hash_table_lookup(fla->global_ops, identity) != NULL) {
        ret |= FLIST_FLAG_GLOBAL_OP;
        ret |= FLIST_FLAG_ADMIN; /* there is currently no way to tell */
    }
    return ret;
}

static PurpleConvChatBuddyFlags flist_flags_lookup(FListAccount *fla, PurpleConversation *convo, const gchar *identity) {
    const gchar *channel = purple_conversation_get_name(convo);
    FListChannel *fchannel = flist_channel_find(fla, channel);
    
    if(!fchannel) {
        purple_debug_error("flist", "Flags requested for %s in channel %s, but no channel was found.\n", identity, channel);
        return PURPLE_CBFLAGS_NONE;
    }
    if(fchannel->owner && !flist_op_strcmp(fchannel->owner, identity)) {
        return PURPLE_CBFLAGS_FOUNDER;
    }
    if(g_hash_table_lookup(fla->global_ops, identity) != NULL) {
        return PURPLE_CBFLAGS_OP;
    }
    if(g_list_find_custom(fchannel->operators, identity, flist_op_strcmp)) {
        return PURPLE_CBFLAGS_HALFOP;
    }
    
    return PURPLE_CBFLAGS_NONE;
}

void flist_update_user_chats_offline(PurpleConnection *pc, const gchar *character) {
    FListAccount *fla = pc->proto_data;
    PurpleAccount *pa = fla->pa;
    FListChannel *fchannel;
    GHashTableIter iter;
    g_hash_table_iter_init(&iter, fla->chat_table);

    while(g_hash_table_iter_next(&iter, NULL, (gpointer*)&fchannel)) {
        gchar *original_character;
        if(g_hash_table_lookup_extended(fchannel->users, character, (void**) &original_character, NULL)) {
            PurpleConversation *convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, fchannel->name, pa);
            if(convo) purple_conv_chat_remove_user(PURPLE_CONV_CHAT(convo), original_character, "offline");
            g_hash_table_remove(fchannel->users, character);
        }
    }
}

void flist_update_user_chats_rank(PurpleConnection *pc, const gchar *character) {
    FListAccount *fla = pc->proto_data;
    PurpleAccount *pa = fla->pa;
    FListChannel *channel;
    GHashTableIter iter;
    g_hash_table_iter_init(&iter, fla->chat_table);

    while(g_hash_table_iter_next(&iter, NULL, (gpointer*)&channel)) {
        if(g_hash_table_lookup_extended(channel->users, character, NULL, NULL)) {
            PurpleConversation *convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, channel->name, pa);
            PurpleConvChatBuddyFlags flags = flist_flags_lookup(fla, convo, character);
            purple_conv_chat_user_set_flags(PURPLE_CONV_CHAT(convo), character, flags);
        }
    }
}

void flist_update_users_chats_rank(PurpleConnection *pc, GList *users) {
    GList *cur = users;
    while(cur) {
        flist_update_user_chats_rank(pc, cur->data);
        cur = g_list_next(cur);
    }
}

void flist_got_channel_title(FListAccount *fla, const gchar *channel, const gchar *title) {
    PurpleConversation *convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, channel, fla->pa);
    FListChannel *fchannel = flist_channel_find(fla, channel);
    PurpleGroup *g = flist_get_chat_group(fla);
    PurpleChat *b;
    
    g_return_if_fail(title != NULL);
    g_return_if_fail(convo != NULL);
    g_return_if_fail(fchannel != NULL);

    b = flist_get_chat(fla, channel);
    if(b && purple_chat_get_group(b) == g) {
        purple_blist_alias_chat(b, title);
    }

    purple_conversation_autoset_title(convo);
    
    if(fchannel->title) g_free(fchannel->title);
    fchannel->title = g_strdup(title);
}

static void flist_show_channel_topic(FListAccount *fla, const gchar *channel) {
    PurpleConversation *convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, channel, fla->pa);
    FListChannel *fchannel = flist_channel_find(fla, channel);
    gchar *escaped_description, *html_description;
    
    g_return_if_fail(convo != NULL);
    g_return_if_fail(fchannel != NULL);
    g_return_if_fail(fchannel->topic != NULL); //TODO: better error handling than not showing anything.
    
    escaped_description = purple_markup_escape_text(fchannel->topic, -1);
    html_description = flist_bbcode_to_html(fla, convo, escaped_description);
    
    //message = g_strdup_printf("The description for %s is: %s", purple_conversation_get_title(convo), html_description);
    purple_conv_chat_write(PURPLE_CONV_CHAT(convo), "", html_description, PURPLE_MESSAGE_SYSTEM, time(NULL));
    
    g_free(escaped_description);
    g_free(html_description);
}

void flist_got_channel_topic(FListAccount *fla, const gchar *channel, const gchar *topic) {
    PurpleConversation *convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, channel, fla->pa);
    FListChannel *fchannel = flist_channel_find(fla, channel);
    gchar *escaped_description, *stripped_description, *stripped_description_2, *unescaped_description;
    
    g_return_if_fail(topic != NULL);
    g_return_if_fail(convo != NULL);
    g_return_if_fail(fchannel != NULL);
    
    if(fchannel->topic) g_free(fchannel->topic);
    fchannel->topic = g_strdup(topic);
    
    escaped_description = purple_markup_escape_text(fchannel->topic, -1);
    stripped_description = flist_bbcode_strip(escaped_description);
    stripped_description_2 = flist_strip_crlf(stripped_description);
    unescaped_description = purple_unescape_html(stripped_description_2);
    
    purple_conv_chat_set_topic(PURPLE_CONV_CHAT(convo), NULL, unescaped_description);
    flist_show_channel_topic(fla, channel);
    
    if(purple_conversation_get_data(convo, CHAT_SHOW_DISPLAY_STATUS)) {
        purple_conversation_set_data(convo, CHAT_SHOW_DISPLAY_STATUS, GINT_TO_POINTER(FALSE));
        flist_channel_show_message(fla, channel);
    }
    
    g_free(escaped_description);
    g_free(stripped_description);
    g_free(stripped_description_2);
    g_free(unescaped_description);
}

void flist_got_channel_userlist(FListAccount *fla, const gchar *channel, GList *userlist) {
    PurpleConversation *convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, channel, fla->pa);
    FListChannel *fchannel = flist_channel_find(fla, channel);
    GList *cur;
    GList *flags = NULL;
    
    g_return_if_fail(convo != NULL);
    g_return_if_fail(fchannel != NULL);
    
    for(cur = userlist; cur; cur = cur->next) {
        flags = g_list_prepend(flags, GINT_TO_POINTER(flist_flags_lookup(fla, convo, cur->data)));
        g_hash_table_replace(fchannel->users, g_strdup(cur->data), NULL);
    }
    flags = g_list_reverse(flags);
    
    purple_conv_chat_add_users(PURPLE_CONV_CHAT(convo), userlist, NULL, flags, FALSE);
}

void flist_got_channel_user_joined(FListAccount *fla, const gchar *channel, const gchar* character) {
    PurpleConversation *convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, channel, fla->pa);
    FListChannel *fchannel = flist_channel_find(fla, channel);
    PurpleConvChatBuddyFlags flags;

    g_return_if_fail(character != NULL);
    g_return_if_fail(fchannel != NULL);
    g_return_if_fail(convo != NULL);
    
    g_hash_table_replace(fchannel->users, g_strdup(character), NULL);
    
    flags = flist_flags_lookup(fla, convo, character);
    purple_conv_chat_add_user(PURPLE_CONV_CHAT(convo), character, NULL, flags, TRUE);
}

void flist_got_channel_user_left(FListAccount *fla, const gchar *channel, const gchar* character, const gchar* message) {
    PurpleConversation *convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, channel, fla->pa);
    FListChannel *fchannel = flist_channel_find(fla, channel);
    gchar *original_character;
    
    g_return_if_fail(character != NULL);
    g_return_if_fail(fchannel != NULL);
    g_return_if_fail(convo != NULL);
    
    if(g_hash_table_lookup_extended(fchannel->users, character, (void**) &original_character, NULL)) {
        purple_conv_chat_remove_user(PURPLE_CONV_CHAT(convo), original_character, message ? message : "left");
        g_hash_table_remove(fchannel->users, character);
    }
}

void flist_got_channel_joined(FListAccount *fla, const gchar *name) {
    FListChannel *fchannel = g_new0(FListChannel, 1);
    
    fchannel->name = g_strdup(name);
    fchannel->users = g_hash_table_new_full((GHashFunc) flist_str_hash, (GEqualFunc) flist_str_equal, g_free, NULL);
    fchannel->mode = CHANNEL_MODE_BOTH;
    g_hash_table_replace(fla->chat_table, g_strdup(name), fchannel);
    purple_debug(PURPLE_DEBUG_INFO, "flist", "We (%s) have joined channel %s.\n", fla->proper_character, name);
}

void flist_got_channel_left(FListAccount *fla, const gchar *name) {
    flist_remove_chat(fla, name);
    g_hash_table_remove(fla->chat_table, name);
    purple_debug(PURPLE_DEBUG_INFO, "flist", "We (%s) have left channel %s.\n", fla->proper_character, name);
}

void flist_got_channel_mode(FListAccount *fla, const gchar *channel, const gchar *mode) {
    PurpleConversation *convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, channel, fla->pa);
    FListChannel *fchannel = flist_channel_find(fla, channel);
    FListChannelMode fmode;
    
    g_return_if_fail(fchannel != NULL);
    g_return_if_fail(convo != NULL);
    
    fmode = flist_parse_channel_mode(mode);
    
}

void flist_got_channel_oplist(FListAccount *fla, const gchar *channel, GList *ops) {
    PurpleConversation *convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, channel, fla->pa);
    FListChannel *fchannel = flist_channel_find(fla, channel);
    GList *old_ops, *new_ops;
    gchar *old_owner;
    
    g_return_if_fail(fchannel != NULL);
    g_return_if_fail(convo != NULL);
    
    old_ops = fchannel->operators;
    old_owner = fchannel->owner;
    new_ops = NULL;
    
    for(; ops; ops = ops->next) {
        gchar *identity = g_strdup(ops->data);
        new_ops = g_list_prepend(new_ops, identity);
    }
    new_ops = g_list_reverse(new_ops);
    
    fchannel->operators = new_ops;
    fchannel->owner = new_ops ? g_strdup(new_ops->data) : NULL;
    
    flist_update_users_chats_rank(fla->pc, old_ops);
    flist_update_users_chats_rank(fla->pc, fchannel->operators);
    
    if(old_owner) g_free(old_owner);
    if(old_ops) flist_g_list_free_full(old_ops, g_free);
}

/*
static void flist_got_channel_promote(PurpleConnection *pc, PurpleConversation *convo, const gchar *who) {
    FListAccount *fla = pc->proto_data;
    const gchar *name = purple_conversation_get_name(convo);
    GHashTable *ops = g_hash_table_lookup(fla->chat_ops, name);
    gchar *identity = g_strdup(who);

    if(!ops) {
        ops = flist_chat_ops_new();
        g_hash_table_insert(fla->chat_ops, g_strdup(name), ops);
    }

    g_hash_table_insert(ops, identity, identity);
    flist_update_user_chats_rank(pc, who);
}

static void flist_got_channel_demote(PurpleConnection *pc, PurpleConversation *convo, const gchar *who) {
    FListAccount *fla = pc->proto_data;
    const gchar *name = purple_conversation_get_name(convo);
    GHashTable *ops = g_hash_table_lookup(fla->chat_ops, name);

    if(!ops) return; //this should never happen

    g_hash_table_remove(ops, who);
    flist_update_user_chats_rank(pc, who);
}
*/

gboolean flist_process_CIU(PurpleConnection *pc, JsonObject *root) {
    const gchar *sender, *name, *title;
    GHashTable *data;
    
    sender = json_object_get_string_member(root, "sender");
    name = json_object_get_string_member(root, "name");
    title = json_object_get_string_member(root, "title");
    g_return_val_if_fail(sender, TRUE);
    g_return_val_if_fail(name, TRUE);
    
    if(!title) title = name;
    
    data = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);
    g_hash_table_insert(data, CHANNEL_COMPONENTS_NAME, g_strdup(name));
    
    serv_got_chat_invite(pc, title, sender, NULL, data);
    return TRUE;
}

gboolean flist_process_JCH(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    PurpleConversation *convo;
    JsonObject *character;
    const gchar *identity;
    const gchar *channel;
    const gchar *title;
    static int id = 1;

    channel = json_object_get_string_member(root, "channel");
    character = json_object_get_object_member(root, "character");
    identity = json_object_get_string_member(character, "identity");
    title = json_object_get_string_member(root, "title");

    if(!purple_utf8_strcasecmp(identity, fla->proper_character)) { //we just joined a channel
        convo = serv_got_joined_chat(pc, id++, channel);
        flist_got_channel_joined(fla, channel);
        purple_conv_chat_set_nick(PURPLE_CONV_CHAT(convo), fla->proper_character);
        purple_conversation_set_data(convo, CHAT_SHOW_DISPLAY_STATUS, GINT_TO_POINTER(TRUE));
    } else {
        flist_got_channel_user_joined(fla, channel, identity);
    }
    
    if(title) {
        gchar *unescaped_title = purple_unescape_html(title);
        flist_got_channel_title(fla, channel, unescaped_title);
        g_free(unescaped_title);
    }

    return TRUE;
}

gboolean flist_process_kickban(PurpleConnection *pc, JsonObject *root, gboolean ban) {
    PurpleAccount *pa = purple_connection_get_account(pc);
    FListAccount *fla = pc->proto_data;
    PurpleConversation *convo;
    const gchar *operator, *character, *channel;
    
    channel = json_object_get_string_member(root, "channel");
    operator = json_object_get_string_member(root, "operator");
    character = json_object_get_string_member(root, "character");
    
    g_return_val_if_fail(channel != NULL, TRUE);
    g_return_val_if_fail(character != NULL, TRUE);
    
    convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, channel, pa);
    if(!convo) {
        purple_debug(PURPLE_DEBUG_ERROR, "flist", "User %s was kicked or banned from channel %s, but we are not in this channel.\n", channel, character);
        return TRUE;
    }
    
    if(!flist_strcmp(character, fla->proper_character)) { //we just got kicked
        gchar *message = operator 
                ? g_strdup_printf("You have been %s from the channel!", ban ? "kicked and banned" : "kicked") 
                : g_strdup_printf("%s has %s you from the channel!", operator, ban ? "kicked and banned" : "kicked");
        purple_conv_chat_write(PURPLE_CONV_CHAT(convo), "System", message, PURPLE_MESSAGE_SYSTEM, time(NULL));
        g_free(message);
        flist_got_channel_left(fla, channel);
        serv_got_chat_left(pc, purple_conv_chat_get_id(PURPLE_CONV_CHAT(convo)));
        return TRUE;
    }
    
    gchar *message = g_strdup_printf("%s%s%s", 
            ban ? "kicked and banned" : "kicked", 
            operator ? " by " : "",
            operator ? operator : "");
    flist_got_channel_user_left(fla, channel, character, message);
    g_free(message);
    return TRUE;
}

gboolean flist_process_CKU(PurpleConnection *pc, JsonObject *root) {
    return flist_process_kickban(pc, root, FALSE);
}
gboolean flist_process_CBU(PurpleConnection *pc, JsonObject *root) {
    return flist_process_kickban(pc, root, TRUE);
}

gboolean flist_process_COL(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    JsonArray *users;
    const gchar *channel;
    GList *ops = NULL;
    int i, len;

    channel = json_object_get_string_member(root, "channel");
    users = json_object_get_array_member(root, "oplist");

    len = json_array_get_length(users);
    for(i = 0; i < len; i++) {
        const gchar *identity = json_array_get_string_element(users, i);
        ops = g_list_prepend(ops, (gpointer)identity);
    }
    ops = g_list_reverse(ops);
    
    flist_got_channel_oplist(fla, channel, ops);
    
    g_list_free(ops);

    return TRUE;
}

gboolean flist_process_RMO(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    const gchar *channel, *mode;
    
    channel = json_object_get_string_member(root, "channel");
    mode = json_object_get_string_member(root, "mode");
    
    g_return_val_if_fail(channel != NULL, TRUE);
    
    if(mode) {
        flist_got_channel_mode(fla, channel, mode);
    }
    
    return TRUE;
}

gboolean flist_process_ICH(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    JsonArray *array;
    const gchar *channel, *title, *mode;
    int i, len;
    GList *users = NULL;

    array = json_object_get_array_member(root, "users");
    channel = json_object_get_string_member(root, "channel");
    title = json_object_get_string_member(root, "title");
    mode = json_object_get_string_member(root, "mode");
    
    g_return_val_if_fail(channel != NULL, TRUE);
    
    if(title) {
        gchar *unescaped_title = purple_unescape_html(title);
        flist_got_channel_title(fla, channel, unescaped_title);
        g_free(unescaped_title);
    }
    
    if(mode) {
        flist_got_channel_mode(fla, channel, mode);
    }
    
    len = json_array_get_length(array);
    for(i = 0; i < len; i++) {
        JsonObject *user_object = json_array_get_object_element(array, i);
        const gchar *identity = json_object_get_string_member(user_object, "identity");
        users = g_list_prepend(users, (gpointer) identity);
    }
    users = g_list_reverse(users);
    
    flist_got_channel_userlist(fla, channel, users);
    
    g_list_free(users);
    
    return TRUE;
}

gboolean flist_process_CDS(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    const gchar *channel;
    const gchar *description;

    channel = json_object_get_string_member(root, "channel");
    description = json_object_get_string_member(root, "description");
    flist_got_channel_topic(fla, channel, description);
    return TRUE;
}

gboolean flist_process_LCH(PurpleConnection *pc, JsonObject *root) {
    PurpleAccount *pa = purple_connection_get_account(pc);
    FListAccount *fla = pc->proto_data;
    PurpleConversation *convo;
    const gchar *character;
    const gchar *channel;

    channel = json_object_get_string_member(root, "channel");
    character = json_object_get_string_member(root, "character");

    convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, channel, pa);
    if(!convo) {
        purple_debug(PURPLE_DEBUG_ERROR, "flist", "User %s left channel %s, but we are not in this channel.\n", character, channel);
        return TRUE;
    }

    if(!purple_utf8_strcasecmp(character, fla->proper_character)) { //we just left a channel
        //TODO: add message that says we left?
        flist_got_channel_left(fla, channel);
        serv_got_chat_left(pc, purple_conv_chat_get_id(PURPLE_CONV_CHAT(convo)));
        return TRUE;
    }

    flist_got_channel_user_left(fla, channel, character, NULL);
    return TRUE;
}

gboolean flist_process_CBL(PurpleConnection *pc, JsonObject *root) {
//    CBL {"banlist": [{"chanop": "TestTiger", "character": "TestPanther", "time": "2011-03-20T11:21:33.154111"}],
//    "channel": "ADH-4268aebc94bd8c5362e1"}
    return TRUE;
}

void flist_channel_print_error(PurpleConversation *convo, const gchar *message) {
    purple_conv_chat_write(PURPLE_CONV_CHAT(convo), NULL, message, PURPLE_MESSAGE_ERROR, time(NULL));
}

static void flist_who_single(FListAccount *fla, PurpleConversation *convo, FListCharacter *character, gboolean icon) {
    GString *message_str = g_string_new("");
    gchar *message, *parsed_message;
    
    if(icon) {
        g_string_append_printf(message_str, "[icon]%s[/icon] ", character->name);
    } else {
        g_string_append_printf(message_str, "[user]%s[/user] ", character->name);
    }
    g_string_append_printf(message_str, "(%s) ", flist_format_gender_color(character->gender));
    if(character->status_message && strlen(character->status_message)) {
        g_string_append_printf(message_str, "%s - %s", flist_format_status(character->status), character->status_message);
    } else {
        g_string_append(message_str, flist_format_status(character->status));
    }
    
    message = g_string_free(message_str, FALSE);
    parsed_message = flist_bbcode_to_html(fla, NULL, message);
    
    purple_conversation_write(convo, NULL, parsed_message, PURPLE_MESSAGE_SYSTEM, time(NULL));

    g_free(message);
    g_free(parsed_message);
}

static gint flist_who_compare(gconstpointer a, gconstpointer b) {
    const FListCharacter *c1 = a, *c2 = b;
    return flist_strcmp(c1->name, c2->name);
}

static void flist_who(FListAccount *fla, PurpleConversation *convo, GList *who, gboolean icons) {
    who = g_list_sort(who, flist_who_compare);
    
    while(who) {
        FListCharacter *character = who->data;
        flist_who_single(fla, convo, character, icons);
        who = who->next;
    }
}

PurpleCmdRet flist_channel_who_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    const gchar *name = purple_conversation_get_name(convo);
    FListChannel *fchannel = flist_channel_find(fla, name);
    GList *chat_buddies, *list = NULL;
    
    g_return_val_if_fail(fchannel != NULL, PURPLE_CMD_STATUS_FAILED);
    
    chat_buddies = purple_conv_chat_get_users(PURPLE_CONV_CHAT(convo));
    while(chat_buddies) {
        PurpleConvChatBuddy *buddy = chat_buddies->data;
        FListCharacter *character = flist_get_character(fla, purple_conv_chat_cb_get_name(buddy));
        
        if(character) {
            list = g_list_prepend(list, character);
        } else {
            purple_debug_warning(FLIST_DEBUG, "Unable to find character to display info. (Target: %s)\n", purple_conv_chat_cb_get_name(buddy));
        }
        chat_buddies = chat_buddies->next;
    }
    list = g_list_reverse(list);
    
    flist_who(fla, convo, list, FALSE);
    
    return PURPLE_CMD_RET_OK;
}

PurpleCmdRet flist_channel_oplist_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    const gchar *name = purple_conversation_get_name(convo);
    FListChannel *fchannel = flist_channel_find(fla, name);
    GString *str;
    gchar *to_print;
    GList *cur;
    
    g_return_val_if_fail(fchannel != NULL, PURPLE_CMD_STATUS_FAILED);
    
    str = g_string_new(NULL);
    if(!fchannel->owner && !fchannel->operators) {
        g_string_append(str, "This channel has no operators.");
    } else {
        gboolean first = TRUE;
        g_string_append(str, "The operators for this channel are: ");
        if(fchannel->owner) {
            first = FALSE;
            g_string_append_printf(str, "%s (Owner)", fchannel->owner);
        }
        for(cur = fchannel->operators; cur; cur = cur->next) {
            const gchar *character = (const gchar *) cur->data;
            if(!fchannel->owner || !flist_str_equal(character, fchannel->owner)) {
                g_string_append_printf(str, "%s%s", !first ? ", " : "", character);
                first = FALSE;
            }
        }
    }

    to_print = g_string_free(str, FALSE);
    purple_conv_chat_write(PURPLE_CONV_CHAT(convo), "", to_print, PURPLE_MESSAGE_SYSTEM, time(NULL));
    g_free(to_print);

    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_channel_op_deop_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    const gchar *channel, *character;
    JsonObject *json;
    const gchar *code = NULL;
    PurpleConvChatBuddyFlags flags;

    channel = purple_conversation_get_name(convo);
    character = args[0];

    flags = flist_flags_lookup(fla, convo, fla->proper_character);
    if(!(flags & (PURPLE_CBFLAGS_OP | PURPLE_CBFLAGS_FOUNDER))) {
        *error = g_strdup(_("You must be the channel owner or a global operator to add or remove channel operators."));
        return PURPLE_CMD_STATUS_FAILED;
    }

    if(!purple_utf8_strcasecmp(cmd, "cop")) code = FLIST_CHANNEL_ADD_OP;
    if(!purple_utf8_strcasecmp(cmd, "cdeop")) code = FLIST_CHANNEL_REMOVE_OP;
    if(!code) return PURPLE_CMD_STATUS_NOT_FOUND;

    json = json_object_new();
    json_object_set_string_member(json, "channel", channel);
    json_object_set_string_member(json, "character", character);
    flist_request(pc, code, json);
    json_object_unref(json);

    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_channel_code_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    const gchar *name, *title;
    gchar *name_escaped, *title_escaped, *message;
    
    name = purple_conversation_get_name(convo);
    title = purple_conversation_get_title(convo);
    name_escaped = purple_markup_escape_text(name, -1);
    title_escaped = purple_markup_escape_text(title, -1);

    message = g_strdup_printf("Copy this text into your post: [session=%s]%s[/session]", title_escaped, name_escaped);
    purple_conv_chat_write(PURPLE_CONV_CHAT(convo), "", message, PURPLE_MESSAGE_SYSTEM, time(NULL));

    g_free(name_escaped);
    g_free(title_escaped);
    g_free(message);
    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_channel_join_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    const gchar *channel = args[0];
    GHashTable* components = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free); 
    g_hash_table_insert(components, CHANNEL_COMPONENTS_NAME, g_strdup(channel));
    serv_join_chat(pc, components);
    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_channel_make_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    const gchar *channel = args[0];
    JsonObject *json;

    json = json_object_new();
    json_object_set_string_member(json, "channel", channel);
    flist_request(pc, FLIST_CHANNEL_CREATE, json);
    json_object_unref(json);

    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_channel_banlist_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    const gchar *channel = purple_conversation_get_name(convo);
    JsonObject *json;
    PurpleConvChatBuddyFlags flags;

    flags = flist_flags_lookup(fla, convo, fla->proper_character);
    if(!(flags & (PURPLE_CBFLAGS_OP | PURPLE_CBFLAGS_HALFOP | PURPLE_CBFLAGS_FOUNDER))) {
        *error = g_strdup(_("You must be a channel operator to view the channel banlist."));
        return PURPLE_CMD_STATUS_FAILED;
    }

    json = json_object_new();
    json_object_set_string_member(json, "channel", channel);
    flist_request(pc, FLIST_CHANNEL_GET_BANLIST, json);
    json_object_unref(json);

    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_channel_open_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    const gchar *channel = purple_conversation_get_name(convo);
    JsonObject *json;
    PurpleConvChatBuddyFlags flags;

    flags = flist_flags_lookup(fla, convo, fla->proper_character);
    if(!(flags & (PURPLE_CBFLAGS_OP | PURPLE_CBFLAGS_FOUNDER))) {
        *error = g_strdup(_("You must be the channel owner or a global operator to open or close a private channel."));
        return PURPLE_CMD_STATUS_FAILED;
    }

    json = json_object_new();
    json_object_set_string_member(json, "channel", channel);
    json_object_set_string_member(json, "status", "public");
    flist_request(pc, FLIST_SET_CHANNEL_STATUS, json);
    json_object_unref(json);

    //TODO: don't allow this on public channels?
    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_channel_close_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    const gchar *channel = purple_conversation_get_name(convo);
    JsonObject *json = json_object_new();
    PurpleConvChatBuddyFlags flags;

    flags = flist_flags_lookup(fla, convo, fla->proper_character);
    if(!(flags & (PURPLE_CBFLAGS_OP | PURPLE_CBFLAGS_FOUNDER))) {
        *error = g_strdup(_("You must be the channel owner or a global operator to open or close a private channel."));
        return PURPLE_CMD_STATUS_FAILED;
    }

    json_object_set_string_member(json, "channel", channel);
    json_object_set_string_member(json, "status", "private");
    flist_request(pc, FLIST_SET_CHANNEL_STATUS, json);

    //TODO: don't allow this on public channels

    json_object_unref(json);
    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_channel_show_topic_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc ? pc->proto_data : NULL;
    const gchar *channel;
    
    g_return_val_if_fail(fla, PURPLE_CMD_STATUS_FAILED);

    channel = purple_conversation_get_name(convo);
    flist_show_channel_topic(fla, channel);

    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_channel_show_raw_topic_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc ? pc->proto_data : NULL;
    const gchar *channel;
    FListChannel *fchannel;
    
    g_return_val_if_fail(fla, PURPLE_CMD_STATUS_FAILED);
    
    channel = purple_conversation_get_name(convo);
    fchannel = flist_channel_find(fla, channel);
    g_return_val_if_fail(fchannel != NULL, PURPLE_CMD_STATUS_FAILED);
    
    if(!fchannel->topic) {
        purple_conv_chat_write(PURPLE_CONV_CHAT(convo), "", "The description for this channel is currently unset.", PURPLE_MESSAGE_SYSTEM, time(NULL));
    } else {
        gchar *escaped_topic = purple_markup_escape_text(fchannel->topic, -1);
        purple_conv_chat_write(PURPLE_CONV_CHAT(convo), "", escaped_topic, PURPLE_MESSAGE_SYSTEM, time(NULL));
        g_free(escaped_topic);
    }
    
    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_channel_set_topic_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    const gchar *channel;
    const gchar *topic;
    JsonObject *json;
    PurpleConvChatBuddyFlags flags;

    flags = flist_flags_lookup(fla, convo, fla->proper_character);
    if(!(flags & (PURPLE_CBFLAGS_OP | PURPLE_CBFLAGS_FOUNDER | PURPLE_CBFLAGS_HALFOP))) {
        *error = g_strdup(_("You must be a channel or global operator to set the channel topic."));
        return PURPLE_CMD_STATUS_FAILED;
    }

    channel = purple_conversation_get_name(convo);
    topic = args[0];

    json = json_object_new();
    json_object_set_string_member(json, "channel", channel);
    json_object_set_string_member(json, "description", topic);
    flist_request(pc, FLIST_SET_CHANNEL_DESCRIPTION, json);
    json_object_unref(json);

    //TODO: don't allow this on public channels? (or do we?)
    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_channel_kick_ban_unban_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;
    const gchar *channel, *character, *code;
    JsonObject *json;
    PurpleConvChatBuddyFlags flags;
    FListCharacter *target;
    gboolean must_be_online = TRUE;

    flags = flist_flags_lookup(fla, convo, fla->proper_character);
    if(!(flags & (PURPLE_CBFLAGS_OP | PURPLE_CBFLAGS_FOUNDER | PURPLE_CBFLAGS_HALFOP))) {
        *error = g_strdup(_("You must be a channel or global operator to kick, ban, or unban."));
        return PURPLE_CMD_STATUS_FAILED;
    }

    if(!purple_utf8_strcasecmp(cmd, "kick")) code = FLIST_CHANNEL_KICK;
    if(!purple_utf8_strcasecmp(cmd, "ban")) code = FLIST_CHANNEL_BAN;
    if(!purple_utf8_strcasecmp(cmd, "unban")) {
        code = FLIST_CHANNEL_UNBAN;
        must_be_online = FALSE;
    }
    if(!code) return PURPLE_CMD_STATUS_NOT_FOUND;

    channel = purple_conversation_get_name(convo);
    character = args[0];

    target = flist_get_character(fla, character);
    if(must_be_online && !target) {
        *error = g_strdup(_("You may only kick or ban users that are online!"));
        return PURPLE_CMD_STATUS_FAILED;
    }

    json = json_object_new();
    json_object_set_string_member(json, "channel", channel);
    json_object_set_string_member(json, "character", character);
    flist_request(pc, code, json);
    json_object_unref(json);

    return PURPLE_CMD_STATUS_OK;
}

PurpleCmdRet flist_channel_invite_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    const gchar *channel, *character;
    JsonObject *json;

    channel = purple_conversation_get_name(convo);
    character = args[0];

    json = json_object_new();
    json_object_set_string_member(json, "channel", channel);
    json_object_set_string_member(json, "character", character);
    flist_request(pc, FLIST_CHANNEL_INVITE, json);
    json_object_unref(json);

    return PURPLE_CMD_STATUS_OK;
}

static void flist_channel_destroy(void *p) {
    FListChannel *fchannel = (FListChannel *) p;
    g_free(fchannel->name);
    if(fchannel->owner) g_free(fchannel->owner);
    if(fchannel->topic) g_free(fchannel->topic);
    flist_g_list_free_full(fchannel->operators, g_free);
    g_hash_table_destroy(fchannel->users);
}

FListChannel *flist_channel_find(FListAccount *fla, const gchar *name) {
    return g_hash_table_lookup(fla->chat_table, name);
}

const gchar *flist_channel_get_title(FListChannel *channel) {
    if(channel->title) return channel->title;
    return channel->name;
}

GList *flist_channel_list_all(FListAccount *fla) {
    return g_hash_table_get_values(fla->chat_table);
}

GList *flist_channel_list_names(FListAccount *fla) {
    return g_hash_table_get_keys(fla->chat_table);
}

void flist_channel_subsystem_load(FListAccount *fla) {
    fla->chat_timestamp = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    fla->chat_table = g_hash_table_new_full((GHashFunc) flist_str_hash, (GEqualFunc) flist_str_equal, g_free, (GDestroyNotify) flist_channel_destroy);
}

void flist_channel_subsystem_unload(FListAccount *fla) {
    g_hash_table_destroy(fla->chat_timestamp);
    g_hash_table_destroy(fla->chat_table);
}
