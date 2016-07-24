#include "f-list_ignore.h"

void flist_ignore_list_request_add(FListAccount *fla, const gchar *character)
{
    JsonObject *json = json_object_new();
    json_object_set_string_member(json, "action", "add");
    json_object_set_string_member(json, "character", character);
    flist_request(fla, FLIST_IGNORE, json);
    json_object_unref(json);
}

void flist_ignore_list_request_remove(FListAccount *fla, const gchar *character)
{
    JsonObject *json = json_object_new();
    json_object_set_string_member(json, "action", "delete");
    json_object_set_string_member(json, "character", character);
    flist_request(fla, FLIST_IGNORE, json);
    json_object_unref(json);
}

void flist_ignore_list_request_list(FListAccount *fla)
{
    JsonObject *json = json_object_new();
    json_object_set_string_member(json, "action", "list");
    flist_request(fla, FLIST_IGNORE, json);
    json_object_unref(json);
}

void flist_ignore_list_action(PurplePluginAction *action) {
    PurpleConnection *pc = action->context;
    g_return_if_fail(pc);
    FListAccount *fla = pc->proto_data;
    flist_ignore_list_request_list(fla);
}

static void ignore_list_remove_cb(PurpleConnection *pc, GList *row, gpointer user_data)
{
    FListAccount *fla = pc->proto_data;
    flist_ignore_list_request_remove(fla, g_list_nth_data(row, 0));
}

gboolean flist_ignore_character_is_ignored(FListAccount *fla, const gchar *character)
{
    return g_list_find_custom(fla->ignore_list, character, (GCompareFunc)g_ascii_strcasecmp) != NULL;
}

void flist_blist_node_ignore_action(PurpleBlistNode *node, gpointer data) {
    PurpleBuddy *b = (PurpleBuddy*) node;
    PurpleAccount *pa = purple_buddy_get_account(b);
    PurpleConnection *pc;
    FListAccount *fla;
    FListIgnoreActionType type = GPOINTER_TO_INT(data);
    const gchar *name = purple_buddy_get_name(b);

    g_return_if_fail((pc = purple_account_get_connection(pa)));
    g_return_if_fail((fla = pc->proto_data));

    if (type == FLIST_NODE_IGNORE)
    {
        flist_ignore_list_request_add(fla, name);
    }
    else if (type == FLIST_NODE_UNIGNORE)
    {
        flist_ignore_list_request_remove(fla, name);
    }
}

gboolean flist_process_IGN(FListAccount *fla, JsonObject *root) {
    const gchar *action;
    const gchar *character;

    action = json_object_get_string_member(root, "action");

    if (g_ascii_strncasecmp(action, "add", 3) == 0)
    {
        char msg[128];
        character = json_object_get_string_member(root, "character");
        fla->ignore_list = g_list_append(fla->ignore_list, g_strdup(character));
        g_snprintf( msg, sizeof( msg ), _( "'%s' has been added to your ignore list." ), character );
        purple_notify_info(fla->pc, "User ignored!", msg, NULL);
    }
    else if (g_ascii_strncasecmp(action, "delete", 6) == 0)
    {
        char msg[128];
        character = json_object_get_string_member(root, "character");
        GList *ignored_char = g_list_find_custom(fla->ignore_list, character, (GCompareFunc)g_ascii_strcasecmp);
        
        if (ignored_char != NULL)
        {
            g_free(ignored_char->data);
            fla->ignore_list = g_list_delete_link(fla->ignore_list, ignored_char);
        }

        g_snprintf( msg, sizeof( msg ), _( "'%s' has been removed from your ignore list." ), character );
        purple_notify_info(fla->pc, "User removed!", msg, NULL);
    }
    else if (g_ascii_strncasecmp(action, "list", 4) == 0)
    {
        // Response looks like
        //  IGN {"characters":["asdasdasdasdasdf343ddfsd","testytest","blablabla"],"action":"list"}
        
        JsonArray *chars = json_object_get_array_member(root, "characters");
        PurpleNotifySearchResults *results = purple_notify_searchresults_new();
        PurpleNotifySearchColumn *column = purple_notify_searchresults_column_new("Character");
        purple_notify_searchresults_column_add(results, column);

        for (size_t i = 0; i < json_array_get_length(chars); i++)
        {
            GList *row = NULL;
            row = g_list_append(row, (gpointer) g_strdup(json_array_get_string_element(chars, i)));
            purple_notify_searchresults_row_add(results, row);
        }

        // XXX This button is ignored, might be a bug in libpurple/pidgin
	    purple_notify_searchresults_button_add_labeled(results, "Remove", ignore_list_remove_cb);
        purple_notify_searchresults(fla->pc, NULL, NULL, _("Ignore List"), results, NULL, NULL);
    }
    else if (g_ascii_strncasecmp(action, "init", 4) == 0)
    {
        flist_g_list_free_full(fla->ignore_list, g_free);
        fla->ignore_list = NULL;

        JsonArray *chars = json_object_get_array_member(root, "characters");

        for (size_t i = 0; i < json_array_get_length(chars); i++)
        {
            fla->ignore_list = g_list_append(fla->ignore_list, g_strdup(json_array_get_string_element(chars, i)));
        }
    }

    return TRUE;
}

PurpleCmdRet flist_ignore_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data) {
    FListAccount *fla = flist_get_account_from_conversation(convo);
    g_return_val_if_fail(fla, PURPLE_CMD_RET_FAILED);
    
    if (args[0] == NULL) {
        *error = g_strdup("Please specify an action : /ignore [add|delete|list] [character]");
        return PURPLE_CMD_RET_FAILED;
    }

    gchar *subcmd = args[0];
    if (g_ascii_strncasecmp(subcmd, "add", 3) == 0)
    {
        if (args[1] == NULL) {
            *error = g_strdup("You must provide a character name to ignore");
            return PURPLE_CMD_RET_FAILED;
        }

        flist_ignore_list_request_add(fla, args[1]);
    }
    else if (g_ascii_strncasecmp(subcmd, "delete", 6) == 0)
    {
        if (args[1] == NULL) {
            *error = g_strdup("You must provide a nickname to unignore");
            return PURPLE_CMD_RET_FAILED;
        }

        flist_ignore_list_request_remove(fla, args[1]);
    }
    else if (g_ascii_strncasecmp(subcmd, "list", 4) == 0)
    {
        flist_ignore_list_request_list(fla);
    } else {
        *error = g_strdup("Unrecognized action, please use : /ignore [add|delete|list] [character]");
        return PURPLE_CMD_RET_FAILED;
    }

    return PURPLE_CMD_RET_OK;
}

