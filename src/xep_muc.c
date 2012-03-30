/*
 * Copyright (c) 2012 Tom Wambold <tom5760@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file xep_muc.c
 * Implements XEP-0045 Multi-User Chat
 */

#include <uthash.h>
#include <utlist.h>

#include "jid.h"
#include "log.h"
#include "utils.h"
#include "xmp3_module.h"
#include "xmpp_client.h"
#include "xmpp_im.h"
#include "xmpp_server.h"
#include "xmpp_stanza.h"

static const char *DEFAULT_DOMAIN = "conference.localhost";
static const char *XMPP_STANZA_TYPE_GROUPCHAT = "groupchat";
static const char *XMPP_STANZA_TYPE_UNAVAILABLE = "unavailable";

/** Represents a particular client in a chat room. */
struct room_client {
    /** The user's nickname. */
    char *nickname;

    /** The jid of the client. */
    struct jid *client_jid;

    /** @{ These are kept in a doubly-linked list. */
    struct room_client *prev;
    struct room_client *next;
    /** @} */
};

/** Represents a chat room. */
struct room {
    /** The name of the room. */
    char *name;

    /** The JID of the room. */
    struct jid *jid;

    /** A list of clients in the room. */
    struct room_client *clients;

    /** These are kept in a hash table. */
    UT_hash_handle hh;
};

/** Represence the MUC component. */
struct xep_muc {
    struct xmpp_server *server;
    struct room *rooms;
    struct jid *jid;
};

/* Forward declarations. */
static void* xep_muc_new(void);
static void xep_muc_del(void *data);
static bool xep_muc_conf(void *data, const char *key, const char *value);
static bool xep_muc_start(void *data, struct xmpp_server *server);
static bool xep_muc_stop(void *data);

static bool stanza_handler(struct xmpp_stanza *stanza,
                           struct xmpp_server *server, void *data);

static bool handle_message(struct xmpp_stanza *stanza, struct xep_muc *muc);

static bool handle_presence(struct xmpp_stanza *stanza, struct xep_muc *muc);

static bool handle_iq(struct xmpp_stanza *stanza, struct xep_muc *muc);

static bool enter_room_presence(const char *search_room,
                                struct xmpp_stanza *stanza,
                                struct xep_muc *muc);

static bool leave_room_presence(const char *search_room,
                                struct xmpp_stanza *stanza,
                                struct xep_muc *muc);

static void client_disconnect(struct xmpp_client *client, void *data);
static bool leave_room(struct xep_muc *muc, struct room *room,
                       struct room_client *room_client);

static struct room* room_new(const struct xep_muc *muc, const char *name);
static void room_del(struct room *room);

static struct room_client* room_client_new(const char *nickname,
                                           const struct jid *client_jid);
static void room_client_del(struct room_client *room_client);

static bool handle_items_query(struct xmpp_stanza *stanza,
                               struct xep_muc *muc);

static bool handle_info_query(struct xmpp_stanza *stanza, struct xep_muc *muc);

static void send_nickname_conflict(struct xep_muc *muc,
                                   const struct jid *room_jid, const char *to,
                                   const char *from);
static void send_presence_broadcast(struct xep_muc *muc, struct room *room,
                                    const char *to, const char *from,
                                    const char *nickname);

/* Global module definition */
struct xmp3_module XMP3_MODULE = {
    .mod_new = xep_muc_new,
    .mod_del = xep_muc_del,
    .mod_conf = xep_muc_conf,
    .mod_start = xep_muc_start,
    .mod_stop = xep_muc_stop,
};

static void* xep_muc_new(void) {
    struct xep_muc *muc = calloc(1, sizeof(*muc));
    check_mem(muc);

    muc->jid = jid_new();
    jid_set_domain(muc->jid, DEFAULT_DOMAIN);

    return muc;
}

static void xep_muc_del(void *data) {
    struct xep_muc *muc = data;

    struct room *room, *room_tmp;
    HASH_ITER(hh, muc->rooms, room, room_tmp) {
        HASH_DEL(muc->rooms, room);

        struct room_client *r_client, *r_client_tmp;
        DL_FOREACH_SAFE(room->clients, r_client, r_client_tmp) {
            DL_DELETE(room->clients, r_client);
            room_client_del(r_client);
        }
        room_del(room);
    }
    jid_del(muc->jid);
    free(muc);
}

static bool xep_muc_conf(void *data, const char *key, const char *value) {
    struct xep_muc *muc = data;

    if (strcmp(key, "domain") == 0) {
        jid_set_domain(muc->jid, value);

    } else {
        log_err("Unknown configuration key '%s'", key);
        return false;
    }

    return true;
}

static bool xep_muc_start(void *data, struct xmpp_server *server) {
    struct xep_muc *muc = data;

    muc->server = server;

    jid_set_local(muc->jid, "*");
    jid_set_resource(muc->jid, "*");
    xmpp_server_add_stanza_route(server, muc->jid, stanza_handler, muc);
    jid_set_local(muc->jid, NULL);
    jid_set_resource(muc->jid, NULL);

    xmpp_server_add_disco_item(server, "Public Chatrooms", muc->jid);

    return true;
};

static bool xep_muc_stop(void *data) {
    struct xep_muc *muc = data;

    jid_set_local(muc->jid, "*");
    jid_set_resource(muc->jid, "*");

    xmpp_server_del_stanza_route(muc->server, muc->jid, stanza_handler, muc);
    xmpp_server_del_disco_item(muc->server, "Public Chatrooms", muc->jid);

    jid_set_local(muc->jid, NULL);
    jid_set_resource(muc->jid, NULL);

    return true;
}

static bool stanza_handler(struct xmpp_stanza *stanza,
                           struct xmpp_server *server, void *data) {
    struct xep_muc *muc = (struct xep_muc*)data;

    const char *from = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_FROM);
    check(from != NULL, "MUC message without from attribute.");

    if (strcmp(xmpp_stanza_name(stanza), XMPP_STANZA_MESSAGE) == 0) {
        debug("MUC Message");
        return handle_message(stanza, muc);
    } else if (strcmp(xmpp_stanza_name(stanza), XMPP_STANZA_PRESENCE) == 0) {
        debug("MUC Presence");
        return handle_presence(stanza, muc);
    } else if (strcmp(xmpp_stanza_name(stanza), XMPP_STANZA_IQ) == 0) {
        debug("MUC IQ");
        return handle_iq(stanza, muc);
    } else {
        log_warn("Unknown MUC stanza");
        return false;
    }

error:
    return false;
}

static bool handle_message(struct xmpp_stanza *stanza, struct xep_muc *muc) {
    const char *type = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TYPE);
    check(type != NULL && strcmp(type, XMPP_STANZA_TYPE_GROUPCHAT) == 0,
          "MUC message stanza type other than groupchat.");

    char *to = strdup(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TO));
    check(to != NULL, "MUC message without to attribute.");

    struct jid *to_jid = jid_new_from_str(to);
    struct room *room = NULL;
    HASH_FIND_STR(muc->rooms, jid_local(to_jid), room);
    jid_del(to_jid);
    check(room != NULL, "MUC message to non-existent room.");

    char *from = strdup(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_FROM));
    check(from != NULL, "MUC message without from attribute.");

    struct jid *from_jid = jid_new_from_str(from);
    struct room_client *room_client = NULL;
    DL_FOREACH(room->clients, room_client) {
        if (jid_cmp(room_client->client_jid, from_jid) == 0) {
            break;
        }
    }
    jid_del(from_jid);
    check(room_client != NULL, "MUC message to unjoined room.");

    /* We need the "from" field of the stanza we send to be from the nickname
     * of the user who sent it. */
    struct jid *nick_jid = jid_new_from_jid(room->jid);
    jid_set_resource(nick_jid, room_client->nickname);
    char *nick_jid_str = jid_to_str(nick_jid);
    jid_del(nick_jid);

    xmpp_stanza_set_attr(stanza, XMPP_STANZA_ATTR_FROM, nick_jid_str);

    struct room_client *room_client_tmp;
    DL_FOREACH_SAFE(room->clients, room_client, room_client_tmp) {
        char *to_str = jid_to_str(room_client->client_jid);
        xmpp_stanza_set_attr(stanza, XMPP_STANZA_ATTR_TO, to_str);
        xmpp_server_route_stanza(muc->server, stanza);
    }

    /* Set the "to" and "from" back to their orignal values. */
    xmpp_stanza_set_attr(stanza, XMPP_STANZA_ATTR_TO, to);
    xmpp_stanza_set_attr(stanza, XMPP_STANZA_ATTR_FROM, from);
    return true;

error:
    return false;
}

static bool handle_presence(struct xmpp_stanza *stanza, struct xep_muc *muc) {
    const char *to = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TO);
    check(to != NULL, "MUC message without to attribute.");

    struct jid *to_jid = jid_new_from_str(to);
    check(jid_resource(to_jid) != NULL, "MUC presence has no nickname");
    const char *search_room = jid_local(to_jid);

    bool rv;
    const char *type = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TYPE);
    if (type != NULL && strcmp(type, XMPP_STANZA_TYPE_UNAVAILABLE) == 0) {
        debug("Leaving room");
        rv = leave_room_presence(search_room, stanza, muc);
    } else {
        debug("Entering room");
        rv = enter_room_presence(search_room, stanza, muc);
    }
    jid_del(to_jid);
    return rv;

error:
    return false;
}

static bool handle_iq(struct xmpp_stanza *stanza, struct xep_muc *muc) {
    check(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID) != NULL,
          "MUC IQ with no id.");

    struct xmpp_stanza *child = xmpp_stanza_children(stanza);
    check(child != NULL, "MUC IQ with no child.");

    const char *uri = xmpp_stanza_uri(child);
    check(uri != NULL, "MUC IQ with no namespace URI.");

    if (strcmp(uri, XMPP_IQ_DISCO_ITEMS_NS) == 0) {
        return handle_items_query(stanza, muc);
    } else if (strcmp(uri, XMPP_IQ_DISCO_INFO_NS) == 0) {
        return handle_info_query(stanza, muc);
    } else {
        log_err("Unknown MUC IQ namespace URI");
        return false;
    }

error:
    return false;
}

static bool enter_room_presence(const char *search_room,
                                struct xmpp_stanza *stanza,
                                struct xep_muc *muc) {
    const char *to = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TO);
    const char *from = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_FROM);

    struct jid *to_jid = jid_new_from_str(to);
    struct jid *from_jid = jid_new_from_str(from);

    const char *nickname = jid_resource(to_jid);

    struct room *room = NULL;
    HASH_FIND_STR(muc->rooms, search_room, room);
    if (room == NULL) {
        /* Room doesn't exist, create it. */
        debug("New room, creating");
        room = room_new(muc, search_room);
        HASH_ADD_KEYPTR(hh, muc->rooms, room->name, strlen(room->name), room);
    } else {
        /* Room exists, check to make sure this nickname isn't used already. */
        struct room_client *room_client;
        DL_FOREACH(room->clients, room_client) {
            if (strncmp(nickname, room_client->nickname,
                        JID_PART_MAX_LEN) == 0) {

                /* XEP-0045 Section 7.2.9 specifies that a client requesting to
                 * join a room with a duplicate nickname should be denied. */
                debug("Duplicate nickname: %s", room_client->nickname);
                /* "to" attribute is who we got the presence from, so reverse
                 * from/to. */
                send_nickname_conflict(muc, room->jid, from, to);

                goto done;
            }
        }
    }

    struct room_client *new_client = room_client_new(nickname, from_jid);
    check_mem(new_client);

    /* If this is a local client, we need to know when they disconnect. */
    struct xmpp_client *client = xmpp_server_find_client(muc->server,
                                                         from_jid);
    if (client != NULL) {
        xmpp_server_add_client_listener(client, client_disconnect, muc);
    }

    send_presence_broadcast(muc, room, to, from, nickname);

    /* Add the client to the room client list. */
    DL_APPEND(room->clients, new_client);

done:
    jid_del(to_jid);
    jid_del(from_jid);
    return true;
}

static bool leave_room_presence(const char *search_room,
                                struct xmpp_stanza *stanza,
                                struct xep_muc *muc) {
    const char *from = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_FROM);
    struct jid *from_jid = jid_new_from_str(from);

    struct room *room = NULL;
    HASH_FIND_STR(muc->rooms, search_room, room);
    check(room != NULL, "Trying to leave a nonexistent room");

    struct room_client *room_client = NULL;
    DL_FOREACH(room->clients, room_client) {
        if (jid_cmp(room_client->client_jid, from_jid) == 0) {
            break;
        }
    }
    check(room_client != NULL, "Non member trying to leave room.");

    jid_del(from_jid);
    return leave_room(muc, room, room_client);

error:
    jid_del(from_jid);
    return false;
}

static void client_disconnect(struct xmpp_client *client, void *data) {
    struct xep_muc *muc = (struct xep_muc*)data;

    struct room *room, *room_tmp;
    HASH_ITER(hh, muc->rooms, room, room_tmp) {
        struct room_client *room_client, *room_client_tmp;
        DL_FOREACH_SAFE(room->clients, room_client, room_client_tmp) {
            if (jid_cmp(room_client->client_jid,
                        xmpp_client_jid(client)) == 0) {
                leave_room(muc, room, room_client);
            }
        }
    }
}

static bool leave_room(struct xep_muc *muc, struct room *room,
                       struct room_client *room_client) {

    jid_set_resource(room->jid, room_client->nickname);

    struct xmpp_stanza *presence = xmpp_stanza_new("presence", (const char*[]){
            XMPP_STANZA_ATTR_TYPE, "unavailable", NULL});
    xmpp_stanza_set_attr(presence, XMPP_STANZA_ATTR_FROM,
                         jid_to_str(room->jid));
    jid_set_resource(room->jid, NULL);
    xmpp_stanza_set_attr(presence, XMPP_STANZA_ATTR_TO,
                         jid_to_str(room_client->client_jid));

    struct xmpp_stanza *presence_x = xmpp_stanza_new("x", (const char*[]){
            "xmlns", "http://jabber.org/protocol/muc#user",
            NULL});
    xmpp_stanza_append_child(presence, presence_x);
    struct xmpp_stanza *presence_item = xmpp_stanza_new("item",
            (const char*[]){
            "affiliation", "member",
            "role", "none",
            NULL});
    xmpp_stanza_append_child(presence_x, presence_item);

    /* Send self-presence back to the occupant. */
    struct xmpp_stanza *status = xmpp_stanza_new("status",
        (const char*[]){"code", "110", NULL});
    xmpp_stanza_append_child(presence_x, status);

    xmpp_server_route_stanza(muc->server, presence);
    DL_DELETE(room->clients, room_client);
    room_client_del(room_client);

    xmpp_stanza_remove_child(presence_x, status);
    xmpp_stanza_del(status, true);

    /* Send the leave presence to other occupants in the room. */
    struct room_client *other_client;
    DL_FOREACH(room->clients, other_client) {
        xmpp_stanza_set_attr(presence, XMPP_STANZA_ATTR_TO,
                             jid_to_str(other_client->client_jid));
        xmpp_server_route_stanza(muc->server, presence);
    }
    xmpp_stanza_del(presence, true);

    /* If the room is empty now, delete it. */
    if (room->clients == NULL) {
        HASH_DEL(muc->rooms, room);
        room_del(room);
    }
    return true;
}

static struct room* room_new(const struct xep_muc *muc, const char *name) {
    struct room *room = calloc(1, sizeof(*room));
    check_mem(room);

    room->name = strdup(name);
    check_mem(room->name);

    room->jid = jid_new_from_jid(muc->jid);
    check_mem(room->jid);
    jid_set_local(room->jid, room->name);

    return room;
}

static void room_del(struct room *room) {
    free(room->name);
    jid_del(room->jid);

    struct room_client *room_client, *room_client_tmp;
    DL_FOREACH_SAFE(room->clients, room_client, room_client_tmp) {
        DL_DELETE(room->clients, room_client);
        room_client_del(room_client);
    }
    free(room);
}

static struct room_client* room_client_new(const char *nickname,
                                           const struct jid *client_jid) {
    struct room_client *room_client = calloc(1, sizeof(*room_client));
    check_mem(room_client);

    room_client->nickname = strdup(nickname);
    check_mem(room_client->nickname);

    room_client->client_jid = jid_new_from_jid(client_jid);
    return room_client;
}

static void room_client_del(struct room_client *room_client) {
    free(room_client->nickname);
    jid_del(room_client->client_jid);
    free(room_client);
}

static bool handle_items_query(struct xmpp_stanza *stanza,
                               struct xep_muc *muc) {
    const char *id = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID);
    const char *from = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_FROM);

    struct xmpp_stanza *result = xmpp_stanza_new("iq", (const char*[]){
            XMPP_STANZA_ATTR_ID, id,
            XMPP_STANZA_ATTR_TYPE, XMPP_STANZA_TYPE_RESULT,
            XMPP_STANZA_ATTR_TO, from,
            NULL});

    struct xmpp_stanza *query = xmpp_stanza_new("query", (const char*[]){
            "xmlns", XMPP_IQ_DISCO_ITEMS_NS, NULL});
    xmpp_stanza_append_child(result, query);

    struct room *room, *room_tmp;
    HASH_ITER(hh, muc->rooms, room, room_tmp) {
        struct xmpp_stanza *item = xmpp_stanza_new("item", (const char*[]){
                "name", room->name, NULL});
        xmpp_stanza_set_attr(item, "jid", jid_to_str(room->jid));
        xmpp_stanza_append_child(query, item);
    }

    xmpp_server_route_stanza(muc->server, result);
    xmpp_stanza_del(result, true);
    return true;
}

static bool handle_info_query(struct xmpp_stanza *stanza,
                              struct xep_muc *muc) {
    const char *id = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID);
    const char *from = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_FROM);

    struct xmpp_stanza *result = xmpp_stanza_new("iq", (const char*[]){
            XMPP_STANZA_ATTR_ID, id,
            XMPP_STANZA_ATTR_TYPE, XMPP_STANZA_TYPE_RESULT,
            XMPP_STANZA_ATTR_FROM, "conference.localhost",
            XMPP_STANZA_ATTR_TO, from,
            NULL});

    struct xmpp_stanza *query = xmpp_stanza_new("query", (const char*[]){
            "xmlns", XMPP_IQ_DISCO_INFO_NS, NULL});
    xmpp_stanza_append_child(result, query);

    struct xmpp_stanza *tmp = xmpp_stanza_new("identity", (const char*[]){
            "category", "conference",
            "name", "Public Chatrooms",
            "type", "text",
            NULL});
    xmpp_stanza_append_child(query, tmp);

    tmp = xmpp_stanza_new("feature", (const char*[]){
            "var", "http://jabber.org/protocol/muc",
            NULL});
    xmpp_stanza_append_child(query, tmp);

    tmp = xmpp_stanza_new("feature", (const char*[]){
            "var", "http://jabber.org/protocol/disco#info",
            NULL});
    xmpp_stanza_append_child(query, tmp);

    tmp = xmpp_stanza_new("feature", (const char*[]){
            "var", "http://jabber.org/protocol/disco#items",
            NULL});
    xmpp_stanza_append_child(query, tmp);

    xmpp_server_route_stanza(muc->server, result);
    xmpp_stanza_del(result, true);
    return true;
}

static void send_nickname_conflict(struct xep_muc *muc,
                                   const struct jid *room_jid, const char *to,
                                   const char *from) {
    struct xmpp_stanza *presence = xmpp_stanza_new("presence", (const char*[]){
            XMPP_STANZA_ATTR_TO, to,
            XMPP_STANZA_ATTR_FROM, from,
            XMPP_STANZA_ATTR_TYPE, "error",
            NULL});
    xmpp_stanza_set_attr(presence, XMPP_STANZA_ATTR_ID, make_uuid());

    struct xmpp_stanza *x = xmpp_stanza_new("x", (const char*[]){
            "xmlns", "http://jabber.org/protocol/muc",
            NULL});
    xmpp_stanza_append_child(presence, x);

    struct xmpp_stanza *error = xmpp_stanza_new("error", (const char*[]){
            "type", "cancel", NULL});
    xmpp_stanza_set_attr(error, "by", jid_to_str(room_jid));
    xmpp_stanza_append_child(presence, error);

    struct xmpp_stanza *conflict = xmpp_stanza_new("conflict", (const char*[]){
            "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas",
            NULL});
    xmpp_stanza_append_child(error, conflict);
    xmpp_server_route_stanza(muc->server, presence);
    xmpp_stanza_del(presence, true);
}

static void send_presence_broadcast(struct xep_muc *muc, struct room *room,
                                    const char *to, const char *from,
                                    const char *nickname) {
    struct xmpp_stanza *presence = xmpp_stanza_new("presence", (const char*[]){
            XMPP_STANZA_ATTR_TO, from,
            NULL});

    struct xmpp_stanza *presence_x = xmpp_stanza_new("x", (const char*[]){
            "xmlns", "http://jabber.org/protocol/muc#user",
            NULL});
    xmpp_stanza_append_child(presence, presence_x);

    struct xmpp_stanza *item = xmpp_stanza_new("item", (const char*[]){
            "affiliation", "member",
            "role", "participant",
            NULL});
    xmpp_stanza_append_child(presence_x, item);

    struct jid *tmp_jid = jid_new_from_jid(room->jid);

    /* XEP-0045 Section 7.2.3: Broadcast presence of existing occupants to the
     * new occupant. */
    struct room_client *room_client;
    DL_FOREACH(room->clients, room_client) {
        xmpp_stanza_set_attr(presence, XMPP_STANZA_ATTR_ID, make_uuid());

        jid_set_resource(tmp_jid, room_client->nickname);
        xmpp_stanza_set_attr(presence, XMPP_STANZA_ATTR_FROM,
                             jid_to_str(tmp_jid));
        xmpp_server_route_stanza(muc->server, presence);
    }

    /* Send presence of the new occupant to all occupants. */
    jid_set_resource(tmp_jid, nickname);
    xmpp_stanza_set_attr(presence, XMPP_STANZA_ATTR_FROM, jid_to_str(tmp_jid));

    DL_FOREACH(room->clients, room_client) {
        xmpp_stanza_set_attr(presence, XMPP_STANZA_ATTR_ID, make_uuid());
        xmpp_stanza_set_attr(presence, XMPP_STANZA_ATTR_TO,
                             jid_to_str(room_client->client_jid));
        xmpp_server_route_stanza(muc->server, presence);
    }

    /* Send the self-presence to the client. */
    xmpp_stanza_set_attr(presence, XMPP_STANZA_ATTR_ID, make_uuid());
    xmpp_stanza_copy_attr(presence, XMPP_STANZA_ATTR_TO, from);

    struct xmpp_stanza *status = xmpp_stanza_new("status",
        (const char*[]){"code", "110", NULL});
    xmpp_stanza_append_child(presence_x, status);

    xmpp_server_route_stanza(muc->server, presence);

    /* Clean up. */
    xmpp_stanza_del(presence, true);
    jid_del(tmp_jid);
}
