#ifndef FLIST_IGNORE_H
#define FLIST_IGNORE_H

#include "f-list.h"

gboolean flist_process_IGN(FListAccount *fla, JsonObject *root);
PurpleCmdRet flist_ignore_cmd(PurpleConversation *convo, const gchar *cmd, gchar **args, gchar **error, void *data);

void flist_ignore_list_add(FListAccount *fla, const gchar *character);
void flist_ignore_list_remove(FListAccount *fla, const gchar *character);
void flist_ignore_list_request_list(FListAccount *fla);

void flist_ignore_list_action(PurplePluginAction *action);

gboolean flist_ignore_character_is_ignored(FListAccount *fla, const gchar *character);

void flist_blist_node_ignore_action(PurpleBlistNode *node, gpointer data);

#endif // FLIST_IGNORE_H
