
#include "f-list_pidgin.h"

#include "gtkimhtml.h"

//TODO: fix these up so they're only one function ...

static gboolean flist_channel_activate_real(const gchar *host, const gchar *path) {
    PurpleAccount *pa;
    PurpleConnection *pc;
    GHashTable *components;

    purple_debug_info("flist", "We are attempting to join a channel. Account: %s Channel: %s\n", host, path);
    pa = flist_deserialize_account(host);
    if(!pa) {
        purple_debug_warning("flist", "Attempt failed. The account is not found.");
        return FALSE;
    }

    pc = purple_account_get_connection(pa);
    if(!pc) {
        purple_debug_warning("flist", "Attempt failed. The account has no connection.");
        return FALSE;
    }
    if(purple_connection_get_state(pc) != PURPLE_CONNECTED) {
        purple_debug_warning("flist", "Attempt failed. The account is not online.");
        return FALSE;
    }

    components = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
    g_hash_table_insert(components, (gpointer) "channel", (gpointer) purple_url_decode(path));
    flist_join_channel(pc, components);
    g_hash_table_destroy(components);

    return TRUE;
}

static gboolean flist_channel_activate(GtkIMHtml *imhtml, GtkIMHtmlLink *link) {
    const gchar *url = gtk_imhtml_link_get_url(link);
    gchar *host, *path, *user, *password;
    int port;
    gboolean ret = FALSE;

    url += strlen("flistc://");
    purple_debug_info("flist", "FList channel URL activated: %s\n", url);

    if(!purple_url_parse(url, &host, &port, &path, &user, &password)) {
        purple_debug_warning("flist", "The FList channel URL did not parse.");
        return FALSE;
    }

    purple_debug_info("flist", "The FList channel URL is parsed. Host: %s Path: %s\n", host, path);
    
    if(host && path) {
        gchar *host_fixed = g_strdup(purple_url_decode(host));
        gchar *path_fixed = g_strdup(purple_url_decode(path));
        ret = flist_channel_activate_real(host_fixed, path_fixed);
        g_free(host_fixed);
        g_free(path_fixed);
    }

    if(host) g_free(host);
    if(path) g_free(path);
    if(user) g_free(user);
    if(password) g_free(password);

    return ret;
}

gboolean flist_staff_activate_real(const gchar *host, const gchar *path) {
    PurpleAccount *pa;
    PurpleConnection *pc;

    purple_debug_info("flist", "We are attempting to send a staff confirmation. Account: %s Callid: %s\n", host, path);
    pa = flist_deserialize_account(host);
    if(!pa) {
        purple_debug_warning("flist", "Attempt failed. The account is not found.");
        return FALSE;
    }

    pc = purple_account_get_connection(pa);
    if(!pc) {
        purple_debug_warning("flist", "Attempt failed. The account has no connection.");
        return FALSE;
    }
    if(purple_connection_get_state(pc) != PURPLE_CONNECTED) {
        purple_debug_warning("flist", "Attempt failed. The account is not online.");
        return FALSE;
    }

    flist_send_sfc_confirm(pc, path);

    return TRUE;
}

static gboolean flist_staff_activate(GtkIMHtml *imhtml, GtkIMHtmlLink *link) {
    const gchar *url = gtk_imhtml_link_get_url(link);
    gchar *host, *path, *user, *password;
    int port;
    gboolean ret = FALSE;

    url += strlen("flistsfc://");
    purple_debug_info("flist", "FList staff URL activated: %s\n", url);

    if(!purple_url_parse(url, &host, &port, &path, &user, &password)) {
        purple_debug_warning("flist", "The FList staff URL did not parse.");
        return FALSE;
    }

    purple_debug_info("flist", "The FList staff URL is parsed. Host: %s Path: %s\n", host, path);

    if(host && path) {
        gchar *host_fixed = g_strdup(purple_url_decode(host));
        gchar *path_fixed = g_strdup(purple_url_decode(path));
        ret = flist_staff_activate_real(host_fixed, path_fixed);
        g_free(host_fixed);
        g_free(path_fixed);
    }
    
    return ret;
}

void flist_pidgin_init() {
    gtk_imhtml_class_register_protocol("flistc", flist_channel_activate, NULL);
    gtk_imhtml_class_register_protocol("flistsfc", flist_staff_activate, NULL);
}
