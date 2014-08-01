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
#include "f-list_connection.h"

/* disconnect after 90 seconds without a ping response */
#define FLIST_TIMEOUT 90
/* how often we request a new ticket for the API */
#define FLIST_TICKET_TIMER_TIMEOUT 600

//TODO: put tickets into their own code file
GHashTable *ticket_table;
const gchar *flist_get_ticket(FListAccount *fla) {
    return g_hash_table_lookup(ticket_table, fla->username);
}

static gboolean flist_disconnect_cb(gpointer user_data) {
    PurpleConnection *pc = user_data;
    FListAccount *fla = pc->proto_data;

    fla->ping_timeout_handle = 0;

    purple_connection_error_reason(pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, "Connection timed out.");

    return FALSE;
}

void flist_receive_ping(PurpleConnection *pc) {
    FListAccount *fla = pc->proto_data;

    if(fla->ping_timeout_handle) {
        purple_timeout_remove(fla->ping_timeout_handle);
    }
    fla->ping_timeout_handle = purple_timeout_add_seconds(FLIST_TIMEOUT, flist_disconnect_cb, pc);
}

static gsize flist_write_raw(FListAccount *fla, gchar *buf, gsize len) {
    if(fla->secure) {
        return purple_ssl_write(fla->ssl_con, buf, len);
    } else {
        return write(fla->fd, buf, len);
    }
}

static void flist_write_hybi(FListAccount *fla, guint8 opcode, gchar *content, gsize len) {
    guint8 *frame, *ptr;
    gsize frame_len;
    guint8 mask[4];
    int offset;
    gsize sent;
    
    frame = g_malloc(10 + len);
    ptr = frame;
    *ptr = (opcode << 4) | 0x01; ptr++;
    
    /* The length is sent as a big endian integer. */
    if(len < 126) {
        *ptr = (guint8) ((len & 0x7F) | 0x80); ptr++;
    } else if(len >= 126 && len < 65536) {
        *ptr = 0xFE; ptr++;
        *ptr = (guint8) ((len >>  8) & 0xFF); ptr++;
        *ptr = (guint8) ((len >>  0) & 0xFF); ptr++;
    } else if(len >= 65536) {
        *ptr = 0xFF; ptr++;
        *ptr = (guint8) 0; ptr++;
        *ptr = (guint8) 0; ptr++;
        *ptr = (guint8) 0; ptr++;
        *ptr = (guint8) 0; ptr++;
        *ptr = (guint8) ((len >>  24) & 0xFF); ptr++;
        *ptr = (guint8) ((len >>  16) & 0xFF); ptr++;
        *ptr = (guint8) ((len >>   8) & 0xFF); ptr++;
        *ptr = (guint8) ((len >>   0) & 0xFF); ptr++;
    }
    
    mask[0] = (guint8) (g_random_int() & 0xFF);
    mask[1] = (guint8) (g_random_int() & 0xFF);
    mask[2] = (guint8) (g_random_int() & 0xFF);
    mask[3] = (guint8) (g_random_int() & 0xFF);
    memcpy(ptr, mask, 4); ptr += 4;
    memcpy(ptr, content, len);
    
    for(offset = 0; offset < len; offset++) { /* Apply the mask. */
        ptr[offset] = ptr[offset] ^ mask[offset % 4];
    }
    
    frame_len = (gsize) (ptr + len - frame);
    // TODO: check the return value of write()
    sent = flist_write_raw(fla, (gchar*) frame, frame_len);
    g_free(frame);
}

void flist_request(PurpleConnection *pc, const gchar* type, JsonObject *object) {
    FListAccount *fla = pc->proto_data;
    gsize json_len;
    gchar *json_text = NULL;
    GString *to_write_str;
    gchar *to_write;
    gsize to_write_len;
    
    to_write_str = g_string_new(NULL);
    g_string_append(to_write_str, type);
    
    if(object) {
        JsonNode *root = json_node_new(JSON_NODE_OBJECT);
        JsonGenerator *gen = json_generator_new();
        json_node_set_object(root, object);
        json_generator_set_root(gen, root);
        json_text = json_generator_to_data(gen, &json_len);
        g_string_append(to_write_str, " ");
        g_string_append(to_write_str, json_text);
        g_free(json_text);
        g_object_unref(gen);
        json_node_free(root);
    }
        
    to_write_len = to_write_str->len;
    to_write = g_string_free(to_write_str, FALSE);
    flist_write_hybi(fla, WS_OPCODE_TYPE_TEXT, to_write, to_write_len);
    g_free(to_write);
}

static gboolean flist_recv(FListAccount *fla) {
    gchar buf[4096];
    gssize len;
    
    if(fla->secure) {
        len = purple_ssl_read(fla->ssl_con, buf, sizeof(buf) - 1);
    } else {
        len = recv(fla->fd, buf, sizeof(buf) - 1, 0);
    }
    if(len <= 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) return FALSE; //try again later
        //TODO: better error reporting
        purple_connection_error_reason(fla->pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, "The connection has failed.");
        return FALSE;
    }
    buf[len] = '\0';
    fla->rx_buf = g_realloc(fla->rx_buf, fla->rx_len + len + 1);
    memcpy(fla->rx_buf + fla->rx_len, buf, len + 1);
    fla->rx_len += len;
    return TRUE;
}

static gboolean flist_dispatch(FListAccount *fla, gchar *message, gsize len) {
    JsonParser *parser = NULL;
    JsonNode *root = NULL;
    JsonObject *object = NULL;
    GError *err = NULL;
    gchar *code = NULL;
    gboolean success = FALSE;
    
    if(len < 3) {
        purple_connection_error_reason(fla->pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, "The server sent an invalid packet.");
        goto cleanup;
    }
    
    code = g_strndup(message, 3);
    if(fla->debug_mode) purple_debug_info(FLIST_DEBUG, "Parsing message... (Code: %s)\n", code);
    
    if(len > 3) {
        parser = json_parser_new();
        json_parser_load_from_data(parser, message + 4, len - 4, &err);
        
        if(err) { /* not valid json */
            purple_connection_error_reason(fla->pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, "The server sent non-JSON data.");
            g_error_free(err);
            goto cleanup;
        }
        root = json_parser_get_root(parser);
        if(json_node_get_node_type(root) != JSON_NODE_OBJECT) {
            purple_connection_error_reason(fla->pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, "The server sent JSON data that is not an object.");
            goto cleanup;
        }
        object = json_node_get_object(root);
    } else {
        if(fla->debug_mode) purple_debug_info(FLIST_DEBUG, "(No JSON data was included.)\n");
    }
    
    success = TRUE;
    purple_debug_info(FLIST_DEBUG, "Processing message... (Code: %s)\n", code);
    flist_callback(fla->pc, code, object);
    
    cleanup:
    if(code) g_free(code);
    if(parser) g_object_unref(parser);
    
    return success;
}

static gboolean flist_process_hybi_frame(FListAccount *fla, gboolean is_final, guint8 opcode, gchar *payload, gsize payload_len) {
    switch(opcode) {
    case WS_OPCODE_TYPE_BINARY:
    case WS_OPCODE_TYPE_TEXT:
        if(fla->frame_buffer) {
            purple_connection_error_reason(fla->pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, "The server sent a WebSocket data frame, but we are expecting a continuation frame.");
            return FALSE;
        }
        if(!is_final) { // We only have part of a frame. Save it instead of returning it.
            fla->frame_buffer = g_byte_array_new();
            g_byte_array_append(fla->frame_buffer, (guint8*) payload, payload_len);
            return TRUE;
        }
        return flist_dispatch(fla, payload, payload_len);
    case WS_OPCODE_TYPE_CONTINUATION: {
        gchar *full_message;
        gsize full_len;
        gboolean ret;
        if(!fla->frame_buffer) {
            purple_connection_error_reason(fla->pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, "The server unexpectedly sent a WebSocket continuation frame.");
            return FALSE;
        }
        g_byte_array_append(fla->frame_buffer, (guint8*) payload, payload_len);
        if(!is_final) return TRUE;
        full_len = fla->frame_buffer->len;
        full_message = (gchar*) g_byte_array_free(fla->frame_buffer, FALSE);
        fla->frame_buffer = NULL;
        ret = flist_dispatch(fla, full_message, full_len);
        g_free(full_message);
        return ret;
    }
    case WS_OPCODE_TYPE_PING: //We have to respond with a 'pong' frame.
        if(!is_final) {
            purple_connection_error_reason(fla->pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, "The server sent a fragmented WebSocket ping frame.");
            return FALSE;
        }
        flist_write_hybi(fla, WS_OPCODE_TYPE_PONG, payload, payload_len);
        return TRUE;
    case WS_OPCODE_TYPE_PONG: //Silently discard.
        if(!is_final) {
            purple_connection_error_reason(fla->pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, "The server sent a fragmented WebSocket pong frame.");
            return FALSE;
        }
        return TRUE;
    case WS_OPCODE_TYPE_CLOSE:
        purple_connection_error_reason(fla->pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, "The server has terminated the connection with a WebSocket close frame.");
        return FALSE;
    default:
        purple_connection_error_reason(fla->pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, "The server sent a WebSocket frame with an unknown opcode.");
        return FALSE;
    }
}

/* This function processes all available hybi frames. */
static void flist_process_hybi(FListAccount *fla) {
    gchar *cur, *rx_end;
    guint8 opcode;
    gboolean is_final, is_masked;
    guint8 raw_len;
    guint8 mask[4];
    gchar *payload;
    gsize payload_len;
    int offset;
    gboolean stop = FALSE;
    
    while(!stop) {
        rx_end = fla->rx_buf + fla->rx_len;
        
        cur = fla->rx_buf;
        if(cur + 2 > rx_end) return;

        is_final = (*cur & 0x80) != 0;
        opcode = *cur & 0x0F;
        cur += 1;
        is_masked = (*cur & 0x80) != 0;
        raw_len = *cur & 0x7F;
        cur += 1;
        
        switch(raw_len) { /* TODO: These conversations should work, but we should not be casting pointers. */
        case 0x7E: /* The actual length is the next two bytes in big endian format. */
            if(cur + 2 > rx_end) return;
            payload_len = GUINT16_FROM_BE(*((guint16*) cur));
            cur += 2;
            break;
        case 0x7F: /* The actual length is the next eight bytes in big endian format. */
            if(cur + 8 > rx_end) return;
            payload_len = GUINT64_FROM_BE(*((guint64*) cur));
            cur += 8;
            break;
        default: /* The actual length is the raw length. */
            payload_len = raw_len;
        }
        
        if(is_masked) {
            if(cur + 4 > rx_end) return;
            mask[0] = cur[0];
            mask[1] = cur[1];
            mask[2] = cur[2];
            mask[3] = cur[3];
            cur += 4;
        }
        
        if(cur + payload_len > rx_end) return;
        payload = g_malloc(payload_len);
        memcpy(payload, cur, payload_len);
        cur += payload_len;
        
        if(is_masked) { // Unmask the data.
            for(offset = 0; offset < payload_len; offset++) {
                payload[offset] = payload[offset] ^ mask[offset % 4];
            }
        }
        
        // We've read one new frame. We clean up the buffer here.
        fla->rx_len = (gsize) (fla->rx_buf + fla->rx_len - cur);
        memmove(fla->rx_buf, cur, fla->rx_len + 1);
        
        stop = !flist_process_hybi_frame(fla, is_final, opcode, payload, payload_len);
        
        g_free(payload);
    }
}

static void flist_identify(FListAccount *fla) {
    JsonObject *object;
    const gchar *ticket = flist_get_ticket(fla);
    
    object = json_object_new();
    if(ticket) {
        json_object_set_string_member(object, "method", "ticket");
        json_object_set_string_member(object, "ticket", ticket);
        json_object_set_string_member(object, "account", fla->username);
        json_object_set_string_member(object, "cname", FLIST_CLIENT_NAME);
        json_object_set_string_member(object, "cversion", FLIST_PLUGIN_VERSION);
    }
    json_object_set_string_member(object, "character", fla->character);
    
    purple_debug_info(FLIST_DEBUG, "Identifying... (Ticket: %s) (Account: %s) (Character: %s)\n", ticket, fla->username, fla->character);
    flist_request(fla->pc, "IDN", object);
    
    json_object_unref(object);
}

static gboolean flist_handle_handshake(PurpleConnection *pc) {
    FListAccount *fla = pc->proto_data;
    gchar *last = fla->rx_buf;
    gchar *read = strstr(last, "\r\n");
    
    while(read != NULL && read > last) {
        last = read + 2;
        read = strstr(last, "\r\n");
    }
    
    if(read == NULL) return FALSE;

    read += 2; //last line
    fla->rx_len -= (gsize) (read - fla->rx_buf);
    memmove(fla->rx_buf, read, fla->rx_len + 1);
    
    flist_identify(fla);
    fla->connection_status = FLIST_IDENTIFY;
    return TRUE;
}

static void flist_process_real(FListAccount *fla) {
    if(!flist_recv(fla)) return;
    if(fla->connection_status == FLIST_HANDSHAKE && !flist_handle_handshake(fla->pc)) return;
    flist_process_hybi(fla);
}

static void flist_process(gpointer data, gint source, PurpleInputCondition cond) {
    PurpleConnection *pc = data;
    FListAccount *fla = pc->proto_data;
    flist_process_real(fla);
}

static void flist_process_secure(gpointer data, PurpleSslConnection *ssl_con, PurpleInputCondition cond) {
    PurpleConnection *pc = data;
    FListAccount *fla = pc->proto_data;
    flist_process_real(fla);
}

static void flist_handshake(FListAccount *fla) {
    GString *headers_str = g_string_new(NULL);
    gchar *headers, *b64_data;
    int i, len;
    guchar nonce[16];
    for(i = 0; i < 16; i++) {
        nonce[i] = (guchar) (g_random_int() & 0xFF);
    }
    b64_data = g_base64_encode(nonce, 16);
    
    fla->ping_timeout_handle = purple_timeout_add_seconds(FLIST_TIMEOUT, flist_disconnect_cb, fla->pc);
    
    g_string_append(headers_str, "GET / HTTP/1.1\r\n");
    g_string_append(headers_str, "Upgrade: WebSocket\r\n");
    g_string_append(headers_str, "Connection: Upgrade\r\n");
    g_string_append_printf(headers_str, "Host: %s:%d\r\n", fla->server_address, fla->server_port);
    g_string_append(headers_str, "Origin: http://www.f-list.net\r\n");
    g_string_append_printf(headers_str, "Sec-WebSocket-Key: %s\r\n", b64_data); //TODO: insert proper randomness here!
    g_string_append(headers_str, "Sec-WebSocket-Version: 13\r\n");
    g_string_append(headers_str, "\r\n");
    headers = g_string_free(headers_str, FALSE);

    purple_debug_info(FLIST_DEBUG, "Sending WebSocket handshake...\n");
    len = flist_write_raw(fla, headers, strlen(headers));
    fla->connection_status = FLIST_HANDSHAKE;
    g_free(headers);
    
    //TODO: save this to verify the handshake
    g_free(b64_data);
}

static void flist_ssl_cb(gpointer user_data, PurpleSslConnection *ssl_con, PurpleInputCondition ic) {
    FListAccount *fla = user_data;
    purple_debug_info(FLIST_DEBUG, "SSL handshake successful.\n");
    purple_ssl_input_add(ssl_con, flist_process_secure, fla->pc);
    flist_handshake(fla);
}

static void flist_ssl_error_cb(PurpleSslConnection *ssl_con, PurpleSslErrorType err, gpointer user_data) {
    FListAccount *fla = user_data;
    fla->ssl_con = NULL;
    purple_debug_info(FLIST_DEBUG, "SSL handshake failed.\n");
    purple_connection_error_reason(fla->pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, "The SSL handshake failed.");
}

static void flist_connected(gpointer user_data, int fd, const gchar *err) {
    FListAccount *fla = user_data;
    
    if(err) {
        purple_connection_error_reason(fla->pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, err);
        fla->connection_status = FLIST_OFFLINE;
        return;
    }
    
    if(fla->secure) { // If this is a secure connection, we have to perform the SSL handshake.
        purple_debug_info(FLIST_DEBUG, "Sending SSL handshake...\n");
        fla->ssl_con = purple_ssl_connect_with_host_fd(fla->pa, fd, flist_ssl_cb, flist_ssl_error_cb, fla->server_address, fla);
        return;
    }
    
    fla->fd = fd;
    fla->input_handle = purple_input_add(fla->fd, PURPLE_INPUT_READ, flist_process, fla->pc);
    flist_handshake(fla);
}

static void flist_connect(FListAccount *fla) {
    purple_debug_info(FLIST_DEBUG, "Connecting... (Server: %s) (Port: %d) (Secure: %s)\n", 
            fla->server_address, fla->server_port, fla->secure ? "yes" : "no");
    if(!purple_proxy_connect(fla->pc, fla->pa, fla->server_address, fla->server_port, flist_connected, fla)) {
        purple_connection_error_reason(fla->pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, _("Unable to open a connection."));
    } else {
        fla->connection_status = FLIST_CONNECT;
    }
}

static void flist_receive_ticket(FListWebRequestData *req_data, gpointer data, JsonObject *root, const gchar *error) {
    FListAccount *fla = data;
    const gchar *ticket;
    gboolean first = fla->connection_status == FLIST_OFFLINE;
    
    fla->ticket_request = NULL;
    flist_ticket_timer(fla, FLIST_TICKET_TIMER_TIMEOUT);
    
    if(error) {
        if(first) purple_connection_error_reason(fla->pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, error);
        return;
    }
    
    error = json_object_get_string_member(root, "error");
    if(error && strlen(error)) {
        if(first) purple_connection_error_reason(fla->pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, error);
        return;
    }
    
    ticket = json_object_get_string_member(root, "ticket");
    if(!ticket) {
        if(first) purple_connection_error_reason(fla->pc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, "No ticket returned.");
        return;
    }
    
    purple_debug_info(FLIST_DEBUG, "Ticket received. (Account: %s) (Character: %s) (Ticket: %s)\n", fla->username, fla->character, ticket);
    
    g_hash_table_insert(ticket_table, g_strdup(fla->username), g_strdup(ticket));
    purple_debug_info("flist", "Login Ticket: %s\n", ticket);
    
    if(first) {
        flist_connect(fla);
    }
}

static gboolean flist_ticket_timer_cb(gpointer data) {
    FListAccount *fla = data;
    GHashTable *args = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);
    g_hash_table_insert(args, "account", g_strdup(fla->username));
    g_hash_table_insert(args, "password", g_strdup(fla->password));
    
    purple_debug_info(FLIST_DEBUG, "Requesting ticket... (Account: %s) (Character: %s)\n", fla->username, fla->character);
    fla->ticket_request = flist_web_request(JSON_GET_TICKET, args, TRUE, fla->secure, flist_receive_ticket, fla);
    fla->ticket_timer = 0;
    
    g_hash_table_destroy(args);
    
    return FALSE;
}

void flist_ticket_timer(FListAccount *fla, guint timeout) {
    if(fla->ticket_timer) {
        purple_timeout_remove(fla->ticket_timer);
    }
    fla->ticket_timer = purple_timeout_add_seconds(timeout, (GSourceFunc) flist_ticket_timer_cb, fla);
}

void flist_ticket_init() {
    ticket_table = g_hash_table_new_full((GHashFunc) flist_str_hash, (GEqualFunc) flist_str_equal, g_free, g_free);
}
