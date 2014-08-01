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
#ifndef FLIST_CONNECTION_H
#define FLIST_CONNECTION_H

#define WS_FINAL_SEGMENT 0x01
#define WS_OPCODE_TYPE_CONTINUATION 0x00

#define WS_OPCODE_TYPE_TEXT 1
#define WS_OPCODE_TYPE_BINARY 2
#define WS_OPCODE_TYPE_CLOSE 8
#define WS_OPCODE_TYPE_PING 9
#define WS_OPCODE_TYPE_PONG 10

#include "f-list.h"

const gchar *flist_get_ticket(FListAccount *);
void flist_request(PurpleConnection *, const gchar *, JsonObject *);

void flist_receive_ping(PurpleConnection *);
void flist_ticket_timer(FListAccount *, guint);

void flist_ticket_init();

#endif
