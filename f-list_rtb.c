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
#include "f-list_rtb.h"

gchar *flist_rtb_get_comment_url(const gchar *type, gint target_id, gint comment_id)
{
    gchar *url = NULL;

    if (flist_str_equal(type, "newspost"))
        url = g_strdup_printf(FLIST_URL_NEWSPOST_COMMENT, target_id, comment_id);
    else if(flist_str_equal(type, "bugreport"))
        url = g_strdup_printf(FLIST_URL_BUGREPORT_COMMENT, target_id, comment_id);
    else if(flist_str_equal(type, "changelog"))
        url = g_strdup_printf(FLIST_URL_CHANGELOG_COMMENT, target_id, comment_id);
    else if(flist_str_equal(type, "feature"))
        url = g_strdup_printf(FLIST_URL_FEATURE_COMMENT, target_id, comment_id);

    return url;
}

RTB_TYPE flist_rtb_get_type(const gchar *type_str)
{
    RTB_TYPE type = None;

    if(flist_str_equal(type_str, "friendrequest"))
        type = FriendRequest;
    else if(flist_str_equal(type_str, "friendadd"))
        type = FriendAdd;
    else if(flist_str_equal(type_str, "friendremove"))
        type = FriendRemove;
    else if(flist_str_equal(type_str, "note"))
        type = Note;
    else if(flist_str_equal(type_str, "bugreport"))
        type = BugReport;
    else if(flist_str_equal(type_str, "helpdeskticket"))
        type = HelpdeskTicket;
    else if(flist_str_equal(type_str, "ticketcreate"))
        type = TicketCreate;
    else if(flist_str_equal(type_str, "helpdeskreply"))
        type = HelpdeskReply;
    else if(flist_str_equal(type_str, "featurerequest"))
        type = FeatureRequest;
    else if(flist_str_equal(type_str, "comment"))
        type = Comment;

    return type;
}

gboolean flist_process_RTB(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    const gchar *subject = NULL, *name = NULL, *title = NULL;
    gchar *url = NULL;
    gchar *msg = NULL;
    gint id;  gboolean has_id;
    gint target_id; gboolean has_target_id;
    const gchar *type_str = json_object_get_string_member(root, "type");

    /* Ignore notification if the user doesn't want to see it */
    if (!fla->receive_rtb) {
        return TRUE;
    }

    purple_debug_info(FLIST_DEBUG, "Processing RTB... (Character: %s, Type: %s)\n", fla->character, type_str);

    RTB_TYPE type = flist_rtb_get_type(type_str);

    if (type == None)
    {
        purple_debug_error(FLIST_DEBUG, "Error parsing RTB: Unknown type (Character: %s, Type: %s)\n", fla->character, type_str);
        return TRUE;
    }

    // Handle friend list notifications
    switch(type)
    {
        case FriendRequest:
            flist_friends_received_request(fla);
            return TRUE;
        case FriendAdd:
            flist_friends_added_friend(fla);
            return TRUE;
        case FriendRemove:
            flist_friends_removed_friend(fla);
            return TRUE;
    }

    // --- Past this point, we're going to build a message.

    // Fields in some RTB message are named differently, depending on their type
    // Normalize this here
    if (type == TicketCreate)
    {
        name = json_object_get_string_member(root, "user");
        title = json_object_get_string_member(root, "subject");
        id = json_object_get_parse_int_member(root, "id", &has_id);
    }
    else if (type == Note)
    {
        name = json_object_get_string_member(root, "sender");
        title = json_object_get_string_member(root, "subject");
        id = json_object_get_parse_int_member(root, "id", &has_id);
    }
    else if (type == Comment)
    {
        name = json_object_get_string_member(root, "name");
        title = json_object_get_string_member(root, "target");
        id = json_object_get_parse_int_member(root, "id", &has_id);
    }
    else
    {
        name = json_object_get_string_member(root, "name");
        title = json_object_get_string_member(root, "title");
        id = json_object_get_parse_int_member(root, "id", &has_id);
    }

    if (!has_id)
    {
        purple_debug_error(FLIST_DEBUG, "Error parsing RTB: Could not parse id (Character: %s, Type: %s)\n", fla->character, type_str);
        return TRUE;
    }

    switch(type)
    {
        case Note:
            msg = g_strdup_printf("New note received from %s: %s (<a href=\"" FLIST_URL_NOTE "\">Open Note</a>)", name, title, id);
            break;

        case BugReport:
            msg = g_strdup_printf("%s submitted a bugreport, \"<a href=\"" FLIST_URL_BUGREPORT "\">%s</a>\"", name, id, title);
            break;

        case FeatureRequest:
            msg = g_strdup_printf("%s submitted a feature request, \"<a href=\"" FLIST_URL_FEATUREREQUEST "\">%s</a>\"", name, id, title);
            break;

        // Helpdesk Tickets
        case HelpdeskTicket:
            msg = g_strdup_printf("%s submitted a helpdesk ticket, \"<a href=\"" FLIST_URL_HELPDESKTICKET "\">%s</a>\"", name, id, title);
            break;
        case TicketCreate:
            msg = g_strdup_printf("%s submitted a helpdesk ticket via email, \"<a href=\"" FLIST_URL_HELPDESKTICKET "\">%s</a>\"", name, id, title);
            break;
        case HelpdeskReply:
            msg = g_strdup_printf("%s submitted a reply to your helpdesk ticket, \"<a href=\"" FLIST_URL_HELPDESKTICKET "\">%s</a>\"", name, id, title);
            break;

        case Comment:
            subject = json_object_get_string_member(root, "target_type");
            target_id = json_object_get_parse_int_member(root, "target_id", &has_target_id);

            if (!has_target_id)
            {
                purple_debug_error(FLIST_DEBUG, "Error parsing RTB: Could not parse target_id (Character: %s, Type: %s)\n", fla->character, type_str);
                return TRUE;
            }

            url = flist_rtb_get_comment_url(subject, target_id, id);
            msg = g_strdup_printf("%s submitted a comment on the %s \"<a href=\"%s\">%s</a>\"", name, subject, url, title);
            g_free(url);
            break;
    }

    serv_got_im(pc, GLOBAL_NAME, msg, PURPLE_MESSAGE_RECV, time(NULL));
    g_free(msg);

    return TRUE;
}
