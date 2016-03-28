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

#include "f-list_util.h"

#include <ctype.h>

FListPermissionMask flist_get_permissions(FListAccount *fla, const gchar *character, const gchar *channel)
{
    FListPermissionMask ret = FLIST_PERMISSION_NONE;
    FListChannel *fchannel = channel ? flist_channel_find(fla, channel) : NULL;

    if(channel && !fchannel) {
        purple_debug_error(FLIST_DEBUG, "Flags requested for %s in channel %s, but no channel was found.\n", character, channel);
        return ret;
    }

    if(fchannel && fchannel->owner && !purple_utf8_strcasecmp(fchannel->owner, character))
        ret |= FLIST_PERMISSION_CHANNEL_OWNER;

    if(fchannel && g_list_find_custom(fchannel->operators, character, (GCompareFunc)purple_utf8_strcasecmp))
        ret |= FLIST_PERMISSION_CHANNEL_OP;

    if(g_hash_table_lookup(fla->global_ops, character) != NULL)
        ret |= FLIST_PERMISSION_GLOBAL_OP;

    return ret;
}

PurpleConvChatBuddyFlags flist_permissions_to_purple(FListPermissionMask permission)
{
    PurpleConvChatBuddyFlags flags = 0;

    if (FLIST_HAS_PERMISSION(permission, FLIST_PERMISSION_CHANNEL_OWNER))
        flags |= PURPLE_CBFLAGS_FOUNDER;

    if (FLIST_HAS_PERMISSION(permission, FLIST_PERMISSION_CHANNEL_OP))
        flags |= PURPLE_CBFLAGS_HALFOP;

    if (FLIST_HAS_PERMISSION(permission, FLIST_PERMISSION_GLOBAL_OP))
        flags |= PURPLE_CBFLAGS_OP;

    return flags;
}

//TODO: you're supposed to change spaces to "+" values??
static void g_string_append_cgi(GString *str, GHashTable *table) {
    GHashTableIter iter;
    gpointer key, value;
    gboolean first = TRUE;
    g_hash_table_iter_init(&iter, table);
    while(g_hash_table_iter_next(&iter, &key, &value)) {
        purple_debug_info(FLIST_DEBUG, "cgi writing key, value: %s, %s\n", (gchar *)key, (gchar *)value);
        if(!first) g_string_append(str, "&");

        // We use g_uri_escape_string instead of purple_url_encode 
        // because the latter only supports strings up to 2048 bytes
        // (depending on BUF_LEN)
        gchar *encoded_key = g_uri_escape_string(key, NULL, FALSE);
        g_string_append_printf(str, "%s", encoded_key);
        g_free(encoded_key);

        g_string_append(str, "=");

        gchar *encoded_value = g_uri_escape_string(value, NULL, FALSE);
        g_string_append_printf(str, "%s", encoded_value);
        g_free(encoded_value);

        first = FALSE;
    }
}

static void g_string_append_cookies(GString *str, GHashTable *table) {
    GHashTableIter iter;
    gpointer key, value;
    gboolean first = TRUE;
    g_hash_table_iter_init(&iter, table);
    while(g_hash_table_iter_next(&iter, &key, &value)) {
        if(!first) g_string_append(str, " ");
        g_string_append_printf(str, "%s", purple_url_encode(key));
        g_string_append(str, "=");
        g_string_append_printf(str, "%s;", purple_url_encode(value));
        first = FALSE;
    }
}

//mostly shamelessly stolen from pidgin's "util.c"
gchar *http_request(const gchar *url, gboolean http11, gboolean post, const gchar *user_agent, GHashTable *req_table, GHashTable *cookie_table) {
    GString *request_str = g_string_new(NULL);
    gchar *address = NULL, *page = NULL, *user = NULL, *password = NULL;
    int port;

    purple_url_parse(url, &address, &port, &page, &user, &password);

    g_string_append_printf(request_str, "%s /%s%s", (post ? "POST" : "GET"), page, (!post && req_table ? "?" : ""));
    if(req_table && !post) g_string_append_cgi(request_str, req_table);
    g_string_append_printf(request_str, " HTTP/%s\r\n", (http11 ? "1.1" : "1.0"));
    g_string_append_printf(request_str, "Connection: close\r\n");
    if(user_agent) g_string_append_printf(request_str, "User-Agent: %s\r\n", user_agent);
    g_string_append_printf(request_str, "Accept: */*\r\n");
    g_string_append_printf(request_str, "Host: %s\r\n", address);

    if(cookie_table) {
        g_string_append(request_str, "Cookie: ");
        g_string_append_cookies(request_str, cookie_table);
        g_string_append(request_str, "\r\n");
    }

    if(post) {
        GString *post_str = g_string_new(NULL);
        gchar *post = NULL;

        if(req_table) g_string_append_cgi(post_str, req_table);

        post = g_string_free(post_str, FALSE);

        purple_debug_info(FLIST_DEBUG, "posting (len: %" G_GSIZE_FORMAT "): %s\n", strlen(post), post);

        g_string_append(request_str, "Content-Type: application/x-www-form-urlencoded\r\n");
        g_string_append_printf(request_str, "Content-Length: %" G_GSIZE_FORMAT "\r\n", strlen(post));
        g_string_append(request_str, "\r\n");

        g_string_append(request_str, post);

        g_free(post);
    } else {
        g_string_append(request_str, "\r\n");
    }

    if(address) g_free(address);
    if(page) g_free(page);
    if(user) g_free(user);
    if(password) g_free(password);

    return g_string_free(request_str, FALSE);
}

guint64 flist_parse_duration_str(const gchar* dur) {
    gchar *endptr = NULL;
    const gchar *startptr = dur;
    guint64 duration = 0;

    while (!endptr || *endptr != '\0') {
        endptr = NULL;
        guint64 tmp = g_ascii_strtoull(startptr, &endptr, 10);

        switch(*endptr) {
            case 'w': duration += tmp * 10080; break;
            case 'd': duration += tmp * 1440; break;
            case 'h': duration += tmp * 60; break;
            case 'm':
            case '\0': duration += tmp; break;
            default: return 0;
        }

        startptr = endptr + 1;
    }

    return duration;
}

gchar *flist_format_duration_str(guint64 dur) {
    GString *tmp = g_string_new(NULL);

    guint64 minutes = dur % 60;
    guint64 total_hours = dur / 60;
    guint64 hours = total_hours % 24;
    guint64 total_days = total_hours / 24;
    guint64 days = total_days % 7;
    guint64 total_weeks = total_days / 7;

    if (total_weeks > 0) g_string_append_printf(tmp, "%luw", (unsigned long)total_weeks);
    if (days > 0) g_string_append_printf(tmp, "%lud", (unsigned long)days);
    if (hours > 0) g_string_append_printf(tmp, "%luh", (unsigned long)hours);
    if (minutes > 0) g_string_append_printf(tmp, "%lum", (unsigned long)minutes);

    return g_string_free(tmp, FALSE);
}

struct EntityTableEntry
{
    const char name[12];
    const unsigned char result[4];
    size_t namelen;
};

// 252 HTML4 entities + &apos;
struct EntityTableEntry entity_table_utf8[] = {
    {"&Agrave;",   {0xC3, 0x80}, 8},
    {"&Aacute;",   {0xC3, 0x81}, 8},
    {"&Acirc;",    {0xC3, 0x82}, 7},
    {"&Atilde;",   {0xC3, 0x83}, 8},
    {"&Auml;",     {0xC3, 0x84}, 6},
    {"&Aring;",    {0xC3, 0x85}, 7},
    {"&AElig;",    {0xC3, 0x86}, 7},
    {"&Alpha;",    {0xCE, 0x91}, 7},
    {},
    {"&Beta;",     {0xCE, 0x92}, 6},
    {},
    {"&Ccedil;",   {0xC3, 0x87}, 8},
    {"&Chi;",      {0xCE, 0xA7}, 5},
    {},
    {"&Dagger;",   {0xE2, 0x80, 0xA1}, 8},
    {"&Delta;",    {0xCE, 0x94}, 7},
    {},
    {"&Egrave;",   {0xC3, 0x88}, 8},
    {"&Eacute;",   {0xC3, 0x89}, 8},
    {"&Ecirc;",    {0xC3, 0x8A}, 7},
    {"&Euml;",     {0xC3, 0x8B}, 6},
    {"&ETH;",      {0xC3, 0x90}, 5},
    {"&Epsilon;",  {0xCE, 0x95}, 9},
    {"&Eta;",      {0xCE, 0x97}, 5},
    {},
    {},
    {"&Gamma;",    {0xCE, 0x93}, 7},
    {},
    {},
    {"&Igrave;",   {0xC3, 0x8C}, 8},
    {"&Iacute;",   {0xC3, 0x8D}, 8},
    {"&Icirc;",    {0xC3, 0x8E}, 7},
    {"&Iuml;",     {0xC3, 0x8F}, 6},
    {"&Iota;",     {0xCE, 0x99}, 6},
    {},
    {},
    {"&Kappa;",    {0xCE, 0x9A}, 7},
    {},
    {"&Lambda;",   {0xCE, 0x9B}, 8},
    {},
    {"&Mu;",       {0xCE, 0x9C}, 4},
    {},
    {"&Ntilde;",   {0xC3, 0x91}, 8},
    {"&Nu;",       {0xCE, 0x9D}, 4},
    {},
    {"&Ograve;",   {0xC3, 0x92}, 8},
    {"&Oacute;",   {0xC3, 0x93}, 8},
    {"&Ocirc;",    {0xC3, 0x94}, 7},
    {"&Otilde;",   {0xC3, 0x95}, 8},
    {"&Ouml;",     {0xC3, 0x96}, 6},
    {"&Oslash;",   {0xC3, 0x98}, 8},
    {"&OElig;",    {0xC5, 0x92}, 7},
    {"&Omicron;",  {0xCE, 0x9F}, 9},
    {"&Omega;",    {0xCE, 0xA9}, 7},
    {},
    {"&Pi;",       {0xCE, 0xA0}, 4},
    {"&Phi;",      {0xCE, 0xA6}, 5},
    {"&Psi;",      {0xCE, 0xA8}, 5},
    {"&Prime;",    {0xE2, 0x80, 0xB3}, 7},
    {},
    {},
    {"&Rho;",      {0xCE, 0xA1}, 5},
    {},
    {"&Scaron;",   {0xC5, 0xA0}, 8},
    {"&Sigma;",    {0xCE, 0xA3}, 7},
    {},
    {"&THORN;",    {0xC3, 0x9E}, 7},
    {"&Theta;",    {0xCE, 0x98}, 7},
    {"&Tau;",      {0xCE, 0xA4}, 5},
    {},
    {"&Ugrave;",   {0xC3, 0x99}, 8},
    {"&Uacute;",   {0xC3, 0x9A}, 8},
    {"&Ucirc;",    {0xC3, 0x9B}, 7},
    {"&Uuml;",     {0xC3, 0x9C}, 6},
    {"&Upsilon;",  {0xCE, 0xA5}, 9},
    {},
    {},
    {},
    {"&Xi;",       {0xCE, 0x9E}, 4},
    {},
    {"&Yacute;",   {0xC3, 0x9D}, 8},
    {"&Yuml;",     {0xC5, 0xB8}, 6},
    {},
    {"&Zeta;",     {0xCE, 0x96}, 6},
    {},
    {"&acute;",    {0xC2, 0xB4}, 7},
    {"&agrave;",   {0xC3, 0xA0}, 8},
    {"&aacute;",   {0xC3, 0xA1}, 8},
    {"&acirc;",    {0xC3, 0xA2}, 7},
    {"&atilde;",   {0xC3, 0xA3}, 8},
    {"&auml;",     {0xC3, 0xA4}, 6},
    {"&aring;",    {0xC3, 0xA5}, 7},
    {"&aelig;",    {0xC3, 0xA6}, 7},
    {"&amp;",      {0x26}, 5},
    {"&alpha;",    {0xCE, 0xB1}, 7},
    {"&alefsym;",  {0xE2, 0x84, 0xB5}, 9},
    {"&ang;",      {0xE2, 0x88, 0xA0}, 5},
    {"&and;",      {0xE2, 0x88, 0xA7}, 5},
    {"&asymp;",    {0xE2, 0x89, 0x88}, 7},
    {"&apos;",     {0x27}, 6},
    {},
    {"&brvbar;",   {0xC2, 0xA6}, 8},
    {"&bdquo;",    {0xE2, 0x80, 0x9E}, 7},
    {"&beta;",     {0xCE, 0xB2}, 6},
    {"&bull;",     {0xE2, 0x80, 0xA2}, 6},
    {},
    {"&cent;",     {0xC2, 0xA2}, 6},
    {"&curren;",   {0xC2, 0xA4}, 8},
    {"&copy;",     {0xC2, 0xA9}, 6},
    {"&cedil;",    {0xC2, 0xB8}, 7},
    {"&ccedil;",   {0xC3, 0xA7}, 8},
    {"&circ;",     {0xCB, 0x86}, 6},
    {"&chi;",      {0xCF, 0x87}, 5},
    {"&crarr;",    {0xE2, 0x86, 0xB5}, 7},
    {"&cap;",      {0xE2, 0x88, 0xA9}, 5},
    {"&cup;",      {0xE2, 0x88, 0xAA}, 5},
    {"&cong;",     {0xE2, 0x89, 0x85}, 6},
    {"&clubs;",    {0xE2, 0x99, 0xA3}, 7},
    {},
    {"&deg;",      {0xC2, 0xB0}, 5},
    {"&divide;",   {0xC3, 0xB7}, 8},
    {"&dagger;",   {0xE2, 0x80, 0xA0}, 8},
    {"&delta;",    {0xCE, 0xB4}, 7},
    {"&darr;",     {0xE2, 0x86, 0x93}, 6},
    {"&dArr;",     {0xE2, 0x87, 0x93}, 6},
    {"&diams;",    {0xE2, 0x99, 0xA6}, 7},
    {},
    {"&egrave;",   {0xC3, 0xA8}, 8},
    {"&eacute;",   {0xC3, 0xA9}, 8},
    {"&ecirc;",    {0xC3, 0xAA}, 7},
    {"&euml;",     {0xC3, 0xAB}, 6},
    {"&eth;",      {0xC3, 0xB0}, 5},
    {"&ensp;",     {0xE2, 0x80, 0x82}, 6},
    {"&emsp;",     {0xE2, 0x80, 0x83}, 6},
    {"&euro;",     {0xE2, 0x82, 0xAC}, 6},
    {"&epsilon;",  {0xCE, 0xB5}, 9},
    {"&eta;",      {0xCE, 0xB7}, 5},
    {"&exist;",    {0xE2, 0x88, 0x83}, 7},
    {"&empty;",    {0xE2, 0x88, 0x85}, 7},
    {"&equiv;",    {0xE2, 0x89, 0xA1}, 7},
    {},
    {"&frac14;",   {0xC2, 0xBC}, 8},
    {"&frac12;",   {0xC2, 0xBD}, 8},
    {"&frac34;",   {0xC2, 0xBE}, 8},
    {"&fnof;",     {0xC6, 0x92}, 6},
    {"&frasl;",    {0xE2, 0x81, 0x84}, 7},
    {"&forall;",   {0xE2, 0x88, 0x80}, 8},
    {},
    {"&gt;",       {0x3E}, 4},
    {"&gamma;",    {0xCE, 0xB3}, 7},
    {"&ge;",       {0xE2, 0x89, 0xA5}, 4},
    {},
    {"&hellip;",   {0xE2, 0x80, 0xA6}, 8},
    {"&harr;",     {0xE2, 0x86, 0x94}, 6},
    {"&hArr;",     {0xE2, 0x87, 0x94}, 6},
    {"&hearts;",   {0xE2, 0x99, 0xA5}, 8},
    {},
    {"&iexcl;",    {0xC2, 0xA1}, 7},
    {"&iquest;",   {0xC2, 0xBF}, 8},
    {"&igrave;",   {0xC3, 0xAC}, 8},
    {"&iacute;",   {0xC3, 0xAD}, 8},
    {"&icirc;",    {0xC3, 0xAE}, 7},
    {"&iuml;",     {0xC3, 0xAF}, 6},
    {"&iota;",     {0xCE, 0xB9}, 6},
    {"&image;",    {0xE2, 0x84, 0x91}, 7},
    {"&isin;",     {0xE2, 0x88, 0x88}, 6},
    {"&infin;",    {0xE2, 0x88, 0x9E}, 7},
    {"&int;",      {0xE2, 0x88, 0xAB}, 5},
    {},
    {},
    {"&kappa;",    {0xCE, 0xBA}, 7},
    {},
    {"&laquo;",    {0xC2, 0xAB}, 7},
    {"&lt;",       {0x3C}, 4},
    {"&lrm;",      {0xE2, 0x80, 0x8E}, 5},
    {"&lsquo;",    {0xE2, 0x80, 0x98}, 7},
    {"&ldquo;",    {0xE2, 0x80, 0x9C}, 7},
    {"&lsaquo;",   {0xE2, 0x80, 0xB9}, 8},
    {"&lambda;",   {0xCE, 0xBB}, 8},
    {"&larr;",     {0xE2, 0x86, 0x90}, 6},
    {"&lArr;",     {0xE2, 0x87, 0x90}, 6},
    {"&lowast;",   {0xE2, 0x88, 0x97}, 8},
    {"&le;",       {0xE2, 0x89, 0xA4}, 4},
    {"&lceil;",    {0xE2, 0x8C, 0x88}, 7},
    {"&lfloor;",   {0xE2, 0x8C, 0x8A}, 8},
    {"&lang;",     {0xE2, 0x8C, 0xA9}, 6},
    {"&loz;",      {0xE2, 0x97, 0x8A}, 5},
    {},
    {"&macr;",     {0xC2, 0xAF}, 6},
    {"&micro;",    {0xC2, 0xB5}, 7},
    {"&middot;",   {0xC2, 0xB7}, 8},
    {"&mdash;",    {0xE2, 0x80, 0x94}, 7},
    {"&mu;",       {0xCE, 0xBC}, 4},
    {"&minus;",    {0xE2, 0x88, 0x92}, 7},
    {},
    {"&nbsp;",     {0xC2, 0xA0}, 6},
    {"&not;",      {0xC2, 0xAC}, 5},
    {"&ntilde;",   {0xC3, 0xB1}, 8},
    {"&ndash;",    {0xE2, 0x80, 0x93}, 7},
    {"&nu;",       {0xCE, 0xBD}, 4},
    {"&nabla;",    {0xE2, 0x88, 0x87}, 7},
    {"&notin;",    {0xE2, 0x88, 0x89}, 7},
    {"&ni;",       {0xE2, 0x88, 0x8B}, 4},
    {"&ne;",       {0xE2, 0x89, 0xA0}, 4},
    {"&nsub;",     {0xE2, 0x8A, 0x84}, 6},
    {},
    {"&ordf;",     {0xC2, 0xAA}, 6},
    {"&ordm;",     {0xC2, 0xBA}, 6},
    {"&ograve;",   {0xC3, 0xB2}, 8},
    {"&oacute;",   {0xC3, 0xB3}, 8},
    {"&ocirc;",    {0xC3, 0xB4}, 7},
    {"&otilde;",   {0xC3, 0xB5}, 8},
    {"&ouml;",     {0xC3, 0xB6}, 6},
    {"&oslash;",   {0xC3, 0xB8}, 8},
    {"&oelig;",    {0xC5, 0x93}, 7},
    {"&omicron;",  {0xCE, 0xBF}, 9},
    {"&omega;",    {0xCF, 0x89}, 7},
    {"&oline;",    {0xE2, 0x80, 0xBE}, 7},
    {"&or;",       {0xE2, 0x88, 0xA8}, 4},
    {"&oplus;",    {0xE2, 0x8A, 0x95}, 7},
    {"&otimes;",   {0xE2, 0x8A, 0x97}, 8},
    {},
    {"&pound;",    {0xC2, 0xA3}, 7},
    {"&plusmn;",   {0xC2, 0xB1}, 8},
    {"&para;",     {0xC2, 0xB6}, 6},
    {"&permil;",   {0xE2, 0x80, 0xB0}, 8},
    {"&pi;",       {0xCF, 0x80}, 4},
    {"&phi;",      {0xCF, 0x86}, 5},
    {"&psi;",      {0xCF, 0x88}, 5},
    {"&piv;",      {0xCF, 0x96}, 5},
    {"&prime;",    {0xE2, 0x80, 0xB2}, 7},
    {"&part;",     {0xE2, 0x88, 0x82}, 6},
    {"&prod;",     {0xE2, 0x88, 0x8F}, 6},
    {"&prop;",     {0xE2, 0x88, 0x9D}, 6},
    {"&perp;",     {0xE2, 0x8A, 0xA5}, 6},
    {},
    {"&quot;",     {0x22}, 6},
    {},
    {"&reg;",      {0xC2, 0xAE}, 5},
    {"&raquo;",    {0xC2, 0xBB}, 7},
    {"&rlm;",      {0xE2, 0x80, 0x8F}, 5},
    {"&rsquo;",    {0xE2, 0x80, 0x99}, 7},
    {"&rdquo;",    {0xE2, 0x80, 0x9D}, 7},
    {"&rsaquo;",   {0xE2, 0x80, 0xBA}, 8},
    {"&rho;",      {0xCF, 0x81}, 5},
    {"&real;",     {0xE2, 0x84, 0x9C}, 6},
    {"&rarr;",     {0xE2, 0x86, 0x92}, 6},
    {"&rArr;",     {0xE2, 0x87, 0x92}, 6},
    {"&radic;",    {0xE2, 0x88, 0x9A}, 7},
    {"&rceil;",    {0xE2, 0x8C, 0x89}, 7},
    {"&rfloor;",   {0xE2, 0x8C, 0x8B}, 8},
    {"&rang;",     {0xE2, 0x8C, 0xAA}, 6},
    {},
    {"&sect;",     {0xC2, 0xA7}, 6},
    {"&shy;",      {0xC2, 0xAD}, 5},
    {"&sup2;",     {0xC2, 0xB2}, 6},
    {"&sup3;",     {0xC2, 0xB3}, 6},
    {"&sup1;",     {0xC2, 0xB9}, 6},
    {"&szlig;",    {0xC3, 0x9F}, 7},
    {"&scaron;",   {0xC5, 0xA1}, 8},
    {"&sbquo;",    {0xE2, 0x80, 0x9A}, 7},
    {"&sigmaf;",   {0xCF, 0x82}, 8},
    {"&sigma;",    {0xCF, 0x83}, 7},
    {"&sum;",      {0xE2, 0x88, 0x91}, 5},
    {"&sim;",      {0xE2, 0x88, 0xBC}, 5},
    {"&sub;",      {0xE2, 0x8A, 0x82}, 5},
    {"&sup;",      {0xE2, 0x8A, 0x83}, 5},
    {"&sube;",     {0xE2, 0x8A, 0x86}, 6},
    {"&supe;",     {0xE2, 0x8A, 0x87}, 6},
    {"&sdot;",     {0xE2, 0x8B, 0x85}, 6},
    {"&spades;",   {0xE2, 0x99, 0xA0}, 8},
    {},
    {"&times;",    {0xC3, 0x97}, 7},
    {"&thorn;",    {0xC3, 0xBE}, 7},
    {"&tilde;",    {0xCB, 0x9C}, 7},
    {"&thinsp;",   {0xE2, 0x80, 0x89}, 8},
    {"&theta;",    {0xCE, 0xB8}, 7},
    {"&tau;",      {0xCF, 0x84}, 5},
    {"&thetasym;", {0xCF, 0x91}, 10},
    {"&trade;",    {0xE2, 0x84, 0xA2}, 7},
    {"&there4;",   {0xE2, 0x88, 0xB4}, 8},
    {},
    {"&uml;",      {0xC2, 0xA8}, 5},
    {"&ugrave;",   {0xC3, 0xB9}, 8},
    {"&uacute;",   {0xC3, 0xBA}, 8},
    {"&ucirc;",    {0xC3, 0xBB}, 7},
    {"&uuml;",     {0xC3, 0xBC}, 6},
    {"&upsilon;",  {0xCF, 0x85}, 9},
    {"&upsih;",    {0xCF, 0x92}, 7},
    {"&uarr;",     {0xE2, 0x86, 0x91}, 6},
    {"&uArr;",     {0xE2, 0x87, 0x91}, 6},
    {},
    {},
    {"&weierp;",   {0xE2, 0x84, 0x98}, 8},
    {},
    {"&xi;",       {0xCE, 0xBE}, 4},
    {},
    {"&yen;",      {0xC2, 0xA5}, 5},
    {"&yacute;",   {0xC3, 0xBD}, 8},
    {"&yuml;",     {0xC3, 0xBF}, 6},
    {},
    {"&zwnj;",     {0xE2, 0x80, 0x8C}, 6},
    {"&zwj;",      {0xE2, 0x80, 0x8D}, 5},
    {"&zeta;",     {0xCE, 0xB6}, 6},
    {}
};

static size_t entity_jump_upper[26] = {
      0,   9,  11,  14, // ABCD
     17,  25,  26,  28, // EFGH
     29,  35,  36,  38, // IJKL
     40,  42,  45,  55, // MNOP
     60,  61,  63,  66, // QRST
     70,  76,  77,  78, // UVWX
     80,  83            // YZ
};

static size_t entity_jump_lower[26] = {
     85, 101, 106, 119, // abcd
    127, 141, 148, 152, // efgh
    157, 169, 170, 172, // ijkl
    188, 195, 206, 222, // mnop
    236, 238, 253, 272, // qrst
    282, 292, 293, 295, // uvwx
    297, 301            // yz
};

static const char *entity_table_lookup(const char *text, int *length, struct EntityTableEntry *table)
{
    for (; table->name[0]; ++table) {
        if (strncmp(text, table->name, table->namelen) == 0) {
            if (length)
                *length = table->namelen;

            return (const char *)table->result;
        }
    }

    return NULL;
}

// Based on purple_markup_unescape_entity
const char *flist_html_markup_unescape_entity_utf8(const char *text, int *length)
{
    const char *pln;
    int pound;
    char temp[2];

    if (!text || *text != '&')
        return NULL;

    if (text[1] >= 'A' && text[1] <= 'Z')
        pln = entity_table_lookup(text, length, entity_table_utf8 + entity_jump_upper[text[1] - 'A']);
    else if (text[1] >= 'a' && text[1] <= 'z')
        pln = entity_table_lookup(text, length, entity_table_utf8 + entity_jump_lower[text[1] - 'a']);
    else if (text[1] == '#' &&
        (sscanf(text, "&#%u%1[;]", &pound, temp) == 2 ||
         sscanf(text, "&#x%x%1[;]", &pound, temp) == 2) &&
        pound != 0) {
            static char buf[7];
            int buflen = g_unichar_to_utf8((gunichar)pound, buf);
            int len;
            buf[buflen] = '\0';
            pln = buf;

            len = (*(text+2) == 'x' ? 3 : 2);
            while(isxdigit((gint) text[len])) len++;
            if(text[len] == ';') len++;

            if (length)
                *length = len;
    }
    else
        return NULL;

    return pln;
}

// Based on purple_unescape_text
char *flist_html_unescape_utf8(const char *in)
{
    GString *ret;
    const char *c = in;

    if (in == NULL)
        return NULL;

    ret = g_string_new("");
    while (*c) {
        int len;
        const char *ent;

        if ((ent = flist_html_markup_unescape_entity_utf8(c, &len)) != NULL) {
            g_string_append(ret, ent);
            c += len;
        } else {
            g_string_append_c(ret, *c);
            c++;
        }
    }

    return g_string_free(ret, FALSE);
}
