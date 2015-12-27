
#include "f-list_pidgin.h"
#include "f-list_report.h"

#include "gtkimhtml.h"
#include "gtkconv.h"
#include "gtkutils.h"
#include "pidginstock.h"

//TODO: fix these up so they're only one function ...

static gboolean flist_channel_activate_real(const gchar *host, const gchar *path) {
    PurpleAccount *pa;
    PurpleConnection *pc;
    GHashTable *components;

    purple_debug_info(FLIST_DEBUG, "We are attempting to join a channel. Account: %s Channel: %s\n", host, path);
    pa = flist_deserialize_account(host);
    if(!pa) {
        purple_debug_warning(FLIST_DEBUG, "Attempt failed. The account is not found.");
        return FALSE;
    }

    pc = purple_account_get_connection(pa);
    if(!pc) {
        purple_debug_warning(FLIST_DEBUG, "Attempt failed. The account has no connection.");
        return FALSE;
    }
    if(purple_connection_get_state(pc) != PURPLE_CONNECTED) {
        purple_debug_warning(FLIST_DEBUG, "Attempt failed. The account is not online.");
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
    purple_debug_info(FLIST_DEBUG, "FList channel URL activated: %s\n", url);

    if(!purple_url_parse(url, &host, &port, &path, &user, &password)) {
        purple_debug_warning(FLIST_DEBUG, "The FList channel URL did not parse.");
        return FALSE;
    }

    purple_debug_info(FLIST_DEBUG, "The FList channel URL is parsed. Host: %s Path: %s\n", host, path);

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

    purple_debug_info(FLIST_DEBUG, "We are attempting to send a staff confirmation. Account: %s Callid: %s\n", host, path);
    pa = flist_deserialize_account(host);
    if(!pa) {
        purple_debug_warning(FLIST_DEBUG, "Attempt failed. The account is not found.");
        return FALSE;
    }

    pc = purple_account_get_connection(pa);
    if(!pc) {
        purple_debug_warning(FLIST_DEBUG, "Attempt failed. The account has no connection.");
        return FALSE;
    }
    if(purple_connection_get_state(pc) != PURPLE_CONNECTED) {
        purple_debug_warning(FLIST_DEBUG, "Attempt failed. The account is not online.");
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
    purple_debug_info(FLIST_DEBUG, "FList staff URL activated: %s\n", url);

    if(!purple_url_parse(url, &host, &port, &path, &user, &password)) {
        purple_debug_warning(FLIST_DEBUG, "The FList staff URL did not parse.");
        return FALSE;
    }

    purple_debug_info(FLIST_DEBUG, "The FList staff URL is parsed. Host: %s Path: %s\n", host, path);

    if(host && path) {
        gchar *host_fixed = g_strdup(purple_url_decode(host));
        gchar *path_fixed = g_strdup(purple_url_decode(path));
        ret = flist_staff_activate_real(host_fixed, path_fixed);
        g_free(host_fixed);
        g_free(path_fixed);
    }

    return ret;
}

GtkWidget *get_account_icon(FListAccount *fla)
{
    PurpleStoredImage *account_image = purple_buddy_icons_find_account_icon(fla->pa);
    GtkWidget *icon = NULL;
    if (account_image)
    {
        GdkPixbuf *pixbuf = pidgin_pixbuf_from_imgstore(account_image);
        GdkPixbuf *scale = gdk_pixbuf_scale_simple(pixbuf, 22, 22, GDK_INTERP_BILINEAR);
        icon = gtk_image_new_from_pixbuf(scale);
        gtk_widget_set_sensitive(icon, TRUE);
        g_object_unref(pixbuf);
        g_object_unref(scale);

        return icon;
    }

    return NULL;
}

static void alert_staff_button_clicked_cb(GtkButton* button, gpointer func_data) {
    PurpleConversation *convo = (PurpleConversation*) func_data;
    PurpleConnection *pc = purple_conversation_get_gc(convo);
    FListAccount *fla = pc->proto_data;

    FListReport *flr = flist_report_new(fla, convo, NULL, NULL);
    flist_report_display_ui(flr);
}

static void character_button_clicked_cb(GtkButton* button, gpointer func_data) {
    PurpleConnection *pc = (PurpleConnection*) func_data;
    FListAccount *fla = pc->proto_data;
    flist_get_profile(pc, fla->proper_character);
}

static void flist_conversation_created_cb(PurpleConversation *conv, FListAccount *fla)
{
    g_return_if_fail(PIDGIN_IS_PIDGIN_CONVERSATION(conv));
    PidginConversation *pidgin_conv;
    g_return_if_fail((pidgin_conv = PIDGIN_CONVERSATION(conv)));

    PurpleConnection *pc = purple_conversation_get_gc(conv);

    // Is this a conversation of our account?
    if (fla->pc != pc)
        return;

    // We are going to add a button to the conversation's toolbar that simply displays our characters name
    // This helps in case you are online with more than one character
    GtkWidget *toolbar = pidgin_conv->toolbar;

    // Check, if the button already exists
    GtkWidget *char_button = purple_conversation_get_data(conv, FLIST_CONV_ACCOUNT_BUTTON);
    GtkWidget *alert_button = purple_conversation_get_data(conv, FLIST_CONV_ALERT_STAFF_BUTTON);

    // Snippet taken from pidgin-otr 4.0.1
    /* Check if we've been removed from the toolbar; purple does this
     * when the user changes her prefs for the style of buttons to
     * display. */
    GList *children = gtk_container_get_children(GTK_CONTAINER(toolbar));
    if (char_button && !g_list_find(children, char_button)) {
        if (fla->show_own_character) {
            gtk_box_pack_end(GTK_BOX(toolbar), char_button, FALSE, FALSE, 0);
            gtk_widget_show_all(char_button);
        }
    }

    if (alert_button && !g_list_find(children, alert_button)) {
        gtk_box_pack_end(GTK_BOX(toolbar), alert_button, FALSE, FALSE, 0);
        gtk_widget_show_all(alert_button);
    }
    g_list_free(children);

    // Let's check if we have created the icon widget yet
    // In case Pidgin hasn't yet fetched the account's icon, we need to do it here
    if (char_button) {
        GtkWidget *icon = gtk_button_get_image(GTK_BUTTON(char_button));
        if (!icon) {
            icon = get_account_icon(fla);

            if (icon)
            {
                GList *children = gtk_container_get_children(GTK_CONTAINER(char_button));
                gtk_box_pack_start(GTK_BOX(g_list_first(children)->data), icon, TRUE, FALSE, 0);
            }
        }
    }

    if (fla->show_own_character) {
        // Everything we allocate (and add to the conversation UI) here gets destroyed automatically once the conversation is destroyed

        // Character button
        char_button = gtk_button_new_with_label(fla->proper_character);
        GtkWidget *icon = get_account_icon(fla);
        if (icon)
            gtk_button_set_image(GTK_BUTTON(char_button), icon);

        gtk_button_set_relief(GTK_BUTTON(char_button), GTK_RELIEF_NONE);
        gtk_box_pack_end(GTK_BOX(toolbar), char_button, FALSE, FALSE, 0);

        gtk_widget_show_all(char_button);
        gtk_signal_connect(GTK_OBJECT(char_button), "clicked", GTK_SIGNAL_FUNC(character_button_clicked_cb), pc);
        purple_conversation_set_data(conv, FLIST_CONV_ACCOUNT_BUTTON, char_button);
    }

    // Alert Staff button
    alert_button = gtk_button_new_with_label("Alert Staff");
    gtk_button_set_image(GTK_BUTTON(alert_button), gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_SMALL_TOOLBAR));

    gtk_button_set_relief(GTK_BUTTON(alert_button), GTK_RELIEF_NONE);
    gtk_box_pack_end(GTK_BOX(toolbar), alert_button, FALSE, FALSE, 0);

    gtk_widget_show_all(alert_button);
    gtk_signal_connect(GTK_OBJECT(alert_button), "clicked", GTK_SIGNAL_FUNC(alert_staff_button_clicked_cb), conv);

    purple_conversation_set_data(conv, FLIST_CONV_ALERT_STAFF_BUTTON, alert_button);
}

void flist_pidgin_init() {
    gtk_imhtml_class_register_protocol("flistc", flist_channel_activate, NULL);
    gtk_imhtml_class_register_protocol("flistsfc", flist_staff_activate, NULL);
}

void flist_pidgin_enable_signals(FListAccount *fla)
{
    void *conv_handle = purple_conversations_get_handle();
    purple_signal_connect(conv_handle, "conversation-created", fla,
            PURPLE_CALLBACK(flist_conversation_created_cb), fla);
}

void flist_pidgin_disable_signals(FListAccount *fla)
{
    void *conv_handle = purple_conversations_get_handle();
    purple_signal_disconnect(conv_handle, "conversation-created", fla,
            PURPLE_CALLBACK(flist_conversation_created_cb));
}
