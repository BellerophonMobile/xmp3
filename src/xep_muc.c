/**
 * xmp3 - XMPP Proxy
 * xep_muc.{c,h} - Implements XEP-0045 Multi-User Chat
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include <uthash.h>
#include <utlist.h>

#include "jid.h"
#include "log.h"
#include "utils.h"
#include "xmpp_client.h"
#include "xmpp_im.h"
#include "xmpp_server.h"
#include "xmpp_stanza.h"

#include "xep_muc.h"

static const char *MUC_DOMAINPART = "conference.";
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

// Forward declarations
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

struct xep_muc* xep_muc_new(struct xmpp_server *server) {
    struct xep_muc *muc = calloc(1, sizeof(*muc));
    check_mem(muc);

    muc->server = server;
    muc->jid = jid_new_from_jid(xmpp_server_jid(server));

    char new_domain[strlen(MUC_DOMAINPART)
                    + strlen(jid_domain(muc->jid)) + 1];
    strcpy(new_domain, MUC_DOMAINPART);
    strcat(new_domain, jid_domain(muc->jid));
    jid_set_domain(muc->jid, new_domain);

    jid_set_local(muc->jid, "*");
    jid_set_resource(muc->jid, "*");
    xmpp_server_add_stanza_route(server, muc->jid, stanza_handler, muc);
    jid_set_local(muc->jid, NULL);
    jid_set_resource(muc->jid, NULL);

    return muc;
}

void xep_muc_del(struct xep_muc *muc) {
    jid_set_local(muc->jid, "*");
    jid_set_resource(muc->jid, "*");
    xmpp_server_del_stanza_route(muc->server, muc->jid, stanza_handler, muc);
    jid_set_local(muc->jid, NULL);
    jid_set_resource(muc->jid, NULL);

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

static bool stanza_handler(struct xmpp_stanza *stanza,
                           struct xmpp_server *server, void *data) {
    struct xep_muc *muc = (struct xep_muc*)data;

    const char *from = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_FROM);
    check(from != NULL, "MUC message without from attribute.");

    if (strcmp(xmpp_stanza_name(stanza), XMPP_STANZA_MESSAGE) == 0) {
        log_info("MUC Message");
        return handle_message(stanza, muc);
    } else if (strcmp(xmpp_stanza_name(stanza), XMPP_STANZA_PRESENCE) == 0) {
        log_info("MUC Presence");
        return handle_presence(stanza, muc);
    } else if (strcmp(xmpp_stanza_name(stanza), XMPP_STANZA_IQ) == 0) {
        log_info("MUC IQ");
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

    // Set the "to" and "from" back to their orignal values
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
    struct jid *to_jid = jid_new_from_str(to);

    const char *from = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_FROM);
    struct jid *from_jid = jid_new_from_str(from);

    struct room_client *new_client = room_client_new(jid_resource(to_jid),
                                                     from_jid);
    check_mem(new_client);

    // If this is a local client, we need to know when they disconnect
    struct xmpp_client *client = xmpp_server_find_client(muc->server,
                                                         from_jid);
    if (client != NULL) {
        xmpp_server_add_client_listener(client, client_disconnect, muc);
    }

    jid_del(to_jid);
    jid_del(from_jid);

    struct room *room = NULL;
    HASH_FIND_STR(muc->rooms, search_room, room);
    if (room == NULL) {
        // Room doesn't exist, create it
        debug("New room, creating");
        room = room_new(muc, search_room);
        HASH_ADD_KEYPTR(hh, muc->rooms, room->name, strlen(room->name), room);
    }

    struct xmpp_stanza *presence = xmpp_stanza_new("presence",
        (const char*[]){XMPP_STANZA_ATTR_TO, from, NULL});
    struct xmpp_stanza *presence_x = xmpp_stanza_new("x", (const char*[]){
            "xmlns", "http://jabber.org/protocol/muc#user",
            NULL});
    xmpp_stanza_append_child(presence, presence_x);
    struct xmpp_stanza *presence_item = xmpp_stanza_new("item",
            (const char*[]){
            "affiliation", "member",
            "role", "participant",
            NULL});
    xmpp_stanza_append_child(presence_x, presence_item);

    /* XEP-0045 Section 7.2.3: Broadcast presence of existing occupants to the
     * new occupant. */
    struct room_client *room_client;
    DL_FOREACH(room->clients, room_client) {
        xmpp_stanza_set_attr(presence, XMPP_STANZA_ATTR_ID, make_uuid());

        jid_set_resource(room->jid, room_client->nickname);
        xmpp_stanza_set_attr(presence, XMPP_STANZA_ATTR_FROM,
                             jid_to_str(room->jid));
        xmpp_server_route_stanza(muc->server, presence);
    }

    // Send presence of the new occupant to all occupants.
    jid_set_resource(room->jid, new_client->nickname);
    xmpp_stanza_set_attr(presence, XMPP_STANZA_ATTR_FROM,
                         jid_to_str(room->jid));

    DL_FOREACH(room->clients, room_client) {
        xmpp_stanza_set_attr(presence, XMPP_STANZA_ATTR_ID, make_uuid());
        xmpp_stanza_set_attr(presence, XMPP_STANZA_ATTR_TO,
                             jid_to_str(room_client->client_jid));
        xmpp_server_route_stanza(muc->server, presence);
    }

    // Send the self-presence to the client
    DL_APPEND(room->clients, new_client);
    xmpp_stanza_set_attr(presence, XMPP_STANZA_ATTR_ID, make_uuid());
    xmpp_stanza_copy_attr(presence, XMPP_STANZA_ATTR_TO, from);

    struct xmpp_stanza *status = xmpp_stanza_new("status",
        (const char*[]){"code", "110", NULL});
    xmpp_stanza_append_child(presence_x, status);

    xmpp_server_route_stanza(muc->server, presence);

    // Clean up
    xmpp_stanza_del(presence, true);
    jid_set_resource(room->jid, NULL);

    debug("Done!");
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
    return leave_room(muc, room, room_client);

error:
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

    // Send self-presence back to the occupant
    struct xmpp_stanza *status = xmpp_stanza_new("status",
        (const char*[]){"code", "110", NULL});
    xmpp_stanza_append_child(presence_x, status);

    xmpp_server_route_stanza(muc->server, presence);
    DL_DELETE(room->clients, room_client);
    room_client_del(room_client);

    xmpp_stanza_remove_child(presence_x, status);
    xmpp_stanza_del(status, true);

    // Send the leave presence to other occupants in the room
    struct room_client *other_client;
    DL_FOREACH(room->clients, other_client) {
        xmpp_stanza_set_attr(presence, XMPP_STANZA_ATTR_TO,
                             jid_to_str(other_client->client_jid));
        xmpp_server_route_stanza(muc->server, presence);
    }
    xmpp_stanza_del(presence, true);

    // If the room is empty now, delete it
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
