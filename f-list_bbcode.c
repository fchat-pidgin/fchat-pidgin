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
#include "f-list_bbcode.h"

typedef struct ParserVars_ {
    FListAccount *fla;
    PurpleConversation *convo;
} ParserVars;

typedef gchar *(*tag_format)(ParserVars *vars, const gchar *ts, const gchar *inner);
typedef struct BBCodeTag_ {
    const gchar *tag; /* the tag */
    gboolean nesting; /* whether or not other tags are allowed to nest inside this one*/
    gboolean argument; /* whether or not the tag accepts an argument */
    tag_format format; /* this is how we format the string */
} BBCodeTag;

gchar *flist_escape_attribute(const gchar *to_escape) {
    GString *ret = g_string_new(NULL);
    const gchar *current, *next;
    current = to_escape;
    while((next = strchr(current, '\"'))) {
        g_string_append_len(ret, current, (gsize) (next - current));
        g_string_append(ret, "\"");
        current = next + 1;
    }
    g_string_append(ret, current);
    return g_string_free(ret, FALSE);
}

//TODO: replace all of these with CSS
static gchar *format_bold(ParserVars *vars, const gchar *ts, const gchar *inner) {
    return g_strdup_printf("<b>%s</b>", inner);
}
static gchar *format_italic(ParserVars *vars, const gchar *ts, const gchar *inner) {
    return g_strdup_printf("<i>%s</i>", inner);
}
static gchar *format_strike(ParserVars *vars, const gchar *ts, const gchar *inner) {
    return g_strdup_printf("<s>%s</s>", inner);
}
static gchar *format_underline(ParserVars *vars, const gchar *ts, const gchar *inner) {
    return g_strdup_printf("<u>%s</u>", inner);
}
static gchar *format_url(ParserVars *vars, const gchar *ts, const gchar *inner) {
    gchar *escaped, *ret;
    if(strlen(ts) > 0) {
        escaped = flist_escape_attribute(ts);
    } else {
        escaped = flist_escape_attribute(inner);
    }
    ret = g_strdup_printf("<a href=\"%s\">%s</a>", escaped, inner);
    g_free(escaped);
    return ret;
}
static gchar *format_color(ParserVars *vars, const gchar *ts, const gchar *inner) {
    if(strlen(ts) > 0) {
        gchar *escaped, *ret;
        escaped = flist_escape_attribute(ts);
        ret = g_strdup_printf("<font color=\"%s\">%s</font>", escaped, inner);
        g_free(escaped);
        return ret;
    }
    return g_strdup(inner);
}
static gchar *format_user(ParserVars *vars, const gchar *ts, const gchar *inner) {
    const gchar *url_pattern = "<a href=\"http://www.f-list.net/c/%s\">%s</a>";
    gchar *lower = g_utf8_strdown(inner, -1);
    gchar *ret = g_strdup_printf(url_pattern, purple_url_encode(lower), inner);
    g_free(lower);
    return ret;
}
static gchar *format_icon(ParserVars *vars, const gchar *ts, const gchar *inner) {
    gchar *lower = g_utf8_strdown(inner, -1);
    gchar *ret;
    if(vars->fla && vars->convo) {
        gchar *smiley = g_strdup_printf("[icon]%s[/icon]", purple_url_encode(lower));
        ret = g_strdup_printf("%s<a href=\"http://www.f-list.net/c/%s\">(%s)</a>", smiley, purple_url_encode(lower), inner);
        flist_fetch_emoticon(vars->fla, smiley, lower, vars->convo);
        g_free(smiley);
    } else {
        ret = g_strdup_printf("<a href=\"http://www.f-list.net/c/%s\">%s</a>", purple_url_encode(lower), inner);
    }
    g_free(lower);
    return ret;
}

static gchar *format_channel_real(ParserVars *vars, const gchar *name, const gchar *title) {
    PurpleAccount *pa;
    gchar *unescaped, *ret;
    GString *gs;
    if(!vars->fla) return g_strdup(name);
    unescaped = purple_unescape_html(name);

    gs = g_string_new(NULL);
    pa = vars->fla->pa;
    g_string_append(gs, "<a href=\"flistc://");
    g_string_append(gs, purple_url_encode(flist_serialize_account(pa)));
    g_string_append_c(gs, '/');
    g_string_append(gs, purple_url_encode(unescaped));
    g_string_append(gs, "\">(Channel) ");
    g_string_append(gs, title);
    g_string_append(gs, "</a>");
    ret = g_string_free(gs, FALSE);

    g_free(unescaped);
    return ret;
}
static gchar *format_channel(ParserVars *vars, const gchar *ts, const gchar *inner) {
    return format_channel_real(vars, inner, inner);
}
static gchar *format_session(ParserVars *vars, const gchar *ts, const gchar *inner) {
    return format_channel_real(vars, inner, ts ? ts : inner);
}

static GHashTable *tag_table;

BBCodeTag tag_bold = { "b", TRUE, FALSE, format_bold };
BBCodeTag tag_italic = { "i", TRUE, FALSE, format_italic };
BBCodeTag tag_underline = { "u", TRUE, FALSE, format_underline };
BBCodeTag tag_strike = { "s", TRUE, FALSE, format_strike };
BBCodeTag tag_url = { "url", FALSE, TRUE, format_url };
BBCodeTag tag_color = { "color", TRUE, TRUE, format_color };
BBCodeTag tag_user = { "user", FALSE, FALSE, format_user };
BBCodeTag tag_icon = { "icon", FALSE, FALSE, format_icon };
BBCodeTag tag_channel = { "channel", FALSE, FALSE, format_channel };
BBCodeTag tag_session = { "session", FALSE, FALSE, format_session };

typedef struct BBCodeStack_ BBCodeStack;
struct BBCodeStack_ {
    BBCodeStack *next;
    BBCodeTag *tag;
    gchar *tag_argument;
    GString *ret;
    /* the raw tag */
    const gchar *raw_tag;
    gsize raw_tag_len;
};

static gboolean bbcode_parse_tag(const gchar *raw_tag, gsize raw_tag_len,
        BBCodeTag *current_tag, BBCodeTag **ret_tag,
        gchar **ret_tag_argument, gboolean *ret_close_tag) {
    BBCodeTag *bbtag;
    const gchar *start = raw_tag + 1, *end = raw_tag + raw_tag_len - 1;
    const gchar *split = g_strstr_len(raw_tag, raw_tag_len, "=");
    gchar *tag, *arg;
    gboolean close_tag;

    if(raw_tag[1] == '/') {
        start += 1;
        close_tag = TRUE;
    } else {
        close_tag = FALSE;
    }

    if(split) {
        tag = g_strndup(start, (gsize) (split - start));
        arg = g_strndup(split + 1, (gsize) (end - (split + 1)));
    } else {
        tag = g_strndup(start, (gsize) (end - start));
        arg = g_strdup("");
    }

    bbtag = g_hash_table_lookup(tag_table, tag);
    g_free(tag);
    if(bbtag) {
        if(close_tag && bbtag == current_tag) { /* we're closing a tag ... */
            *ret_close_tag = TRUE;
            g_free(arg);
            return TRUE;
        }
        if(!close_tag && (!current_tag || current_tag->nesting)) { /* we're opening a tag ... */
            *ret_tag = bbtag;
            *ret_tag_argument = arg;
            *ret_close_tag = FALSE;
            return TRUE;
        }
    }
    
    g_free(arg);
    return FALSE;
}

gchar *flist_bbcode_to_html_real(FListAccount *fla, PurpleConversation *convo, const gchar *bbcode, gboolean strip) {
    ParserVars vars = {fla, convo};
    BBCodeStack stack_base = {NULL, NULL, NULL, NULL, NULL, 0};
    BBCodeStack *stack = &stack_base, *stack_tmp;
    const gchar *current, *open, *close;
    const gchar *raw_tag; gsize raw_tag_len;
    BBCodeTag *tag;
    gchar *tag_argument;
    gboolean close_tag;

    stack->ret = g_string_new(NULL);

    current = bbcode;
    while(TRUE) {
        open = strchr(current, '['); if(!open) break;
        close = strchr(open, ']'); if(!close) break;
        close += 1; /* add in the ']' */
        raw_tag = open;
        raw_tag_len = close - open;

        /* append the raw text before the tag */
        g_string_append_len(stack->ret, current, (gsize) (open - current));
        current = close;
        
        if(bbcode_parse_tag(raw_tag, raw_tag_len, stack->tag, &tag, &tag_argument, &close_tag)) {
            if(!close_tag) { /* we have a new tag! push it onto the stack */
                stack_tmp = g_new(BBCodeStack, 1);
                stack_tmp->next = stack;
                stack = stack_tmp;
                stack->raw_tag = raw_tag;
                stack->raw_tag_len = raw_tag_len;
                stack->tag = tag;
                stack->tag_argument = tag_argument;
                stack->ret = g_string_new(NULL);
            } else { /* we are closing a tag! pop it off of the stack */
                gchar *inner = g_string_free(stack->ret, FALSE);
                gchar *final = !strip ? stack->tag->format(&vars, stack->tag_argument, inner) : g_strdup(inner);
                stack_tmp = stack;
                stack = stack->next;
                g_string_append(stack->ret, final);
                g_free(stack_tmp->tag_argument);
                g_free(stack_tmp);
                g_free(inner);
                g_free(final);
            }
        } else { /* this tag is not valid in this context! ignore it */
            g_string_append_len(stack->ret, raw_tag, (gsize) (raw_tag_len));
        }
    }

    while(stack->next) {
        gchar *final = g_string_free(stack->ret, FALSE);
        stack_tmp = stack;
        stack = stack->next;
        g_string_append_len(stack->ret, stack_tmp->raw_tag, stack_tmp->raw_tag_len); /* append the unclosed tag as raw text */
        g_string_append(stack->ret, final); /* append the parsed interior of the unclosed tag */
        g_free(stack_tmp->tag_argument);
        g_free(stack_tmp);
        g_free(final);
    }

    /* append everything past the close of the last tag */
    g_string_append(stack->ret, current);
    return g_string_free(stack->ret, FALSE);
}

gchar *flist_bbcode_to_html(FListAccount *fla, PurpleConversation *convo, const gchar *bbcode) {
    return flist_bbcode_to_html_real(fla, convo, bbcode, FALSE);
}

gchar *flist_bbcode_strip(const gchar *bbcode) {
    return flist_bbcode_to_html_real(NULL, NULL, bbcode, TRUE);
}

gchar *flist_strip_crlf(const gchar *to_strip) {
    GString *ret = g_string_new(NULL);
    const gchar *start = to_strip;
    
    while(*to_strip) {
        if(*to_strip == '\r' || *to_strip == '\n') {
            g_string_append_len(ret, start, to_strip - start);
            start = to_strip + 1;
        }
        to_strip++;
    }
    g_string_append_len(ret, start, to_strip - start);
    
    return g_string_free(ret, FALSE);
}

void flist_bbcode_init() {
    tag_table = g_hash_table_new(g_str_hash, g_str_equal);
    g_hash_table_insert(tag_table, "b", &tag_bold);
    g_hash_table_insert(tag_table, "i", &tag_italic);
    g_hash_table_insert(tag_table, "u", &tag_underline);
    g_hash_table_insert(tag_table, "s", &tag_strike);
    g_hash_table_insert(tag_table, "url", &tag_url);
    g_hash_table_insert(tag_table, "color", &tag_color);
    g_hash_table_insert(tag_table, "user", &tag_user);
    g_hash_table_insert(tag_table, "icon", &tag_icon);
    g_hash_table_insert(tag_table, "channel", &tag_channel);
    g_hash_table_insert(tag_table, "session", &tag_session);
}
