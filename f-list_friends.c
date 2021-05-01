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

#include "f-list_friends.h"

#define FLIST_FRIENDS_SYNC_TIMEOUT 1800

#define ERROR_CANNOT_BOOKMARK "This user doesn't want to be bookmarked."

typedef struct FListFriendsRequest_ FListFriendsRequest;

struct FListFriends_ {
    FListFriendsRequest* update_request;
    guint update_timer;
    gboolean update_timer_active;

    gboolean friends_dirty, bookmarks_dirty, incoming_requests_dirty, outgoing_requests_dirty;

    GList *requests;
    GHashTable *friends;
    GHashTable *cannot_bookmark;
    GList *auth_requests;
};

typedef struct FListFriend_ {
    gchar *name;
    FListFriendStatus status;
    gint code;
    gboolean bookmarked;
} FListFriend;

typedef struct FListFriendAuth_ {
    FListAccount *fla;
    gchar *name;
    void *handle;
} FListFriendAuth;

struct FListFriendsRequest_ {
    FListAccount *fla;
    FListWebRequestData *req_data;
    FListFriendsRequestType type;
    gchar *character;
    gint code;
    gboolean automatic;
};

static void flist_friend_free(gpointer data) {
    FListFriend *f = data;
    g_free(f->name);
    g_free(f);
}

static inline FListFriends *_flist_friends(FListAccount *fla) {
    return fla->flist_friends;
}
static void flist_friends_sync_timer(FListAccount *fla, guint32 timeout);

static void flist_friends_request_delete(FListFriendsRequest *req) {
    if(req->character) g_free(req->character);
    g_free(req);
}

static gint flist_friend_auth_find_name(FListFriendAuth *a, gchar *name) {
    return !flist_str_equal(a->name, name);
}

static void flist_friends_request_cancel(FListFriendsRequest *req) {
    flist_web_request_cancel(req->req_data);
    flist_friends_request_delete(req);
}

static void _delete_request(FListFriends *flf, FListFriendsRequest *data) {
    GList *cur = flf->requests;
    while(cur) {
        if(cur->data == data) {
            flf->requests = g_list_delete_link(flf->requests, cur);
            return;
        }
        cur = cur->next;
    }
}

static void _clear_requests(FListFriends *flf) {
    while(flf->requests) {
        flist_friends_request_cancel((FListFriendsRequest*) flf->requests->data);
        flf->requests = g_list_delete_link(flf->requests, flf->requests);
    }
    while(flf->auth_requests) {
        FListFriendAuth *req = flf->auth_requests->data;
        purple_account_request_close(req->handle);
        g_free(req->name);
        g_free(req);
        flf->auth_requests = g_list_delete_link(flf->auth_requests, flf->auth_requests);
    }
}

static void _clear_updates(FListFriends *flf) {
    if(flf->update_timer_active) {
        flf->update_timer_active = FALSE;
        purple_timeout_remove(flf->update_timer);
    }
    if(flf->update_request) {
        flist_friends_request_cancel((FListFriendsRequest*) flf->update_request);
        flf->update_request = NULL;
    }
}

gboolean flist_friends_is_bookmarked(FListAccount* fla, const gchar* character) {
    FListFriends *flf = _flist_friends(fla);
    FListFriend *friend;

    friend = g_hash_table_lookup(flf->friends, character);
    if(!friend) return FALSE;
    return friend->bookmarked;
}
FListFriendStatus flist_friends_get_friend_status(FListAccount* fla, const gchar* character) {
    FListFriends *flf = _flist_friends(fla);
    FListFriend *friend;
    if(!flf->friends) return FLIST_FRIEND_STATUS_UNKNOWN;

    friend = g_hash_table_lookup(flf->friends, character);
    if(!friend) return FLIST_NOT_FRIEND;
    return friend->status;
}

static void flist_friends_action_cb(FListWebRequestData* req_data, gpointer user_data,
        JsonObject *root, const gchar *error_message) {
    FListFriendsRequest *req = user_data;
    FListAccount *fla = req->fla;
    FListFriends *flf = _flist_friends(fla);
    const gchar *error;
    gchar *primary_error = NULL, *secondary_error = NULL;
    gboolean success = TRUE;

    if(!root) {
        purple_debug_warning(FLIST_DEBUG, "We failed our friends action. Error Message: %s\n", error_message);
        primary_error = g_strdup("Your friends request failed.");
        secondary_error = g_strdup(error_message);
        success = FALSE;
    } else {
        error = json_object_get_string_member(root, "error");
        if(error && strlen(error)) {
            if(!strcmp(error, ERROR_CANNOT_BOOKMARK)) {
                purple_debug_warning(FLIST_DEBUG, "We are unable to bookmark a character. Error Message: %s\n", error);
                success = FALSE;
            } else {
                purple_debug_warning(FLIST_DEBUG, "The server returned an unknown error for our friends action. Error Message: %s\n", error);
                success = FALSE;
            }
            primary_error = g_strdup("Your friends request returned an error.");
            secondary_error = g_strdup(error);
        }
    }

    if(!success && req->type == FLIST_BOOKMARK_ADD) {
        gchar *character = g_strdup(req->character);
        g_hash_table_insert(flf->cannot_bookmark, character, character);
    }

    if(!success && primary_error && secondary_error) {
        if(!(req->automatic)) {
            purple_notify_warning(fla->pc, "F-List Friends Request", primary_error, secondary_error);
        }
    }
    if(primary_error) g_free(primary_error);
    if(secondary_error) g_free(secondary_error);

    _delete_request(flf, req);
    flist_friends_request_delete(req);

    if(!flf->requests) {
        flist_friends_sync_timer(fla, 0);
    }
}

gboolean flist_friend_action(FListAccount *fla, const gchar *name, FListFriendsRequestType type, gboolean automatic) {
    FListFriends *flf = _flist_friends(fla);
    GHashTable *args = flist_web_request_args(fla);
    FListFriend *friend = g_hash_table_lookup(flf->friends, name);
    FListFriendsRequest *req = g_new0(FListFriendsRequest, 1);

    req->fla = fla;
    req->character = g_strdup(name);
    req->type = type;
    req->automatic = automatic;

    switch(type) {
    case FLIST_FRIEND_REQUEST:
        g_hash_table_insert(args, "source_name", g_strdup(fla->character));
        g_hash_table_insert(args, "dest_name", g_strdup(name));
        req->req_data = flist_web_request(JSON_FRIENDS_REQUEST, args, NULL, TRUE, flist_friends_action_cb, req);
        flf->outgoing_requests_dirty = TRUE;
        break;
    case FLIST_FRIEND_REMOVE:
        g_hash_table_insert(args, "source_name", g_strdup(fla->character));
        g_hash_table_insert(args, "dest_name", g_strdup(name));
        req->req_data = flist_web_request(JSON_FRIENDS_REMOVE, args, NULL, TRUE, flist_friends_action_cb, req);
        flf->friends_dirty = TRUE;
        break;
    case FLIST_FRIEND_AUTHORIZE:
        if(!friend) break;
        g_hash_table_insert(args, "request_id", g_strdup_printf("%d", friend->code));
        req->code = friend->code;
        req->req_data = flist_web_request(JSON_FRIENDS_ACCEPT, args, NULL, TRUE, flist_friends_action_cb, req);
        flf->incoming_requests_dirty = TRUE;
        flf->friends_dirty = TRUE;
        break;
    case FLIST_FRIEND_DENY:
        if(!friend) break;
        g_hash_table_insert(args, "request_id", g_strdup_printf("%d", friend->code));
        req->code = friend->code;
        req->req_data = flist_web_request(JSON_FRIENDS_DENY, args, NULL, TRUE, flist_friends_action_cb, req);
        flf->incoming_requests_dirty = TRUE;
        break;
    case FLIST_FRIEND_CANCEL:
        if(!friend) break;
        g_hash_table_insert(args, "request_id", g_strdup_printf("%d", friend->code));
        req->code = friend->code;
        req->req_data = flist_web_request(JSON_FRIENDS_CANCEL, args, NULL, TRUE, flist_friends_action_cb, req);
        flf->outgoing_requests_dirty = TRUE;
        break;
    case FLIST_BOOKMARK_ADD:
        if(g_hash_table_lookup(flf->cannot_bookmark, name)) break; /* We cannot bookmark some users. Move on. */
        g_hash_table_insert(args, "name", g_strdup(name));
        req->req_data = flist_web_request(JSON_BOOKMARK_ADD, args, NULL, TRUE, flist_friends_action_cb, req);
        flf->bookmarks_dirty = TRUE;
        break;
    case FLIST_BOOKMARK_REMOVE:
        g_hash_table_insert(args, "name", g_strdup(name));
        req->req_data = flist_web_request(JSON_BOOKMARK_REMOVE, args, NULL, TRUE, flist_friends_action_cb, req);
        flf->bookmarks_dirty = TRUE;
        break;
    default: break;
    }

    g_hash_table_destroy(args);
    if(req->req_data) {
        _clear_updates(flf);
        flf->requests = g_list_prepend(flf->requests, req);
        return TRUE;
    } else {
        flist_friends_request_delete(req);
        return FALSE;
    }
}

static void flist_friends_refresh(FListAccount *fla) {
    FListFriends *flf = _flist_friends(fla);
    if(flf->requests) return; //We can't refresh until the requests finish.
    flist_friends_sync_timer(fla, 0);
}

void flist_friends_received_request(FListAccount *fla) {
    FListFriends *flf = _flist_friends(fla);
    flf->incoming_requests_dirty = TRUE;
    flist_friends_refresh(fla);
}
void flist_friends_added_friend(FListAccount *fla) {
    FListFriends *flf = _flist_friends(fla);
    flf->outgoing_requests_dirty = TRUE;
    flf->friends_dirty = TRUE;
    flist_friends_refresh(fla);
}
void flist_friends_removed_friend(FListAccount *fla) {
    FListFriends *flf = _flist_friends(fla);
    flf->friends_dirty = TRUE;
    flist_friends_refresh(fla);
}

void flist_bookmarks_updated(FListAccount *fla) {
    FListFriends *flf = _flist_friends(fla);
    flf->bookmarks_dirty = TRUE;
    flist_friends_refresh(fla);
}

void flist_blist_node_action(PurpleBlistNode *node, gpointer data) {
    PurpleBuddy *b = (PurpleBuddy*) node;
    PurpleAccount *pa = purple_buddy_get_account(b);
    PurpleConnection *pc;
    FListAccount *fla;
    FListFriendsRequestType type = GPOINTER_TO_INT(data);
    const gchar *name = purple_buddy_get_name(b);

    g_return_if_fail((pc = purple_account_get_connection(pa)));
    g_return_if_fail((fla = purple_connection_get_protocol_data(pc)));

    flist_friend_action(fla, name, type, FALSE);
}

static void flist_auth_accept_cb(gpointer user_data) {
    FListFriendAuth *auth = user_data;
    FListAccount *fla = auth->fla;
    FListFriends *flf = _flist_friends(fla);
    FListFriend *friend;

    flf->auth_requests = g_list_remove(flf->auth_requests, auth);
    friend = g_hash_table_lookup(flf->friends, auth->name);

    if(friend) {
        flist_friend_action(fla, auth->name, FLIST_FRIEND_AUTHORIZE, FALSE);
    }

    g_free(auth->name);
    g_free(auth);
}

static void flist_auth_deny_cb(gpointer user_data) {
    FListFriendAuth *auth = user_data;
    FListAccount *fla = auth->fla;
    FListFriends *flf = _flist_friends(fla);
    FListFriend *friend;

    flf->auth_requests = g_list_remove(flf->auth_requests, auth);
    friend = g_hash_table_lookup(flf->friends, auth->name);

    if(friend) {
        flist_friend_action(fla, auth->name, FLIST_FRIEND_DENY, FALSE);
    }

    g_free(auth->name);
    g_free(auth);
}

static gboolean _unused_friend(gchar *key, FListFriend *value, gpointer *user_data){
    if (!value->bookmarked && (value->status == FLIST_NOT_FRIEND)){
        return TRUE;
    } else {
        return FALSE;
    }
}

static void flist_cleanup_friends(FListFriends *flf){
    g_return_if_fail(flf);
    GHashTable *friends = flf->friends;
    g_return_if_fail(friends);

    g_hash_table_foreach_remove(friends, (GHRFunc)_unused_friend, NULL);
}

/*
 * This functionality should be disabled by default and enabled via a checkbox in account options.
 * Some people, like me, use non-friend non-bookmark contacts to keep track of people we're RPing with
 * but that we don't have friended due to clutter/preference/etc. Failing it being optional, it should
 * move contacts from the 'CharacterName - Friends' section of the buddy list into a
 * 'CharacterName - Contacts' section or the like to allow the user to remove if they desire.
 */
static void flist_friends_remove_former(FListFriends *flf, PurpleGroup *group) {
    // Clean up friends and bookmark groups
	/*
    PurpleBlistNode *iter = purple_blist_node_get_first_child(PURPLE_BLIST_NODE(group));
    PurpleBuddy *buddy;

    // We need to build a list, because removing a node from the buddy list
    // while traversing it leads to funny things
    GList *removelist = NULL;
    GList *cur = NULL;

    while(iter) {
        buddy = NULL;
        if (PURPLE_BLIST_NODE_IS_BUDDY(iter)) {
            buddy = PURPLE_BUDDY(iter);
        } else if (PURPLE_BLIST_NODE_IS_CONTACT(iter)) {
            buddy = purple_contact_get_priority_buddy(PURPLE_CONTACT(iter));
        }

        if (buddy) {
            // If the buddy's name isn't a known FList friend or bookmark, remember him
            if (g_hash_table_lookup(flf->friends, purple_buddy_get_name(buddy)) == NULL) {
               removelist =  g_list_append(removelist, buddy);
            }
        }
        iter = purple_blist_node_get_sibling_next(iter);
    }

    for (cur = removelist; cur; cur = cur->next) {
        PurpleBuddy *buddy = cur->data;
        purple_debug_info(FLIST_DEBUG, "Removing former contact %s from the buddy list\n",
                purple_buddy_get_name(buddy));

        purple_blist_remove_buddy(buddy);
    }
    g_list_free(removelist);
    */
}


/* This is the callback after every friends response from the server.
 * We sync the Pidgin friends list with what we received.
 */
static void flist_friends_add_buddies(FListAccount *fla) {
    FListFriends *flf = _flist_friends(fla);
    PurpleGroup *f_group, *b_group;
    GList *friends, *cur;


    // Add all f-list friends as pidgin buddies
    f_group = flist_get_friends_group(fla);
    b_group = flist_get_bookmarks_group(fla);
    friends = g_hash_table_get_values(flf->friends);
    cur = friends;

    while(cur) {
        FListFriend *friend = (FListFriend *) cur->data;
        PurpleBuddy *buddy = purple_find_buddy(fla->pa, friend->name);
        gboolean bookmarked = fla->sync_bookmarks && friend->bookmarked;
        gboolean friended = fla->sync_friends && friend->status != FLIST_NOT_FRIEND;
        gboolean requests_auth = friend->status == FLIST_PENDING_IN_FRIEND;

        /* Check to make sure we don't already have a pending authorization request */
        if(requests_auth && g_list_find_custom(
                    flf->auth_requests, friend->name,
                    (GCompareFunc) flist_friend_auth_find_name)) {
            requests_auth = FALSE;
        }

        /* If this is a new request, notify the user! */
        if(requests_auth) {
            FListFriendAuth *auth = g_new0(FListFriendAuth, 1);
            auth->fla = fla;
            auth->name = g_strdup(friend->name);
            auth->handle = purple_account_request_authorization(
                fla->pa, friend->name, NULL, NULL, NULL, buddy || friended || bookmarked,
                flist_auth_accept_cb, flist_auth_deny_cb, auth);
            flf->auth_requests = g_list_append(flf->auth_requests, auth);
        }

        if(buddy) {
            // We already know this buddy, make sure it's not in the wrong group

            if (friended) {
                if (purple_buddy_get_group(buddy) == b_group){
                    purple_blist_add_buddy(buddy, NULL, f_group, NULL);
                }
            } else if (bookmarked) {
                if (purple_buddy_get_group(buddy) == f_group) {
                    purple_blist_add_buddy(buddy, NULL, b_group, NULL);
                }
            }
        } else {
            // Add this buddy if we didn't know it or if it wasn't in the right group
            if (friended) {
                buddy = purple_buddy_new(fla->pa, friend->name, NULL);
                purple_blist_add_buddy(buddy, NULL, f_group, NULL);
                flist_update_friend(fla, friend->name, TRUE, TRUE);
            } else if (bookmarked) {
                buddy = purple_buddy_new(fla->pa, friend->name, NULL);
                purple_blist_add_buddy(buddy, NULL, b_group, NULL);
                flist_update_friend(fla, friend->name, TRUE, TRUE);
            }
        }
        cur = cur->next;
    }

    g_list_free(friends);

    // Remove people who are no longer friends or bookmarks from their groups
    flist_friends_remove_former(flf, f_group);
    flist_friends_remove_former(flf, b_group);
}


static void flist_friends_reset_status(GHashTable *friends, FListFriendStatus status) {
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, friends);
    while(g_hash_table_iter_next(&iter, &key, &value)) {
        FListFriend *friend = value;
        if(friend->status == status) friend->status = FLIST_NOT_FRIEND;
    }
}
static void flist_friends_reset_bookmarks(GHashTable *friends) {
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, friends);
    while(g_hash_table_iter_next(&iter, &key, &value)) {
        FListFriend *friend = value;
        friend->bookmarked = FALSE;
    }
}
static FListFriend* flist_friend_get(GHashTable *friends, const gchar *character) {
    FListFriend *friend = g_hash_table_lookup(friends, character);
    if(!friend) {
        friend = g_new0(FListFriend, 1);
        friend->name = g_strdup(character);
        g_hash_table_replace(friends, friend->name, friend);
        friend->status = FLIST_NOT_FRIEND;
        friend->bookmarked = FALSE;
    }
    return friend;
}

static void flist_handle_friends_list(FListAccount *fla, JsonArray *array, FListFriendStatus status) {
    FListFriends *flf = _flist_friends(fla);
    int index, len;
    gboolean in = (status == FLIST_PENDING_IN_FRIEND);

    flist_friends_reset_status(flf->friends, status);

    len = json_array_get_length(array);
    for(index = 0; index < len; index++) {
        JsonObject *o = json_array_get_object_element(array, index);
        const gchar *source = json_object_get_string_member(o, !in ? "source" : "dest");
        const gchar *dest = json_object_get_string_member(o, !in ? "dest" : "source");

        gint id = 0;
        if (json_object_has_member(o, "id")) {
            id = json_object_get_int_member(o, "id");
        }

        FListFriend *friend;

        //We are only interested in friends of the current character.
        if(purple_utf8_strcasecmp(fla->character, source)) continue;

        friend = flist_friend_get(flf->friends, dest);
        friend->status = status;
        friend->code = id;
    }
}

static void flist_handle_bookmark_list(FListAccount *fla, JsonArray *array) {
    FListFriends *flf = _flist_friends(fla);
    int index, len;

    flist_friends_reset_bookmarks(flf->friends);

    len = json_array_get_length(array);
    for(index = 0; index < len; index++) {
        const gchar *character = json_array_get_string_element(array, index);
        FListFriend *friend = flist_friend_get(flf->friends, character);
        friend->bookmarked = TRUE;
    }
}

static void flist_friends_update_cb(FListWebRequestData *req_data, gpointer user_data,
        JsonObject *root, const gchar *error_message) {
    FListFriendsRequest *req = user_data;
    FListAccount *fla = req->fla;
    FListFriends *flf = _flist_friends(fla);
    const gchar *error;
    gboolean success = TRUE;
    JsonArray *bookmarks, *friends, *requests_in, *requests_out;

    /* We must clear these manually. */
    flist_friends_request_delete(req);
    flf->update_request = NULL;
    /* Might as well start a timer? */
    flist_friends_sync_timer(fla, FLIST_FRIENDS_SYNC_TIMEOUT);

    if(error_message) {
        purple_debug_info(FLIST_DEBUG, "We have failed a friends list request. Error Message: %s\n", error_message);
        success = FALSE;
    } else {
        error = json_object_get_string_member(root, "error");
        if(error && strlen(error)) {
            purple_debug_info(FLIST_DEBUG, "We requested a friends list, but it returned an error. Error Message: %s\n", error);
            success = FALSE;
        }
    }

    if(!success) return; /* We failed to update ... try again later. */

    bookmarks = json_object_get_array_member(root, "bookmarklist");
    friends = json_object_get_array_member(root, "friendlist");
    requests_in = json_object_get_array_member(root, "requestlist"); /* These are the friends requests waiting our approval. */
    requests_out = json_object_get_array_member(root, "requestpending"); /* These are the friends requests we have made. */

    if(bookmarks) {
        flist_handle_bookmark_list(fla, bookmarks);
        flf->bookmarks_dirty = FALSE;
    }
    if(friends) {
        flist_handle_friends_list(fla, friends, FLIST_MUTUAL_FRIEND);
        flf->friends_dirty = FALSE;
    }
    if(requests_in) {
        flist_handle_friends_list(fla, requests_in, FLIST_PENDING_IN_FRIEND);
        flf->incoming_requests_dirty = FALSE;
    }
    if(requests_out) {
        flist_handle_friends_list(fla, requests_out, FLIST_PENDING_OUT_FRIEND);
        flf->outgoing_requests_dirty = FALSE;
    }

    flist_cleanup_friends(flf);
    flist_friends_add_buddies(fla);
}

static gboolean flist_friends_sync_timer_cb(gpointer data) {
    FListAccount *fla = data;
    FListFriends *flf = _flist_friends(fla);
    FListFriendsRequest *req;
    GHashTable *args;

    purple_debug_info(FLIST_DEBUG, "The friends sync timer has gone off.\n");
    if(!flf->friends_dirty && !flf->bookmarks_dirty && !flf->incoming_requests_dirty && !flf->outgoing_requests_dirty) {
        //Nothing to do here ...
        flf->update_timer_active = FALSE;
        return FALSE;
    }

    args = flist_web_request_args(fla);

    /* Decide what we want to request. Don't overdo it or Kira will bite you. */
    if(flf->friends_dirty) {
        g_hash_table_insert(args, "friendlist", g_strdup("true"));
        purple_debug_info(FLIST_DEBUG, "Requesting friendlist.\n");
    }
    if(flf->bookmarks_dirty) {
        g_hash_table_insert(args, "bookmarklist", g_strdup("true"));
        purple_debug_info(FLIST_DEBUG, "Requesting bookmarklist.\n");
    }
    if(flf->incoming_requests_dirty) {
        g_hash_table_insert(args, "requestlist", g_strdup("true"));
        purple_debug_info(FLIST_DEBUG, "Requesting requestlist (incoming friend requests).\n");
    }
    if(flf->outgoing_requests_dirty) {
        g_hash_table_insert(args, "requestpending", g_strdup("true"));
        purple_debug_info(FLIST_DEBUG, "Requesting requestpending (outgoing friend requests).\n");
    }

    /* Request the update. */
    req = g_new0(FListFriendsRequest, 1);
    req->fla = fla; req->automatic = TRUE; req->type = FLIST_FRIENDS_UPDATE;
    req->req_data = flist_web_request(JSON_FRIENDS, args, NULL, TRUE, flist_friends_update_cb, req);
    flf->update_request = req;
    flf->update_timer_active = FALSE;

    g_hash_table_destroy(args);
    return FALSE;
}

static void flist_friends_sync_timer(FListAccount *fla, guint32 timeout) {
    FListFriends *flf = _flist_friends(fla);
    _clear_updates(flf);
    flf->update_timer_active = TRUE;
    flf->update_timer = purple_timeout_add_seconds(timeout, (GSourceFunc) flist_friends_sync_timer_cb, fla);
}

void flist_friends_login(FListAccount *fla) {
    flist_friends_sync_timer(fla, 0);
}

void flist_friends_load(FListAccount *fla) {
    FListFriends *flf;
    fla->flist_friends = g_new0(FListFriends, 1);
    flf = _flist_friends(fla);

    flf->bookmarks_dirty = TRUE;
    flf->friends_dirty = TRUE;
    flf->incoming_requests_dirty = TRUE;
    flf->outgoing_requests_dirty = TRUE;

    flf->friends = g_hash_table_new_full((GHashFunc) flist_str_hash, (GEqualFunc) flist_str_equal, NULL, flist_friend_free);
    flf->cannot_bookmark = g_hash_table_new_full((GHashFunc) flist_str_hash, (GEqualFunc) flist_str_equal, NULL, g_free);
}

void flist_friends_unload(FListAccount *fla) {
    FListFriends *flf = _flist_friends(fla);
    g_return_if_fail(flf);

    if(flf->cannot_bookmark) g_hash_table_destroy(flf->cannot_bookmark);
    if(flf->friends) g_hash_table_destroy(flf->friends);

    /* Cancel all of our pending requests ... */
    _clear_updates(flf);
    _clear_requests(flf);

    g_free(fla->flist_friends);
}

void flist_buddy_add(PurpleConnection *pc, PurpleBuddy *buddy, PurpleGroup *group) {
    PurpleAccount *pa = buddy->account;
    FListAccount *fla = purple_connection_get_protocol_data(pc);
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

    flist_update_friend(fla, identity, TRUE, TRUE);
}

void flist_buddy_remove(PurpleConnection *pc, PurpleBuddy *buddy, PurpleGroup *group) {
    FListAccount *fla = purple_connection_get_protocol_data(pc);
    GList *existing_auth_list;
    const gchar *name = purple_buddy_get_name(buddy);

    g_return_if_fail(fla);

    if(fla->sync_bookmarks && flist_friends_is_bookmarked(fla, name)) {
            purple_debug_warning(FLIST_DEBUG,
                    "Removing F-List bookmark for %s\n", name);
            flist_friend_action(fla, name, FLIST_BOOKMARK_REMOVE, FALSE);
    }

    if (fla->sync_friends) {

        FListFriendStatus friend_status;

        friend_status = flist_friends_get_friend_status(fla, name);

        switch(friend_status) {
            case FLIST_MUTUAL_FRIEND :
                purple_debug_warning(FLIST_DEBUG,
                        "Removing F-List friend %s\n", name);
                flist_friend_action(fla, name, FLIST_FRIEND_REMOVE, FALSE);
                break;
            case FLIST_PENDING_OUT_FRIEND :
                purple_debug_warning(FLIST_DEBUG,
                        "Canceling F-List friend request with %s\n", name);
                flist_friend_action(fla, name, FLIST_FRIEND_CANCEL, FALSE);
                break;
            case FLIST_PENDING_IN_FRIEND :
                // Cancel a pending invitation we might have
                existing_auth_list = g_list_find_custom(
                        _flist_friends(fla)->auth_requests, name,
                        (GCompareFunc) flist_friend_auth_find_name);
                if (existing_auth_list) {
                    FListFriendAuth *auth = existing_auth_list->data;
                    purple_account_request_close(auth->handle);
                }
                purple_debug_warning(FLIST_DEBUG,
                        "Denying F-List friend request with %s\n", name);
                flist_friend_action(fla, name, FLIST_FRIEND_DENY, FALSE);
                break;
            case FLIST_NOT_FRIEND :
            case FLIST_FRIEND_STATUS_UNKNOWN :
            default:
                // Nothing to do
                break;
        }
    }
}

