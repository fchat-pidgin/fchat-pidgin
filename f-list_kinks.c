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
#include "f-list_kinks.h"

typedef struct FListKink_ FListKink;

//TODO: There has to be a better way to do this.
const gchar *ROLES[7] = {"Always dominant", "Usually dominant", "Switch", "Usually submissive", "Always submissive", "None", NULL};
const gchar *POSITIONS[7] = {"Always Top", "Usually Top", "Switch", "Usually Bottom", "Always Bottom", "None", NULL};

#define LAST_LOOKING "last_search0"
#define LAST_KINK1 "last_search1"
#define LAST_KINK2 "last_search2"
#define LAST_KINK3 "last_search3"
#define LAST_GENDERS "last_search4"
#define LAST_ROLES "last_search5"

struct FListKinks_ {
    FListWebRequestData *global_kinks_request;
    
    GHashTable *kinks_table;
    GList *kinks_list;
    GSList *filter_channel_choices;
    GSList *filter_kink_choices;
    GSList *filter_gender_choices;
    GSList *filter_role_choices;
    GSList *filter_position_choices;
    
    gboolean local;
    gboolean looking;
    int kink1, kink2, kink3;
    int genders, roles;
};

struct FListKink_ {
    gchar *kink_id;
    gchar *name;
    gchar *category;
    gchar *description;
};

static inline FListKinks* _flist_kinks(FListAccount *fla) {
    return fla->flist_kinks;
}

static int flist_kinkcmp(FListKink *kink1, FListKink *kink2) {
    return strcmp(kink1->name, kink2->name);
}

static int flist_filter_get_genders(FListKinks *flk) {
    int ret = 0;
    GSList *cur = flk->filter_gender_choices; int index = 0;
    
    while(cur) {
        if(flk->genders & (1 << index)) {
            FListGender gender = flist_parse_gender(g_slist_nth_data(flk->filter_gender_choices, index));
            ret |= gender;
        }
        cur = cur->next; index++;
    }
    
    return ret;
}

static GSList *flist_get_filter_characters(FListAccount *fla, gboolean has_extra, GSList *extra) {
    FListKinks *flk = _flist_kinks(fla);
    GSList *all, *cur;
    GSList *ret = NULL, *names = NULL;
    int genders = flist_filter_get_genders(flk);
    
    /* get the initial list by filtering by gender and status */
    all = flist_get_all_characters(fla);
    cur = all;
    while(cur) {
        FListCharacter *character = cur->data;
        if(!flk->looking || character->status == FLIST_STATUS_LOOKING)
            if(character->gender & genders)
                ret = g_slist_prepend(ret, character);
        cur = g_slist_next(cur);
    }
    g_slist_free(all);
    
    /* if there is a channel chosen, filter by that, too */
    if(fla->filter_channel) {
        PurpleConversation *convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, fla->filter_channel, fla->pa);
        if(convo) {
            GSList *tmp = NULL;
            purple_debug_info("flist", "We filtered on channel %s.\n", fla->filter_channel);
            //TODO: do we have to clean this up when we're done with it? The API is unclear.
            GList *chat_users = purple_conv_chat_get_users(PURPLE_CONV_CHAT(convo));
            while(chat_users) {
                PurpleConvChatBuddy *b = chat_users->data;
                FListCharacter *character = flist_get_character(fla, b->name);
                if(character) tmp = g_slist_prepend(tmp, character);
                chat_users = g_list_next(chat_users);
            }
            ret = flist_g_slist_intersect_and_free(ret, tmp);
        } else {
            purple_debug_info("flist", "We tried to filter on channel %s, but no channel was found.\n", fla->filter_channel);
        }
    }
    
    /* if we have server-side results, filter by them last */
    if(has_extra) {
        GSList *tmp = NULL;
        cur = extra;
        while(cur) {
            FListCharacter *character = flist_get_character(fla, cur->data);
            if(character) tmp = g_slist_prepend(tmp, character);
            cur = g_slist_next(cur);
        }
        ret = flist_g_slist_intersect_and_free(ret, tmp);
    }
    
    cur = ret;
    while(cur) {
        FListCharacter *character = cur->data;
        names = g_slist_prepend(names, character->name);
        cur = g_slist_next(cur);
    }
    g_slist_free(ret);
    
    return names;
}

gboolean flist_process_FKS(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    const gchar *kink;
    JsonArray *characters;
    int i, len;
    GSList *character_list = NULL;
    
    kink = json_object_get_string_member(root, "kinkid");
    characters = json_object_get_array_member(root, "characters");

    len = json_array_get_length(characters);
    for(i = 0; i < len; i++) {
        const gchar *identity = json_array_get_string_element(characters, i);
        character_list = g_slist_prepend(character_list, (gpointer) identity); //we could copy this, but it doesn't matter
    }

    purple_debug_info(FLIST_DEBUG, "There are %d results to our kink search.\n", g_slist_length(character_list));
    
    flist_apply_filter(fla, flist_get_filter_characters(fla, TRUE, character_list));

    g_slist_free(character_list);

    return TRUE;
}

static void flist_filter_json_flags(JsonArray *array, GSList *cur, int flags) {
    int index = 0;
    while(cur) {
        if(flags & (1 << index)) {
            json_array_add_string_element(array, cur->data);
        }
        cur = cur->next; index++;
    }
}

static void flist_filter_pack(PurpleRequestFieldGroup *group, const gchar *name, GSList *choices, int previous) {
    PurpleRequestField *field;
    int index = 0;
    /* this is a server and/or client side filter */
    while(choices) {
        gchar *s = g_strdup_printf("%s%d", name, index);
        field = purple_request_field_bool_new(s, choices->data, previous & (1 << index));
        purple_request_field_group_add_field(group, field);
        choices = choices->next; index++;
        g_free(s);
    }
}

static int flist_filter_unpack(PurpleRequestFields *fields, const gchar *name, GSList *choices) {
    int ret = 0;
    int index = 0;
    
    while(choices) {
        gchar *s = g_strdup_printf("%s%d", name, index);
        if(purple_request_fields_get_bool(fields, s)) {
            ret |= (1 << index);
        }
        choices = choices->next; index++;
        g_free(s);
    }
    return ret;
}

static void flist_filter_done(FListAccount *fla) {
    FListKinks *flk = _flist_kinks(fla);
    JsonObject *json;
    JsonArray *kinks, *genders, *roles;
        
    if(!flk->local) {
        const gchar *kink1, *kink2, *kink3;
        json = json_object_new();
        genders = json_array_new();
        roles = json_array_new();
        kinks = json_array_new();
        
        kink1 = g_slist_nth_data(flk->filter_kink_choices, flk->kink1);
        kink2 = g_slist_nth_data(flk->filter_kink_choices, flk->kink2);
        kink3 = g_slist_nth_data(flk->filter_kink_choices, flk->kink3);
        
        if(kink1) json_array_add_string_element(kinks, kink1);
        if(kink2) json_array_add_string_element(kinks, kink2);
        if(kink3) json_array_add_string_element(kinks, kink3);

        flist_filter_json_flags(genders, flk->filter_gender_choices, flk->genders);
        flist_filter_json_flags(roles, flk->filter_role_choices, flk->roles);
        
        json_object_set_array_member(json, "kinks", kinks);
        json_object_set_array_member(json, "genders", genders);
        json_object_set_array_member(json, "roles", roles);
        
        flist_request(fla->pc, FLIST_KINK_SEARCH, json);
        
        /* unreference the json */
        json_array_unref(kinks);
        json_array_unref(genders);
        json_array_unref(roles);
        json_object_unref(json);
    } else {
        flist_apply_filter(fla, flist_get_filter_characters(fla, FALSE, NULL));
    }
    
    purple_account_set_int(fla->pa, LAST_GENDERS, flk->genders);
    purple_account_set_int(fla->pa, LAST_KINK1, flk->kink1);
    purple_account_set_int(fla->pa, LAST_KINK2, flk->kink2);
    purple_account_set_int(fla->pa, LAST_KINK3, flk->kink3);
    purple_account_set_int(fla->pa, LAST_LOOKING, flk->looking);
    purple_account_set_int(fla->pa, LAST_ROLES, flk->roles);
    
    fla->input_request = FALSE;
}

static void flist_filter_cancel(gpointer user_data) {
    FListAccount *fla = user_data;
    FListKinks *flk = _flist_kinks(fla);
    if(fla->filter_channel) g_free(fla->filter_channel);
    if(flk->filter_channel_choices) {
        flist_g_slist_free_full(flk->filter_channel_choices, (GDestroyNotify) g_free);
        flk->filter_channel_choices = NULL;
    }
    fla->input_request = FALSE;
}

static void flist_filter_dialog(FListAccount *fla, PurpleRequestFields *fields, GCallback callback) {
    purple_request_fields(fla->pc, _("Character Search!"), _("Search for characters on the F-List server."), _("Please fill out the search form."),
        fields,
        _("OK"), callback,
        _("Cancel"), G_CALLBACK(flist_filter_cancel),
        fla->pa, NULL, NULL, fla);
}

static void flist_filter3_cb(gpointer user_data, PurpleRequestFields *fields) {
    FListAccount *fla = user_data;
    FListKinks *flk = _flist_kinks(fla);
    
    flk->roles = flist_filter_unpack(fields, "role", flk->filter_role_choices);
    
    flist_filter_done(fla);
}

static void flist_filter3(FListAccount *fla) {
    FListKinks *flk = _flist_kinks(fla);
    PurpleRequestFields *fields = purple_request_fields_new();
    PurpleRequestFieldGroup *group = purple_request_field_group_new("Options");
    
    flist_filter_pack(group, "role", flk->filter_role_choices, flk->roles);
    purple_request_fields_add_group(fields, group);
        
    flist_filter_dialog(fla, fields, G_CALLBACK(flist_filter3_cb));
}

static void flist_filter2_cb(gpointer user_data, PurpleRequestFields *fields) {
    FListAccount *fla = user_data;
    FListKinks *flk = _flist_kinks(fla);
    
    flk->genders = flist_filter_unpack(fields, "gender", flk->filter_gender_choices);
    
    flk->local = !(flk->kink1 || flk->kink2 || flk->kink3);    
    if(flk->local) { /* If this is a local search, we can't search for roles. */
        flist_filter_done(fla);
    } else { /* If this is not a local search, continue. */
        flist_filter3(fla);
    }
}

static void flist_filter2(FListAccount *fla) {
    FListKinks *flk = _flist_kinks(fla);
    PurpleRequestFields *fields = purple_request_fields_new();
    PurpleRequestFieldGroup *group = purple_request_field_group_new("Options");
    
    purple_request_fields_add_group(fields, group);
    flist_filter_pack(group, "gender", flk->filter_gender_choices, flk->genders);
    
    flist_filter_dialog(fla, fields, G_CALLBACK(flist_filter2_cb));
}

static void flist_filter1_cb(gpointer user_data, PurpleRequestFields *fields) {
    FListAccount *fla = user_data;
    FListKinks *flk = _flist_kinks(fla);
    int channel_index;
    const gchar *channel;
    
    /* these are server-side filters */
    flk->kink1 = purple_request_fields_get_choice(fields, "kink1");
    flk->kink2 = purple_request_fields_get_choice(fields, "kink2");
    flk->kink3 = purple_request_fields_get_choice(fields, "kink3");

    /* this is a client-side filter */
    flk->looking = purple_request_fields_get_bool(fields, "looking");
    
    /* this is a client-side filter*/
    channel_index = purple_request_fields_get_choice(fields, "channel");
    channel = g_slist_nth_data(flk->filter_channel_choices, channel_index);
    if(fla->filter_channel) g_free(fla->filter_channel);
    fla->filter_channel = channel ? g_strdup(channel) : NULL;
    flist_g_slist_free_full(flk->filter_channel_choices, (GDestroyNotify) g_free);
    flk->filter_channel_choices = NULL;
    
    flist_filter2(fla);
}

static void flist_add_kink_field(FListKinks *flk, PurpleRequestFieldGroup *group, const gchar *name, int start) {
    PurpleRequestField *field = purple_request_field_choice_new(name, _("Kink"), start);
    GList *cur;
    
    purple_request_field_choice_add(field, "(No Filter)");
    cur = flk->kinks_list;
    while(cur) {
        FListKink *kink = cur->data;
        purple_request_field_choice_add(field, kink->name);
        cur = g_list_next(cur);
    }
    purple_request_field_group_add_field(group, field);
}

static void flist_filter1(FListAccount *fla, const gchar *default_channel) {
    FListKinks *flk = _flist_kinks(fla);
    PurpleRequestFields *fields;
    PurpleRequestFieldGroup *group;
    PurpleRequestField *field;
    GList *channels, *cur;
    int i = 0;
    
    group = purple_request_field_group_new("Options");
    fields = purple_request_fields_new();
    purple_request_fields_add_group(fields, group);

    field = purple_request_field_bool_new("looking", "Looking Only", flk->looking);
    purple_request_field_group_add_field(group, field);

    /* now, add all of our current channels to the list */
    field = purple_request_field_choice_new("channel", _("Channel"), 0);
    purple_request_field_choice_add(field, "(Any Channel)");
    
    flk->filter_channel_choices = g_slist_prepend(flk->filter_channel_choices, NULL);
    channels = flist_channel_list_all(fla);
    i = 1;
    for(cur = channels; cur; cur = g_list_next(cur)) {
        FListChannel *channel = cur->data;
        purple_request_field_choice_add(field, flist_channel_get_title(channel));
        if(default_channel && flist_str_equal(channel->name, default_channel)) {
            purple_request_field_choice_set_default_value(field, i);
        }
        flk->filter_channel_choices = g_slist_prepend(flk->filter_channel_choices, g_strdup(channel->name));
        i++;
    }
    g_list_free(channels);
    purple_request_field_group_add_field(group, field);
    flk->filter_channel_choices = g_slist_reverse(flk->filter_channel_choices);

    /* now, let's fill in the kink list part */
    if(flk->kinks_table) {
        flist_add_kink_field(flk, group, "kink1", flk->kink1);
        flist_add_kink_field(flk, group, "kink2", flk->kink2);
        flist_add_kink_field(flk, group, "kink3", flk->kink3);
    }
    
    flist_filter_dialog(fla, fields, G_CALLBACK(flist_filter1_cb));
}

static void flist_filter_real(PurpleConnection *pc, const gchar *channel) {
    FListAccount *fla;

    g_return_if_fail(pc);
    g_return_if_fail((fla = pc->proto_data));
    
    if(fla->input_request) return;

    flist_filter1(fla, channel);
}

void flist_filter_action(PurplePluginAction *action) {
    PurpleConnection *pc = action->context;
    flist_filter_real(pc, NULL);
}

PurpleCmdRet flist_filter_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    flist_filter_real(pc, NULL); //TODO: put the proper channel title here

    return PURPLE_CMD_STATUS_OK;
}

static void flist_global_kinks_cb(FListWebRequestData *req_data,
        gpointer user_data, JsonObject *root, const gchar *error_message) {
    FListAccount *fla = user_data;
    FListKinks *flk = _flist_kinks(fla);
    JsonObject *kinks;
    GList *categories, *cur;
    int i, len;
    
    flk->global_kinks_request = NULL;
    
    if(!root) {
        purple_debug_error(FLIST_DEBUG, "Failed to obtain the global list of kinks. Error Message: %s\n", error_message);
        return;
    }
    
    kinks = json_object_get_object_member(root, "kinks");
    if(!kinks) {
        purple_debug_warning(FLIST_DEBUG, "We received the global list of kinks, but it was empty.\n");
        return;
    }
    
    purple_debug_info(FLIST_DEBUG, "Processing global kink list...\n");
    
    flk->kinks_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL); //TODO: write freeing function!
    
    categories = json_object_get_members(kinks);
    cur = categories;
    while(cur) {
        const gchar *category_id = cur->data;
        JsonArray *kinks_array;
        JsonObject *kink_group;
        const gchar *category_name;
        
        kink_group = json_object_get_object_member(kinks, category_id);
        category_name = json_object_get_string_member(kink_group, "group");
        kinks_array = json_object_get_array_member(kink_group, "items");
        len = json_array_get_length(kinks_array);
        for(i = 0; i < len; i++) {
            JsonObject *kink_object = json_array_get_object_element(kinks_array, i);
            FListKink *kink = g_new0(FListKink, 1);
            kink->category = purple_unescape_html(category_name);
            kink->description = purple_unescape_html(json_object_get_string_member(kink_object, "description"));
            kink->name = purple_unescape_html(json_object_get_string_member(kink_object, "name"));
            kink->kink_id = g_strdup_printf("%d", (gint) json_object_get_int_member(kink_object, "kink_id"));
            g_hash_table_insert(flk->kinks_table, g_strdup(kink->name), kink);
            if(fla->debug_mode) {
                purple_debug_info(FLIST_DEBUG, 
                        "Global kink processed. (ID: %s) (Category: %s) (Name: %s) (Description: %s)\n", 
                        kink->kink_id, kink->category, kink->name, kink->description);
            }
        }
        cur = g_list_next(cur);
    }
    g_list_free(categories);

    purple_debug_info(FLIST_DEBUG, "Global kink list processed. (Kinks: %d)\n", g_hash_table_size(flk->kinks_table));

    flk->kinks_list = g_list_sort(g_hash_table_get_values(flk->kinks_table), (GCompareFunc) flist_kinkcmp);
    
    cur = flk->kinks_list; //TODO: filter down to our kinks only!?
    flk->filter_kink_choices = g_slist_prepend(flk->filter_kink_choices, NULL);
    while(cur) {
        FListKink *kink = cur->data;
        flk->filter_kink_choices = g_slist_prepend(flk->filter_kink_choices, kink->kink_id); //TODO: is this safe?
        cur = g_list_next(cur);
    }
    flk->filter_kink_choices = g_slist_reverse(flk->filter_kink_choices);
}

void flist_global_kinks_load(PurpleConnection *pc) {
    FListAccount *fla = pc->proto_data;
    FListKinks *flk;
    GSList *genders;
    const gchar **p;
    fla->flist_kinks = g_new0(FListKinks, 1);
    flk = _flist_kinks(fla);
    
    purple_debug_info("Fetching global kink list... (Account: %s) (Character: %s)\n", fla->username, fla->character);
    flk->global_kinks_request = flist_web_request(JSON_KINK_LIST, NULL, TRUE, fla->secure, flist_global_kinks_cb, fla);
    
    genders = flist_get_gender_list();
    while(genders) {
        flk->filter_gender_choices = g_slist_prepend(flk->filter_gender_choices, genders->data);
        genders = genders->next;
    }
    flk->filter_gender_choices = g_slist_reverse(flk->filter_gender_choices);
    
    p = ROLES;
    while(*p) {
        flk->filter_role_choices = g_slist_prepend(flk->filter_role_choices, g_strdup(*p));
        p++;
    }
    flk->filter_role_choices = g_slist_reverse(flk->filter_role_choices);
    
    flk->kink1 = purple_account_get_int(fla->pa, LAST_KINK1, 0);
    flk->kink2 = purple_account_get_int(fla->pa, LAST_KINK2, 0);
    flk->kink3 = purple_account_get_int(fla->pa, LAST_KINK3, 0);
    flk->genders = purple_account_get_int(fla->pa, LAST_GENDERS, 0xFFFFFFFF);
    flk->roles = purple_account_get_int(fla->pa, LAST_ROLES, 0xFFFFFFFF);
    flk->looking = purple_account_get_bool(fla->pa, LAST_LOOKING, TRUE);
}

void flist_global_kinks_unload(PurpleConnection *pc) {
    FListAccount *fla = pc->proto_data;
    FListKinks *flk = _flist_kinks(fla);
    
    if(flk->global_kinks_request) {
        flist_web_request_cancel(flk->global_kinks_request);
        flk->global_kinks_request = NULL;
    }
    
    if(flk->kinks_list) {
        g_list_free(flk->kinks_list);
        flk->kinks_list = NULL;
    }
    if(flk->kinks_table) {
        g_hash_table_destroy(flk->kinks_table);
    }

    if(flk->filter_channel_choices) {
        flist_g_slist_free_full(flk->filter_channel_choices, (GDestroyNotify) g_free);
    }
    if(flk->filter_kink_choices) {
        g_slist_free(flk->filter_kink_choices);
    }
    if(flk->filter_gender_choices) {
        g_slist_free(flk->filter_gender_choices);
    }
    if(flk->filter_role_choices) {
        flist_g_slist_free_full(flk->filter_role_choices, (GDestroyNotify) g_free);
    }
    
    g_free(flk);
    fla->flist_kinks = NULL;
}
