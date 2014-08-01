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
#include "f-list_callbacks.h"

static GHashTable *callbacks = NULL;

//NOT IMPLEMENTED: AWC // ADMIN ALTERNATE WATCH

//WILL NOT IMPLEMENT: IGN // IGNORE CHARACTER
//WILL NOT IMPLEMENT: OPP // ???????
//TODO: RAN // ADVERTISE PRIVATE CHANNEL

#define FLIST_ERROR_PROFILE_FLOOD    7

static GHashTable *flist_global_ops_new() {
    return g_hash_table_new_full((GHashFunc)flist_str_hash, (GEqualFunc)flist_str_equal, g_free, NULL);
}
static void flist_purple_find_chats_in_node(PurpleAccount *pa, PurpleBlistNode *n, GSList **current) {
    while(n) {
        if(n->type == PURPLE_BLIST_CHAT_NODE && PURPLE_CHAT(n)->account == pa) {
            *current = g_slist_prepend(*current, n);
        }
        if(n->child) flist_purple_find_chats_in_node(pa, n->child, current);
        n = n->next;
    }
}
/* this might not be the best way to find buddies ... */
/*static void flist_purple_find_buddies_in_node(PurpleAccount *pa, PurpleBlistNode *n, GSList **current) {
    while(n) {
        if(PURPLE_BLIST_NODE_IS_BUDDY(n) && PURPLE_BUDDY(n)->account == pa) {
            purple_debug_info("flist", "found buddy: %s %x\n", purple_buddy_get_name(PURPLE_BUDDY(n)), n);
            *current = g_slist_prepend(*current, n);
        }
        if(n->child) flist_purple_find_buddies_in_node(pa, n->child, current);
        n = n->next;
    }
}*/

static void flist_got_online(PurpleConnection *pc) {
    PurpleAccount *pa = purple_connection_get_account(pc);
    FListAccount *fla = pc->proto_data;
    PurpleGroup *filter_group, *chat_group;
    GSList *chats = NULL;
    GSList *cur;

    chat_group = flist_get_chat_group(fla);
    filter_group = flist_get_filter_group(fla);

    if(chat_group) {
        flist_purple_find_chats_in_node(pa, PURPLE_BLIST_NODE(chat_group)->child, &chats);
        cur = chats;
        while(cur) {
            purple_blist_remove_chat(cur->data);
            cur = g_slist_next(cur);
        }
        g_slist_free(chats);
    }

    purple_connection_set_state(pc, PURPLE_CONNECTED);
    fla->online = TRUE;

    flist_update_server_status(fla);
    
    /* Start managing the friends list. */
    flist_friends_login(fla);
}

static gboolean flist_process_ERR(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    const gchar *message;
    long number;
    
    message = json_object_get_string_member(root, "message");

    if(fla->connection_status == FLIST_IDENTIFY) {
        purple_connection_error_reason(pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, message);
        return FALSE;
    }

    number = json_object_get_int_member(root, "number");
    switch(number) {
    case FLIST_ERROR_PROFILE_FLOOD: //too many profile requests
        flist_profile_process_flood(fla, message);
        return TRUE;
    }
    
    purple_notify_warning(pc, "F-List Error", "An error has occurred on F-List.", message);
    
    return TRUE;
    //TODO: error messages have context!
}

static gboolean flist_process_NLN(PurpleConnection *pc, JsonObject *root) {
    PurpleAccount *pa = purple_connection_get_account(pc);
    FListAccount *fla = pc->proto_data;
    
    g_return_val_if_fail(root, TRUE);

    FListCharacter *character = g_new0(FListCharacter, 1);
    character->name = g_strdup(json_object_get_string_member(root, "identity"));
    character->gender = flist_parse_gender(json_object_get_string_member(root, "gender"));
    character->status = flist_parse_status(json_object_get_string_member(root, "status"));
    character->status_message = g_strdup("");

    g_hash_table_replace(fla->all_characters, g_strdup(flist_normalize(pa, character->name)), character);
    fla->character_count += 1;

    flist_update_friend(pc, character->name, TRUE, FALSE);
    
    if(!fla->online && flist_str_equal(fla->proper_character, character->name)) {
        flist_got_online(pc);
    }

    return TRUE;
}

gint flist_channel_cmp(FListRoomlistChannel *c1, FListRoomlistChannel *c2) {
    return ((gint) c2->users) - ((gint) c1->users);
}

static void handle_roomlist(PurpleRoomlist *roomlist, JsonArray *json, gboolean public) {
    int i, len;
    GSList *channels = NULL, *cur;

    len = json_array_get_length(json);
    for(i = 0; i < len; i++) {
        JsonObject *object = json_array_get_object_element(json, i);
        FListRoomlistChannel *c = g_new0(FListRoomlistChannel, 1);
        const gchar *title, *name;
        int characters;
        title = json_object_get_string_member(object, "title");
        name = json_object_get_string_member(object, "name");
        characters = json_object_get_parse_int_member(object, "characters", NULL);
        c->name = name ? g_strdup(name) : NULL;
        c->title = title ? g_strdup(title) : NULL;
        c->users = characters;
        c->is_public = public;
        channels = g_slist_prepend(channels, c);
    }

    channels = g_slist_sort(channels, (GCompareFunc) flist_channel_cmp);

    cur = channels;
    while(cur) {
        FListRoomlistChannel *c = cur->data;
        PurpleRoomlistRoom *room = purple_roomlist_room_new(PURPLE_ROOMLIST_ROOMTYPE_ROOM, c->title ? c->title : c->name, NULL);
        purple_roomlist_room_add_field(roomlist, room, c->name);
        purple_roomlist_room_add_field(roomlist, room, c->is_public ? "Public" : "Private");
        purple_roomlist_room_add_field(roomlist, room, GINT_TO_POINTER(c->users));
        purple_roomlist_room_add(roomlist, room);

        if(c->name) g_free(c->name);
        if(c->title) g_free(c->title);
        g_free(c);

        cur = g_slist_next(cur);
    }

    g_slist_free(channels);
}

static gboolean flist_process_CHA(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    JsonArray *channels;
    
    if(fla->roomlist == NULL) return TRUE;
    
    g_return_val_if_fail(root, TRUE);
    channels = json_object_get_array_member(root, "channels");
    g_return_val_if_fail(channels, TRUE);
    
    handle_roomlist(fla->roomlist, channels, TRUE);
    
    purple_roomlist_set_in_progress(fla->roomlist, TRUE);
    flist_request(pc, FLIST_REQUEST_PRIVATE_CHANNEL_LIST, NULL);
    
    return TRUE;
}

static gboolean flist_process_ORS(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    JsonArray *channels;
    
    if(fla->roomlist == NULL) return TRUE;
    
    g_return_val_if_fail(root, TRUE);
    channels = json_object_get_array_member(root, "channels");
    g_return_val_if_fail(channels, TRUE);
    
    handle_roomlist(fla->roomlist, channels, FALSE);
    
    purple_roomlist_set_in_progress(fla->roomlist, FALSE);
    return TRUE;
}

static gboolean flist_process_STA(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    FListCharacter *character;
    const gchar *name, *status, *status_message;

    g_return_val_if_fail(root, TRUE);
    name = json_object_get_string_member(root, "character");
    status = json_object_get_string_member(root, "status");
    status_message = json_object_get_string_member(root, "statusmsg");
    g_return_val_if_fail(name, TRUE);

    character = g_hash_table_lookup(fla->all_characters, name);
    if(character) {
        if(status) {
            character->status = flist_parse_status(status);
        }
        if(status_message) {
            character->status_message = g_markup_escape_text(status_message, -1);
        }
        flist_update_friend(pc, name, FALSE, FALSE);
    }
    
    return TRUE;
}

static gboolean flist_process_FLN(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    const gchar *character;
    
    g_return_val_if_fail(root, TRUE);
    
    character = json_object_get_string_member(root, "character");
    g_hash_table_remove(fla->all_characters, character);
    fla->character_count -= 1;

    flist_update_friend(pc, character, FALSE, FALSE);
    flist_update_user_chats_offline(pc, character);
    
    return TRUE;
}

static gboolean flist_process_MSG(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    PurpleAccount *pa = purple_connection_get_account(pc);
    PurpleConversation *convo;
    const gchar *character;
    const gchar *message;
    const gchar *channel;
    gchar *parsed;
    gboolean show;
    PurpleMessageFlags flags;
    
    channel = json_object_get_string_member(root, "channel");
    character = json_object_get_string_member(root, "character");
    message = json_object_get_string_member(root, "message");
    
    convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, channel, pa);
    if(!convo) {
        purple_debug_error("flist", "Received message for channel %s, but we are not in this channel.\n", channel);
        return TRUE;
    }
    
    show = flist_get_channel_show_chat(fla, channel);
    flags = (show ? PURPLE_MESSAGE_RECV : PURPLE_MESSAGE_INVISIBLE);

    parsed = flist_bbcode_to_html(fla, convo, message);
    purple_debug_info("flist", "Message: %s\n", parsed);
    if(show) {
        serv_got_chat_in(pc, purple_conv_chat_get_id(PURPLE_CONV_CHAT(convo)), character, flags, parsed, time(NULL));
    }
    g_free(parsed);
    return TRUE;
}

//TODO: Record advertisements for later use.
static gboolean flist_process_LRP(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    PurpleAccount *pa = purple_connection_get_account(pc);
    PurpleConversation *convo;
    const gchar *character;
    const gchar *message;
    const gchar *channel;
    gchar *full_message, *parsed;
    gboolean show;
    PurpleMessageFlags flags;
    
    channel = json_object_get_string_member(root, "channel");
    character = json_object_get_string_member(root, "character");
    message = json_object_get_string_member(root, "message");
    
    convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, channel, pa);
    if(!convo) {
        purple_debug_error("flist", "Received advertisement for channel %s, but we are not in this channel.\n", channel);
        return TRUE;
    }
    
    show = flist_get_channel_show_ads(fla, channel);
    flags = (show ? PURPLE_MESSAGE_RECV : PURPLE_MESSAGE_INVISIBLE);

    full_message = g_strdup_printf("[b](Roleplay Ad)[/b] %s", message);
    parsed = flist_bbcode_to_html(fla, convo, full_message);
    purple_debug_info("flist", "Advertisement: %s\n", parsed);
    if(show) {
        serv_got_chat_in(pc, purple_conv_chat_get_id(PURPLE_CONV_CHAT(convo)), character, flags, parsed, time(NULL));
    }
    g_free(parsed);
    g_free(full_message);
    return TRUE;
}


static gboolean flist_process_BRO(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    const gchar *message, *character;
    gchar *parsed, *final;
    
    message = json_object_get_string_member(root, "message");
    character = json_object_get_string_member(root, "character");
    
    g_return_val_if_fail(message, TRUE);
    if(!character) character = GLOBAL_NAME;
    
    parsed = flist_bbcode_to_html(fla, NULL, message);
    final = g_strdup_printf("(Broadcast) %s", parsed);
    serv_got_im(pc, character, parsed, PURPLE_MESSAGE_RECV, time(NULL));
    
    g_free(parsed); g_free(final);
    return TRUE;
}

static gboolean flist_process_SYS(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    PurpleAccount *pa = purple_connection_get_account(pc);
    const gchar *message;
    const gchar *channel, *sender;
    gchar *parsed, *final;
    PurpleConversation *convo;

    g_return_val_if_fail(root, TRUE);
    message = json_object_get_string_member(root, "message");
    channel = json_object_get_string_member(root, "channel");
    sender = json_object_get_string_member(root, "sender");

    if(channel) {
        convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, channel, pa);
        if(!convo) {
            purple_debug(PURPLE_DEBUG_ERROR, "flist", "Received system message for channel %s, but we are not in this channel.\n", channel);
            return TRUE;
        }
        parsed = flist_bbcode_to_html(fla, convo, message);
        purple_conv_chat_write(PURPLE_CONV_CHAT(convo), "System", parsed, PURPLE_MESSAGE_SYSTEM, time(NULL));
        g_free(parsed);
    } else if(sender) {
        parsed = flist_bbcode_to_html(fla, NULL, message);
        serv_got_im(pc, sender, parsed, PURPLE_MESSAGE_SYSTEM, time(NULL));
        g_free(parsed);
    } else {
        parsed = flist_bbcode_to_html(fla, NULL, message);
        final = g_strdup_printf("(System) %s", parsed);
        serv_got_im(pc, GLOBAL_NAME, final, PURPLE_MESSAGE_SYSTEM, time(NULL));
        g_free(final);
        g_free(parsed);
    }

    //TODO: parse out channel invitations!
    return TRUE;
}

static gboolean flist_process_CON(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    gboolean success;

    fla->characters_remaining = json_object_get_parse_int_member(root, "count", &success);
    
    if(!success) {
        purple_connection_error_reason(pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, "Could not parse the number of users online.");
        return FALSE;
    }

    return TRUE;
}

static gboolean flist_process_LIS(PurpleConnection *pc, JsonObject *root) {
    PurpleAccount *pa = purple_connection_get_account(pc);
    FListAccount *fla = pc->proto_data;
    JsonArray *characters;
    guint32 len;
    JsonArray *character_array;
    int i;
    
    g_return_val_if_fail(root, TRUE);
    characters = json_object_get_array_member(root, "characters");
    g_return_val_if_fail(characters, TRUE);
    
    len = json_array_get_length(characters);
    for(i = 0; i < len; i++) {
        FListCharacter *character = g_new0(FListCharacter, 1);
        character_array = json_array_get_array_element(characters, i);
        
        g_return_val_if_fail(character_array, TRUE);
        g_return_val_if_fail(json_array_get_length(character_array) == 4, TRUE);
        
        character->name = g_strdup(json_array_get_string_element(character_array, 0));
        character->gender = flist_parse_gender(json_array_get_string_element(character_array, 1));
        character->status = flist_parse_status(json_array_get_string_element(character_array, 2));
        character->status_message = g_markup_escape_text(json_array_get_string_element(character_array, 3), -1);
        
        g_hash_table_replace(fla->all_characters, g_strdup(flist_normalize(pa, character->name)), character);
        flist_update_friend(pc, character->name, TRUE, FALSE);
    }
    
    return TRUE;
}

static gboolean flist_process_AOP(PurpleConnection *pc, JsonObject *root) {
    PurpleAccount *pa = purple_connection_get_account(pc);
    FListAccount *fla = pc->proto_data;
    const gchar *character;
    gchar *identity;

    character = json_object_get_string_member(root, "character");
    g_return_val_if_fail(character, TRUE);
    if(!fla->global_ops) fla->global_ops = flist_global_ops_new();

    identity = g_strdup(character);
    g_hash_table_replace(fla->global_ops, identity, identity);
    flist_update_user_chats_rank(pc, identity);

    g_free(identity);

    purple_prpl_got_account_actions(pa);
    return TRUE;
}

static gboolean flist_process_DOP(PurpleConnection *pc, JsonObject *root) {
    PurpleAccount *pa = purple_connection_get_account(pc);
    FListAccount *fla = pc->proto_data;
    const gchar *character;

    character = json_object_get_string_member(root, "character");
    g_return_val_if_fail(character, TRUE);
    if(!fla->global_ops) fla->global_ops = flist_global_ops_new();

    g_hash_table_remove(fla->global_ops, character);
    flist_update_user_chats_rank(pc, character);

    purple_prpl_got_account_actions(pa);
    return TRUE;
}

static gboolean flist_process_ADL(PurpleConnection *pc, JsonObject *root) {
    PurpleAccount *pa = purple_connection_get_account(pc);
    FListAccount *fla = pc->proto_data;
    JsonArray *ops;
    int i, len;
    GHashTable *old_table;
    
    ops = json_object_get_array_member(root, "ops");
    g_return_val_if_fail(ops, TRUE);

    old_table = fla->global_ops;

    /* fill in the new table */
    fla->global_ops = flist_global_ops_new();
    len = json_array_get_length(ops);
    for(i = 0; i < len; i++) {
        gchar *identity = g_strdup(json_array_get_string_element(ops, i));
        g_hash_table_insert(fla->global_ops, identity, identity);
    }

    /* update the status of everyone in the old table */
    if(old_table) {
        GList *list = g_hash_table_get_keys(old_table);
        flist_update_users_chats_rank(pc, list);
        g_list_free(list);
        g_hash_table_destroy(old_table);
    }

    /* update the status of everyone in the new table */
    if(fla->global_ops) { //always TRUE
        GList *list = g_hash_table_get_keys(fla->global_ops);
        flist_update_users_chats_rank(pc, list);
        g_list_free(list);
    }

    purple_prpl_got_account_actions(pa);
    return TRUE;
}

static gboolean flist_process_TPN(PurpleConnection *pc, JsonObject *root) {
    const gchar *character, *status;
    
    character = json_object_get_string_member(root, "character");
    status = json_object_get_string_member(root, "status");
    
    serv_got_typing(pc, character, 0, flist_typing_state(status));
    
    return TRUE;
}


typedef gboolean(*flist_cb_fn)(PurpleConnection *, JsonObject *);

static gboolean flist_process_IDN(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    const gchar *character;
    fla->connection_status = FLIST_ONLINE;
    character = json_object_get_string_member(root, "character");
    fla->proper_character = g_strdup(character);
    return TRUE;
}
static gboolean flist_process_PIN(PurpleConnection *pc, JsonObject *root) {
    flist_request(pc, "PIN", NULL);
    flist_receive_ping(pc);
    return TRUE;
}
static gboolean flist_process_PRI(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    const gchar *character = json_object_get_string_member(root, "character");
    const gchar *message = json_object_get_string_member(root, "message");
    /* TODO: this will not work if this message opens a new window */
    PurpleConversation *convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, character, fla->pa);
    gchar *parsed = flist_bbcode_to_html(fla, convo, message);
    serv_got_im(pc, character, parsed, PURPLE_MESSAGE_RECV, time(NULL));
    g_free(parsed);
    return TRUE;
}

gboolean flist_callback(PurpleConnection *pc, const gchar *code, JsonObject *root) {
    flist_cb_fn callback = g_hash_table_lookup(callbacks, code);
    if(!callback) return TRUE;
    return callback(pc, root);
}
void flist_callback_init() {
    if(callbacks) return;
    callbacks = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
    
    //TODO:
            //HLO - server MOTD
            //KIN - kinks data
            //VAR - server variables
    
    g_hash_table_insert(callbacks, "RTB", flist_process_RTB);
    
    g_hash_table_insert(callbacks, "TPN", flist_process_TPN);
    
    /* info on admins */
    g_hash_table_insert(callbacks, "ADL", flist_process_ADL);
    g_hash_table_insert(callbacks, "AOP", flist_process_AOP);
    g_hash_table_insert(callbacks, "DOP", flist_process_DOP);

    /* admin broadcast */
    g_hash_table_insert(callbacks, "BRO", flist_process_BRO);

    /* system message */
    g_hash_table_insert(callbacks, "SYS", flist_process_SYS);
    
    //TODO: write RLL its own function
    g_hash_table_insert(callbacks, "RLL", flist_process_MSG);

    /* kink search */
    g_hash_table_insert(callbacks, "FKS", flist_process_FKS);

    g_hash_table_insert(callbacks, "PIN", flist_process_PIN);
    g_hash_table_insert(callbacks, "ERR", flist_process_ERR);

    g_hash_table_insert(callbacks, "IDN", flist_process_IDN);
    g_hash_table_insert(callbacks, "PRI", flist_process_PRI);
    g_hash_table_insert(callbacks, "LIS", flist_process_LIS);
    g_hash_table_insert(callbacks, "CON", flist_process_CON);
    g_hash_table_insert(callbacks, "NLN", flist_process_NLN);
    g_hash_table_insert(callbacks, "STA", flist_process_STA);
    g_hash_table_insert(callbacks, "FLN", flist_process_FLN);
    
    //profile request
    g_hash_table_insert(callbacks, "PRD", flist_process_PRD);
    
    //channel list callbacks
    g_hash_table_insert(callbacks, "CHA", flist_process_CHA); //public channel list
    g_hash_table_insert(callbacks, "ORS", flist_process_ORS); //private channel list
    
    //channel event callbacks
    g_hash_table_insert(callbacks, "COL", flist_process_COL); //op list
    g_hash_table_insert(callbacks, "JCH", flist_process_JCH); //join channel
    g_hash_table_insert(callbacks, "LCH", flist_process_LCH); //leave channel
    g_hash_table_insert(callbacks, "CBU", flist_process_CBU); //channel ban
    g_hash_table_insert(callbacks, "CKU", flist_process_CKU); //channel kick
    g_hash_table_insert(callbacks, "ICH", flist_process_ICH); //in channel
    g_hash_table_insert(callbacks, "MSG", flist_process_MSG); //channel message
    g_hash_table_insert(callbacks, "LRP", flist_process_LRP); //channel ad
    g_hash_table_insert(callbacks, "CDS", flist_process_CDS);
    g_hash_table_insert(callbacks, "CIU", flist_process_CIU); //channel invite

    //staff call
    g_hash_table_insert(callbacks, "SFC", flist_process_SFC);
}
