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
#include "f-list_profile.h"
#define FLIST_PROFILE_DEFAULT_VALUE "(Unlisted)"
#define FLIST_PROFILE_DEFAULT_CATEGORY "(Unsorted)"

typedef struct FListProfileFieldCategory_ {
    gint sort;
    gchar *name;
    GSList *fields; /* this is a list of FListProfileField, and this should be sorted */
} FListProfileFieldCategory;

typedef struct FListProfileField_ {
    FListProfileFieldCategory *category;
    gchar *fieldid;
    gchar *name;
} FListProfileField;

struct FListProfiles_ {
    FListWebRequestData *global_profile_request;
    GSList *priority_profile_fields;
    GHashTable *category_table;
    GSList *category_list;
    
    gchar *character; /* current request */
    GHashTable *table; /* current request */
    PurpleNotifyUserInfo *profile_info; /* current request */
    FListWebRequestData *profile_request;
};

static inline FListProfiles *_flist_profiles(FListAccount *fla) {
    return fla->flist_profiles;
}

static int flist_profile_field_cmp(FListProfileField *field1, FListProfileField *field2) {
    return flist_strcmp(field1->name, field2->name);
}

static void flist_show_profile(PurpleConnection *pc, const gchar *character, GHashTable *profile, 
        gboolean by_id, PurpleNotifyUserInfo *info) {
    FListAccount *fla = pc->proto_data;
    FListProfiles *flp = _flist_profiles(fla);
    GSList *priority = flp->priority_profile_fields;
    GSList *category_list = flp->category_list;
    GList *remaining;
    
    //Add a section break after the main info.
    purple_notify_user_info_add_section_break(info);

    while(priority) {
        FListProfileField *field = priority->data;
        const gchar *key = !by_id ? field->name : field->fieldid;
        const gchar *value = g_hash_table_lookup(profile, key);
        if(value) {
            purple_notify_user_info_add_pair(info, field->name, value);
            g_hash_table_remove(profile, key);
        } else {
            purple_notify_user_info_add_pair(info, field->name, FLIST_PROFILE_DEFAULT_VALUE);
        }
        priority = g_slist_next(priority);
    }

    //Now, add by category.
    while(category_list) {
        FListProfileFieldCategory *category = category_list->data;
        GSList *field_list = category->fields;
        gboolean first = TRUE;
        while(field_list) {
            FListProfileField *field = field_list->data;
            const gchar *key = !by_id ? field->name : field->fieldid;
            const gchar *value = g_hash_table_lookup(profile, key);
            if(value) {
                if(first) {
                    purple_notify_user_info_add_section_break(info);
                    //purple_notify_user_info_add_section_header(info, category->name);
                }
                purple_notify_user_info_add_pair(info, field->name, value);
                first = FALSE;
                g_hash_table_remove(profile, key);
            }
            field_list = g_slist_next(field_list);
        }
        category_list = g_slist_next(category_list);
    }

    //Now, add everything that we missed.
    //(Ideally, we should do nothing.)
    remaining = g_hash_table_get_keys(profile);
    if(remaining) {
        GList *cur;
        remaining = g_list_sort(remaining, (GCompareFunc) flist_strcmp);
        cur = remaining;
        purple_notify_user_info_add_section_break(info);
        //purple_notify_user_info_add_section_header(info, FLIST_PROFILE_DEFAULT_CATEGORY);
        while(cur) {
            const gchar *field = cur->data;
            const gchar *value = g_hash_table_lookup(profile, field);
            purple_notify_user_info_add_pair(info, field, value);
            g_hash_table_remove(profile, field);
            cur = g_list_next(cur);
        }
        g_list_free(remaining);
    }

    purple_notify_userinfo(pc, character, info, NULL, NULL);
}

gboolean flist_process_PRD(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    FListProfiles *flp = _flist_profiles(fla);
    const gchar *type;
    const gchar *key, *value;

    if(!flp->character) {
        purple_debug(PURPLE_DEBUG_ERROR, "flist", "Profile information received, but we are not expecting profile information.\n");
        return TRUE;
    }

    type = json_object_get_string_member(root, "type");
    if(!g_strcmp0(type, "start")) { /* we don't care! */
        if(flp->table) g_hash_table_destroy(flp->table);
        flp->table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
        return TRUE;
    }
    if(!g_strcmp0(type, "end")) {
        flist_show_profile(pc, flp->character, flp->table, FALSE, flp->profile_info);
        g_free(flp->character); flp->character = NULL;
        if(flp->table) {
            g_hash_table_destroy(flp->table); flp->table = NULL;
        }
        purple_notify_user_info_destroy(flp->profile_info); flp->profile_info = NULL;
        return TRUE;
    }
    if(!flp->table) {
        return TRUE; //this should never happen
    }

    key = json_object_get_string_member(root, "key");
    value = json_object_get_string_member(root, "value");
    g_hash_table_replace(flp->table, g_strdup(key), g_strdup(value));

    purple_debug_info("flist", "Profile information received for %s. Key: %s. Value: %s.\n", flp->character, key, value);

    return TRUE;
}

static gboolean flist_process_profile(FListAccount *fla, JsonObject *root) {
    FListProfiles *flp = _flist_profiles(fla);
    JsonObject *info;
    GList *categories, *cur;
    GHashTable *profile;
    const gchar *error;

    error = json_object_get_string_member(root, "error");
    if(error && strlen(error) > 0) {
        purple_debug_info(FLIST_DEBUG, "We requested a profile from the Web API, but it returned an error. Error Message: %s\n", error);
        return FALSE; //user probably opted out of API access
    }

    profile = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
    info = json_object_get_object_member(root, "info");
    categories = json_object_get_members(info);

    cur = categories;
    while(cur) {
        const gchar *group_name;
        JsonObject *field_group;
        JsonArray *field_array;
        guint i, len;
        
        field_group = json_object_get_object_member(info, cur->data);
        group_name = json_object_get_string_member(field_group, "group");
        field_array = json_object_get_array_member(field_group, "items");

        len = json_array_get_length(field_array);
        for(i = 0; i < len; i++) {
            JsonObject *field_object = json_array_get_object_element(field_array, i);
            const gchar *field_name = json_object_get_string_member(field_object, "name");
            const gchar *field_value = json_object_get_string_member(field_object, "value");
            g_hash_table_insert(profile, (gpointer) field_name, (gpointer) field_value);
        }
        
        cur = cur->next;
    }
    g_list_free(categories);

    flist_show_profile(fla->pc, flp->character, profile, FALSE, flp->profile_info);

    g_hash_table_destroy(profile);
    
    return TRUE;
}

static void flist_get_profile_cb(FListWebRequestData *req_data, gpointer user_data,
        JsonObject *root, const gchar *error_message) {
    FListAccount *fla = user_data;
    FListProfiles *flp = _flist_profiles(fla);
    gboolean success;
    
    flp->profile_request = NULL;
    
    if(!root) {
        purple_debug_warning(FLIST_DEBUG, "We requested a profile from the Web API, but failed. Error Message: %s\n", error_message);
        success = FALSE;
    } else {
        success = flist_process_profile(fla, root);
    }
    
    if(success) {
        g_free(flp->character); flp->character = NULL;
        purple_notify_user_info_destroy(flp->profile_info); flp->profile_info = NULL;
    } else {
        JsonObject *json = json_object_new();
        json_object_set_string_member(json, "character", flp->character);
        flist_request(fla->pc, "PRO", json);
        json_object_unref(json);
    }
}

void flist_get_profile(PurpleConnection *pc, const char *who) {
    FListAccount *fla;
    FListProfiles *flp;
    GString *link_str;
    gchar *link;
    FListCharacter *character;

    g_return_if_fail((fla = pc->proto_data));
    flp = _flist_profiles(fla);
    
    if(flp->character) g_free(flp->character);
    flp->character = NULL;
    if(flp->profile_info) purple_notify_user_info_destroy(flp->profile_info);
    flp->profile_info = NULL;
    if(flp->table) g_hash_table_destroy(flp->table);
    flp->table = NULL;
    if(flp->profile_request) {
        flist_web_request_cancel(flp->profile_request);
        flp->profile_request = NULL;
    }

    flp->character = g_strdup(who);
    flp->profile_info = purple_notify_user_info_new();

    link_str = g_string_new(NULL);
    g_string_append_printf(link_str, "http://www.f-list.net/c/%s", purple_url_encode(who));
    link = g_string_free(link_str, FALSE);

    character = flist_get_character(fla, who);
    if(!character) {
        purple_notify_user_info_add_pair(flp->profile_info, "Status", "Offline");
    } else {
        gchar *parsed_message = flist_bbcode_to_html(fla, NULL, character->status_message);
        purple_notify_user_info_add_pair(flp->profile_info, "Status", flist_format_status(character->status));
        purple_notify_user_info_add_pair(flp->profile_info, "Gender", flist_format_gender(character->gender));
        purple_notify_user_info_add_pair(flp->profile_info, "Message", parsed_message);
        g_free(parsed_message);
    }
    purple_notify_user_info_add_pair(flp->profile_info, "Link", link);
    purple_notify_userinfo(pc, flp->character, flp->profile_info, NULL, NULL);

    if(!character) {
        //The character is offline. There's nothing more we should do.
        g_free(flp->character); flp->character = NULL;
        purple_notify_user_info_destroy(flp->profile_info); flp->profile_info = NULL;
    } else if(flp->category_table) { /* Try to get the profile through the website API first. */
        GHashTable *args = flist_web_request_args(fla);
        g_hash_table_insert(args, "name", g_strdup(flp->character));
        flp->profile_request = flist_web_request(JSON_CHARACTER_INFO, args, TRUE, fla->secure, flist_get_profile_cb, fla);
        g_hash_table_destroy(args);
    } else { /* Try to get the profile through F-Chat. */
        JsonObject *json = json_object_new();
        json_object_set_string_member(json, "character", flp->character);
        flist_request(pc, "PRO", json);
        json_object_unref(json);
    }
    g_free(link);
}

void flist_profile_process_flood(FListAccount *fla, const gchar *message) {
    FListProfiles *flp = _flist_profiles(fla);
    if(flp->profile_info && flp->character) {
        purple_notify_user_info_add_pair(flp->profile_info, "Error", message);
        purple_notify_userinfo(fla->pc, flp->character, flp->profile_info, NULL, NULL);
        g_free(flp->character); flp->character = NULL;
        purple_notify_user_info_destroy(flp->profile_info); flp->profile_info = NULL;
    }
}

static void flist_global_profile_cb(FListWebRequestData *req_data,
        gpointer user_data, JsonObject *root, const gchar *error_message) {
    FListAccount *fla = user_data;
    FListProfiles *flp = _flist_profiles(fla);
    JsonObject *info;
    GList *categories, *cur;

    flp->global_profile_request = NULL;
    
    if(!root) {
        purple_debug_warning(FLIST_DEBUG, "Failed to obtain the global list of profile fields. Error Message: %s\n", error_message);
        return;
    }

    info = json_object_get_object_member(root, "info");
    if(!info) {
        purple_debug_warning(FLIST_DEBUG, "We received the global list of profile fields, but it was empty.\n");
        return;
    }
    
    purple_debug_info(FLIST_DEBUG, "Processing global profile fields...\n");
    
    flp->category_table = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
    
    categories = json_object_get_members(info);
    cur = categories;
    while(cur) {
        const gchar *group_name;
        JsonObject *field_group;
        JsonArray *field_array;
        FListProfileFieldCategory *category;
        guint i, len;
        
        field_group = json_object_get_object_member(info, cur->data);
        group_name = json_object_get_string_member(field_group, "group");
        field_array = json_object_get_array_member(field_group, "items");
        
        category = g_new0(FListProfileFieldCategory, 1);
        category->name = g_strdup(group_name);

        len = json_array_get_length(field_array);
        for(i = 0; i < len; i++) {
            JsonObject *field_object = json_array_get_object_element(field_array, i);
            FListProfileField *field = g_new0(FListProfileField, 1);
            field->category = category;
            field->fieldid = g_strdup_printf("%d", (gint) json_object_get_int_member(field_object, "id"));
            field->name = g_strdup(json_object_get_string_member(field_object, "name"));
            category->fields = g_slist_prepend(category->fields, field);
            if(fla->debug_mode) {
                purple_debug_info(FLIST_DEBUG, 
                        "Global profile field processed. (ID: %s) (Category: %s) (Name: %s)\n", 
                        field->fieldid, field->category->name, field->name);
            }
        }
        category->fields = g_slist_sort(category->fields, (GCompareFunc) flist_profile_field_cmp);
        flp->category_list = g_slist_append(flp->category_list, category);
        g_hash_table_insert(flp->category_table, category->name, category);
        cur = g_list_next(cur);
    }
    g_list_free(categories);
    
    purple_debug_info(FLIST_DEBUG, "Global profile fields processed. (Categories: %d)\n", g_hash_table_size(flp->category_table));
}

void flist_profile_load(PurpleConnection *pc) {
    FListAccount *fla = pc->proto_data;
    FListProfiles *flp;
    GSList *priority = NULL;
    
    fla->flist_profiles = g_new0(FListProfiles, 1);
    flp = _flist_profiles(fla);
    
    flp->global_profile_request = flist_web_request(JSON_INFO_LIST, NULL, TRUE, fla->secure, flist_global_profile_cb, fla);

    FListProfileField *field;
    
    //TODO: this is really, really ugly
    field = g_new0(FListProfileField, 1);
    field->name = g_strdup("Orientation"); field->fieldid = g_strdup("2");
    priority = g_slist_prepend(priority, field);

    field = g_new0(FListProfileField, 1);
    field->name = g_strdup("Species"); field->fieldid = g_strdup("9");
    priority = g_slist_prepend(priority, field);

    field = g_new0(FListProfileField, 1);
    field->name = g_strdup("Dom/Sub Role"); field->fieldid = g_strdup("15");
    priority = g_slist_prepend(priority, field);

    priority = g_slist_reverse(priority);

    flp->priority_profile_fields = priority;
}

void flist_profile_unload(PurpleConnection *pc) {
    FListAccount *fla = pc->proto_data;
    FListProfiles *flp = _flist_profiles(fla);

    if(flp->global_profile_request) {
        flist_web_request_cancel(flp->global_profile_request);
        flp->global_profile_request = NULL;
    }
    
    if(flp->priority_profile_fields) {
        g_slist_free(flp->priority_profile_fields);
        flp->priority_profile_fields = NULL;
    }

    if(flp->category_list) {
        g_slist_free(flp->category_list);
        flp->category_list = NULL;
    }
    if(flp->category_table) {
        g_hash_table_destroy(flp->category_table);
        flp->category_table = NULL;
    }
    
    if(flp->profile_request) {
        flist_web_request_cancel(flp->profile_request);
        flp->profile_request = NULL;
    }
    if(flp->character) {
        g_free(flp->character);
        flp->character = NULL;
    }
    if(flp->profile_info) {
        purple_notify_user_info_destroy(flp->profile_info);
        flp->profile_info = NULL;
    }
    if(flp->table) {
        g_hash_table_destroy(flp->table);
        flp->table = NULL;
    }
    if(flp->profile_request) flp->profile_request = NULL;
    
    g_free(flp);
    //TODO: lots of memory leaks here to cleanup?
}
