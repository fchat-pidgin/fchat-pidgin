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
#ifndef FLIST_RTB_H
#define	FLIST_RTB_H

#include "f-list.h"

#define FLIST_URL "https://www.f-list.net/"

#define FLIST_URL_NEWSPOST          FLIST_URL "newspost/%d/"
#define FLIST_URL_CHANGELOG         FLIST_URL "log.php?id=%d"
#define FLIST_URL_NOTE              FLIST_URL "view_note.php?note_id=%d"
#define FLIST_URL_BUGREPORT         FLIST_URL "view_bugreport.php?id=%d"
#define FLIST_URL_HELPDESKTICKET    FLIST_URL "view_ticket.php?id=%d"
#define FLIST_URL_FEATUREREQUEST    FLIST_URL "vote.php?fid=%d"

#define FLIST_URL_COMMENT_ANCHOR    "#Comment%d"
#define FLIST_URL_NEWSPOST_COMMENT  FLIST_URL_NEWSPOST FLIST_URL_COMMENT_ANCHOR
#define FLIST_URL_BUGREPORT_COMMENT FLIST_URL_BUGREPORT FLIST_URL_COMMENT_ANCHOR
#define FLIST_URL_CHANGELOG_COMMENT FLIST_URL_CHANGELOG FLIST_URL_COMMENT_ANCHOR
#define FLIST_URL_FEATURE_COMMENT   FLIST_URL_FEATUREREQUEST FLIST_URL_COMMENT_ANCHOR

typedef enum {
    None,
    FriendRequest,
    FriendAdd,
    FriendRemove,
    Note,
    BugReport,
    HelpdeskTicket,
    TicketCreate,
    HelpdeskReply,
    FeatureRequest,
    Comment
} RTB_TYPE;

gboolean flist_process_RTB(PurpleConnection *, JsonObject *);

void flist_init_RTB();

#endif	/* F_LIST_PROFILE_H */

