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

gboolean flist_process_RTB(PurpleConnection *pc, JsonObject *root) {
    FListAccount *fla = pc->proto_data;
    const gchar *sender = NULL, *subject = NULL, *name = NULL, *title = NULL;
    gchar *message = NULL;
    gint id;  gboolean has_id;
    gint target_id; gboolean has_target_id;
    const gchar *type = json_object_get_string_member(root, "type");
    GString *message_str;
   
    purple_debug_info(FLIST_DEBUG, "Processing RTB... (Character: %s) (Type: %s)\n", fla->character, type);
 
    if(flist_str_equal(type, "friendrequest")) {
        flist_friends_received_request(fla);
        return TRUE;
    }
    else if(flist_str_equal(type, "friendadd")) {
        flist_friends_added_friend(fla);
        return TRUE;
    }
    else if(flist_str_equal(type, "friendremove")) {
        flist_friends_removed_friend(fla);
        return TRUE;
    }
    
    /* Past this point, we're going to build a message. */
    message_str = g_string_new(NULL);
    
    if(flist_str_equal(type, "note")) {
        sender = json_object_get_string_member(root, "sender");
        subject = json_object_get_string_member(root, "subject");
        id = json_object_get_parse_int_member(root, "id", &has_id);
        g_string_append_printf(message_str,"New note received from %s: %s",sender,subject);
        g_string_append_printf(message_str, " (<a href=\"http://www.f-list.net/view_note.php?note_id=%d\">Open Note</a>)", id);
        message = g_string_free(message_str, FALSE);
        serv_got_im(pc, GLOBAL_NAME, message, PURPLE_MESSAGE_RECV, time(NULL));
    }
    else if(flist_str_equal(type, "bugreport")) {
        name = json_object_get_string_member(root, "name");
        title = json_object_get_string_member(root, "title");
        id = json_object_get_parse_int_member(root, "id", &has_id);
        g_string_append_printf(message_str,"%s submitted a bugreport, \"<a href=\"http://www.f-list.net/view_bugreport.php?id=%d\">%s</a>\"",name,id,title);
        message = g_string_free(message_str, FALSE);
        serv_got_im(pc, GLOBAL_NAME, message, PURPLE_MESSAGE_RECV, time(NULL));
    }
    else if(flist_str_equal(type, "helpdeskticket")) {
        name = json_object_get_string_member(root, "name");
        title = json_object_get_string_member(root, "title");
        id = json_object_get_parse_int_member(root, "id", &has_id);
        g_string_append_printf(message_str,"%s submitted a helpdesk ticket, \"<a href=\"http://www.f-list.net/view_ticket.php?id=%d\">%s</a>\"",name,id,title);
        message = g_string_free(message_str, FALSE);
        serv_got_im(pc, GLOBAL_NAME, message, PURPLE_MESSAGE_RECV, time(NULL));
    }
    else if(flist_str_equal(type, "ticketcreate")) {
        name = json_object_get_string_member(root, "user");
        title = json_object_get_string_member(root, "subject");
        id = json_object_get_parse_int_member(root, "id", &has_id);
        g_string_append_printf(message_str,"%s submitted a helpdesk ticket via email, \"<a href=\"http://www.f-list.net/view_ticket.php?id=%d\">%s</a>\"",name,id,title);
        message = g_string_free(message_str, FALSE);
        serv_got_im(pc, GLOBAL_NAME, message, PURPLE_MESSAGE_RECV, time(NULL));
    }
    else if(flist_str_equal(type, "helpdeskreply")) {
        name = json_object_get_string_member(root, "name");
        title = json_object_get_string_member(root, "title");
        id = json_object_get_parse_int_member(root, "id", &has_id);
        g_string_append_printf(message_str,"%s submitted a reply to your helpdesk ticket, \"<a href=\"http://www.f-list.net/view_ticket.php?id=%d\">%s</a>\"",name,id,title);
        message = g_string_free(message_str, FALSE);
        serv_got_im(pc, GLOBAL_NAME, message, PURPLE_MESSAGE_RECV, time(NULL));
    }
    else if(flist_str_equal(type, "featurerequest")) {
        name = json_object_get_string_member(root, "name");
        title = json_object_get_string_member(root, "title");
        id = json_object_get_parse_int_member(root, "id", &has_id);
        g_string_append_printf(message_str,"%s submitted a feature request, \"<a href=\"http://www.f-list.net/vote.php?fid=%d\">%s</a>\"",name,id,title);
        message = g_string_free(message_str, FALSE);
        serv_got_im(pc, GLOBAL_NAME, message, PURPLE_MESSAGE_RECV, time(NULL));
    }
    else if(flist_str_equal(type, "comment")) {
        name = json_object_get_string_member(root, "name");
        title = json_object_get_string_member(root, "target");
        subject = json_object_get_string_member(root, "target_type");
        id = json_object_get_parse_int_member(root, "id", &has_id);
        target_id = json_object_get_parse_int_member(root, "target_id", &has_target_id);
        if (has_id && has_target_id) {
            g_string_append_printf(message_str,"%s submitted a comment on the newspost \"<a href=\"http://www.f-list.net/%s/%d/#Comment%d\">%s</a>\"",name,subject,target_id,id,title);
            message = g_string_free(message_str, FALSE);
            serv_got_im(pc, GLOBAL_NAME, message, PURPLE_MESSAGE_RECV, time(NULL));
        } else {
            message = g_string_free(message_str, FALSE);
        }
    }
    else {
        /* We'll silently ignore this for now. */
        message = g_string_free(message_str, FALSE);
    }
 
    g_free(message);
    return TRUE;
}