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

/* This file attemmpts to hook the ssl_ops of the ssl-nss plugin to inject a
 * work-around for a bug in libnss versions below 3.23 causing sessions to be
 * incorrectly reused across domains, and causing HTTPS requests to fail.
 */

#ifndef DISABLE_NSSFIX

#include "f-list.h"

/* NSSSSL */
#include <ssl.h>

/* This should match the PurpleSslNssData structure in the ssl-nss plugin. */
typedef struct {
    PRFileDesc *fd;
    PRFileDesc *in;
    guint handshake_handler;
    guint handshake_timer;
} nssfixPurpleSslNssData;

static PurpleSslOps new_ops = { };
static void (*original_connectfunc)(PurpleSslConnection *gsc) = NULL;

static void hooked_connectfunc(PurpleSslConnection *gsc) {
    if(purple_debug_is_verbose())
        purple_debug_info(FLIST_DEBUG, "nssfix: Running hooked connectfunc\n");

    original_connectfunc(gsc);
    nssfixPurpleSslNssData *nss_data = gsc->private_data;

    if (purple_debug_is_verbose())
        purple_debug_info(FLIST_DEBUG, "nssfix: Setting Peer ID: %s\n", gsc->host);

    SSL_SetSockPeerID(nss_data->in, gsc->host);
}

gboolean flist_nssfix_enable() {
    if(strcmp(NSSSSL_GetVersion(), "3.23") >= 0) {
        purple_debug_info(FLIST_DEBUG, "nssfix: Doing nothing - NSS is version 3.23 or later\n");
        return FALSE;
    }

    gboolean using_nss_ssl = FALSE;
    int num_ssl_plugins = 0;

    for(GList *l = purple_plugins_get_all(); l != NULL; l = l->next) {
        PurplePlugin *plugin = (PurplePlugin *)l->data;

        if (plugin->info != NULL && plugin->info->id != NULL && strncmp(plugin->info->id, "ssl-", 4) == 0) {
            gboolean is_nss_ssl = (strcmp(plugin->info->id, "ssl-nss") == 0);

            if (purple_plugin_is_loaded(plugin)) {
                ++num_ssl_plugins;
                
                if (is_nss_ssl) {
                    if (num_ssl_plugins == 1) {
                        purple_debug_info(FLIST_DEBUG, "nssfix: ssl-nss appears to be the active SSL plugin\n");
                        using_nss_ssl = TRUE;
                    }
                    break;
                }
            } else if (is_nss_ssl && num_ssl_plugins == 0) {
                purple_debug_info(FLIST_DEBUG, "nssfix: Manually loading ssl-nss plugin\n");
                if (purple_plugin_load(plugin)) {
                    ++num_ssl_plugins;
                    using_nss_ssl = TRUE;
                } else {
                    purple_debug_warning(FLIST_DEBUG, "nssfix: The ssl-nss plugin failed to load\n");
                }
            }
        }
    }

    if (!using_nss_ssl) {
        purple_debug_info(FLIST_DEBUG, "nssfix: Doing nothing - ssl-nss plugin does not appear to be the active SSL plugin\n");
        return FALSE;
    }

    if (num_ssl_plugins > 1) {
        purple_debug_warning(FLIST_DEBUG, "nssfix: There is more than one SSL plugin loaded! There is a small chance something could go wrong here.\n");
    }

    PurpleSslOps *current_opts = purple_ssl_get_ops();

    if(!current_opts) {
        purple_debug_info(FLIST_DEBUG, "nssfix: Doing nothing - SSL operations were not set\n");
        return FALSE;
    }

    memcpy(&new_ops, current_opts, sizeof(PurpleSslOps));

    if(original_connectfunc != NULL || new_ops.connectfunc == hooked_connectfunc) {
        purple_debug_info(FLIST_DEBUG, "nssfix: Doing nothing - SSL operations were already hooked\n");
        return FALSE;
    }

    original_connectfunc = new_ops.connectfunc;
    new_ops.connectfunc = hooked_connectfunc;

    purple_debug_info(FLIST_DEBUG, "nssfix: Hooking SSL operations\n");
    purple_ssl_set_ops(&new_ops);

    return TRUE;
}

gboolean flist_nssfix_disable() {
    if(original_connectfunc != NULL) {
        new_ops.connectfunc = original_connectfunc;

        purple_debug_info(FLIST_DEBUG, "nssfix: Un-hooking SSL operations\n");
        purple_ssl_set_ops(&new_ops);

        original_connectfunc = NULL;
    }

    return TRUE;
}

#else // DISABLE_NSSFIX

gboolean flist_nssfix_enable() {
    return FALSE;
}

gboolean flist_nssfix_disable() {
    return FALSE;
}

#endif // DISABLE_NSSFIX
