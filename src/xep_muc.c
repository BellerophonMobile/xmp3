/**
 * xmp3 - XMPP Proxy
 * xep_muc.{c,h} - Implements XEP-0045 Multi-User Chat
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include <expat.h>

#include "uthash.h"
#include "utstring.h"
#include "utlist.h"

#include "log.h"

#include "client_socket.h"
#include "jid.h"
#include "xmpp_client.h"
#include "xmpp_common.h"
#include "xmpp_core.h"
#include "xmpp_im.h"
#include "xmpp_server.h"
#include "xmpp_stanza.h"

#include "xep_muc.h"

static const char *MUC_DOMAINPART = "conference.";

static const char *MSG_DISCO_QUERY_INFO =
    "<iq id='%s' type='result'>"
        "<query xmlns='http://jabber.org/protocol/disco#info'>"
            "<identity category='conference' "
                "name='XMP3 MUC' "
                "type='text'/>"
            "<feature var='http://jabber.org/protocol/muc'/>"
        "</query>"
    "</iq>";

static const char *MSG_DISCO_QUERY_ITEMS =
    "<iq id='%s' type='result'>"
        "<query xmlns='http://jabber.org/protocol/disco#items'>"
            "%s"
        "</query>"
    "</iq>";

static const char *QUERY_ITEMS_ITEM = "<item jid='%s' name='%s'/>";

// TODO: Support other affiliations/roles
static const char *MSG_PRESENCE_ITEM =
    "<presence from='%s' to='%s'>"
        "<x xmlns='http://jabber.org/protocol/muc#user'>"
            "<item affiliation='member' role='participant'/>"
        "</x>"
    "</presence>";

static const char *MSG_PRESENCE_ITEM_SELF =
    "<presence from='%s' to='%s'>"
        "<x xmlns='http://jabber.org/protocol/muc#user'>"
            "<item affiliation='member' role='participant'/>"
            "<status code='110'/>"
        "</x>"
    "</presence>";

static const char *MSG_PRESENCE_EXIT_ITEM =
    "<presence from='%s' to='%s' type='unavailable'>"
        "<x xmlns='http://jabber.org/protocol/muc#user'>"
            "<item affiliation='member' role='none'/>"
        "</x>"
    "</presence>";

static const char *MSG_PRESENCE_EXIT_ITEM_SELF =
    "<presence from='%s' to='%s' type='unavailable'>"
        "<x xmlns='http://jabber.org/protocol/muc#user'>"
            "<item affiliation='member' role='none'/>"
            "<status code='110'/>"
        "</x>"
    "</presence>";

static const char *PRESENCE_TYPE_UNAVAILABLE = "unavailable";
static const char *MESSAGE_TYPE_GROUPCHAT = "groupchat";

/** Represents a particular client in a chat room. */
struct room_client {
    /** The user's nickname. */
    char *nickname;

    /** The actual connected client. */
    struct xmpp_client *client;

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

/** Temporary structure used when parsing a stanza. */
struct muc_tmp {
    struct xmpp_stanza *stanza;
    struct xep_muc *muc;
    struct room *room;
};

struct xep_muc {
    struct room *rooms;
    struct jid *jid;
};

// Forward declarations
static bool stanza_handler(struct xmpp_stanza *stanza, void *data);
static void muc_stanza_end(void *data, const char *name);

static void handle_message(struct xmpp_stanza *stanza, struct xep_muc *muc);
static void message_client_start(void *data, const char *name,
                                 const char **attrs);
static void message_client_end(void *data, const char *name);
static void message_client_data(void *data, const char *s, int len);

static void handle_presence(struct xmpp_stanza *stanza, struct xep_muc *muc);
static void enter_room_presence(const char *search_room,
                                struct xmpp_stanza *stanza,
                                struct xep_muc *muc);
static void leave_room_presence(const char *search_room,
                                struct xmpp_stanza *stanza,
                                struct xep_muc *muc);

static void client_disconnect(struct xmpp_client *client, void *data);
static bool leave_room(struct xep_muc *muc, struct room *room,
                       struct room_client *room_client);

static struct room* room_new(const struct xep_muc *muc, const char *name);
static void room_del(struct room *room);

static struct room_client* room_client_new(const char *nickname,
                                           struct xmpp_client *client);
static void room_client_del(struct room_client *room_client);

static void iq_start(void *data, const char *name, const char **attrs);
static void disco_query_info_end(void *data, const char *name);
static void disco_query_items_end(void *data, const char *name);

struct xep_muc* xep_muc_new(struct xmpp_server *server) {
    struct xep_muc *muc = calloc(1, sizeof(*muc));
    check_mem(muc);

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
    free(muc);
}

static bool stanza_handler(struct xmpp_stanza *stanza, void *data) {
    struct xep_muc *muc = (struct xep_muc*)data;
    struct xmpp_client *client = xmpp_stanza_client(stanza);

    log_info("MUC stanza");

    struct muc_tmp *muc_tmp = calloc(1, sizeof(*muc_tmp));
    check_mem(muc_tmp);
    muc_tmp->stanza = stanza;
    muc_tmp->muc = muc;

    XML_SetElementHandler(xmpp_client_parser(client),
                          xmpp_ignore_start,
                          muc_stanza_end);
    XML_SetUserData(xmpp_client_parser(client), muc_tmp);

    if (strcmp(xmpp_stanza_ns_name(stanza), XMPP_MESSAGE) == 0) {
        log_info("MUC Message");
        handle_message(stanza, muc);
        return true;
    } else if (strcmp(xmpp_stanza_ns_name(stanza), XMPP_PRESENCE) == 0) {
        log_info("MUC Presence");
        handle_presence(stanza, muc);
        return true;
    } else if (strcmp(xmpp_stanza_ns_name(stanza), XMPP_IQ) == 0) {
        log_info("MUC IQ");
        XML_SetStartElementHandler(xmpp_client_parser(client), iq_start);
        return true;
    } else {
        log_warn("Unknown MUC stanza");
    }

    return false;
}

static void muc_stanza_end(void *data, const char *name) {
    struct muc_tmp *muc_tmp = (struct muc_tmp *)data;
    struct xmpp_stanza *stanza = muc_tmp->stanza;
    struct xmpp_client *client = xmpp_stanza_client(stanza);

    if (strcmp(xmpp_stanza_ns_name(stanza), name) != 0) {
        return;
    }

    log_info("MUC stanza end");
    XML_SetUserData(xmpp_client_parser(client), stanza);
    free(muc_tmp);

    xmpp_core_stanza_end(stanza, name);
}

static void handle_message(struct xmpp_stanza *stanza, struct xep_muc *muc) {
    struct xmpp_client *from_client = xmpp_stanza_client(stanza);

    if (xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TYPE) == NULL ||
            strcmp(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TYPE),
                   MESSAGE_TYPE_GROUPCHAT) != 0) {
        log_warn("MUC message stanza type other than groupchat, ignoring.");
        return;
    }

    struct room *room = NULL;
    HASH_FIND_STR(muc->rooms, jid_local(xmpp_stanza_jid_to(stanza)), room);
    if (room == NULL) {
        // TODO: Force presence into room
        log_warn("MUC message to non-existent room, ignoring.");
        return;
    }

    struct room_client *room_client = NULL;
    DL_FOREACH(room->clients, room_client) {
        if (room_client->client == from_client) {
            break;
        }
    }
    if (room_client == NULL) {
        // TODO: Force presence into room
        log_warn("MUC message to unjoined room, ignoring.");
        return;
    }

    /* We need the "from" field of the stanza we send to be from the nickname
     * of the user who sent it. */
    struct jid *nick_jid = jid_new_from_jid(room->jid);
    jid_set_resource(nick_jid, room_client->nickname);
    char *nick_jid_str = jid_to_str(nick_jid);

    char *old_from = strdup(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_FROM));
    xmpp_stanza_set_attr(stanza, XMPP_STANZA_ATTR_FROM, nick_jid_str);

    char* msg = xmpp_stanza_create_tag(stanza);

    xmpp_stanza_set_attr(stanza, XMPP_STANZA_ATTR_FROM, old_from);
    free(old_from);
    free(nick_jid_str);
    jid_del(nick_jid);

    struct room_client *room_client_tmp;
    DL_FOREACH_SAFE(room->clients, room_client, room_client_tmp) {
        if (client_socket_sendall(xmpp_client_socket(room_client->client), msg,
                                  strlen(msg)) <= 0) {
            log_err("Error sending MUC message start tag to destination.");
            xmpp_server_disconnect_client(room_client->client);
        }
    }

    XML_SetElementHandler(xmpp_client_parser(from_client),
                          message_client_start, message_client_end);
    XML_SetCharacterDataHandler(xmpp_client_parser(from_client),
                                message_client_data);

    struct muc_tmp *muc_tmp = XML_GetUserData(xmpp_client_parser(from_client));
    muc_tmp->room = room;
}

static void message_client_start(void *data, const char *name,
                                 const char **attrs) {
    struct muc_tmp *muc_tmp = (struct muc_tmp*)data;
    struct xmpp_stanza *stanza = muc_tmp->stanza;
    struct xmpp_client *from_client = xmpp_stanza_client(stanza);
    struct room *room = muc_tmp->room;

    struct room_client *room_client, *room_client_tmp;
    DL_FOREACH_SAFE(room->clients, room_client, room_client_tmp) {
        if (client_socket_sendxml(
                    xmpp_client_socket(room_client->client),
                    xmpp_client_parser(from_client)) <= 0) {
            log_err("Error sending MUC message to destination.");
            xmpp_server_disconnect_client(room_client->client);
        }
    }
}

static void message_client_end(void *data, const char *name) {
    struct muc_tmp *muc_tmp = (struct muc_tmp*)data;
    struct xmpp_stanza *stanza = muc_tmp->stanza;
    struct xmpp_client *from_client = xmpp_stanza_client(stanza);
    struct room *room = muc_tmp->room;

    /* For self-closing tags "<foo/>" we get an end event without any data to
     * send.  We already sent the end tag with the start tag. */
    if (XML_GetCurrentByteCount(xmpp_client_parser(from_client)) > 0) {
        struct room_client *room_client, *room_client_tmp;
        DL_FOREACH_SAFE(room->clients, room_client, room_client_tmp) {
            if (client_socket_sendxml(
                        xmpp_client_socket(room_client->client),
                        xmpp_client_parser(from_client)) <= 0) {
                log_err("Error sending MUC message to destination.");
                xmpp_server_disconnect_client(room_client->client);
            }
        }
    }

    // If we received a </message> tag, we are done parsing this stanza.
    if (strcmp(name, XMPP_MESSAGE) == 0) {
        muc_stanza_end(data, name);
    }
}

static void message_client_data(void *data, const char *s, int len) {
    struct muc_tmp *muc_tmp = (struct muc_tmp*)data;
    struct xmpp_stanza *stanza = muc_tmp->stanza;
    struct xmpp_client *from_client = xmpp_stanza_client(stanza);
    struct room *room = muc_tmp->room;

    struct room_client *room_client, *room_client_tmp;
    DL_FOREACH_SAFE(room->clients, room_client, room_client_tmp) {
        if (client_socket_sendxml(
                    xmpp_client_socket(room_client->client),
                    xmpp_client_parser(from_client)) <= 0) {
            log_err("Error sending MUC message to destination.");
            xmpp_server_disconnect_client(room_client->client);
        }
    }
}

static void handle_presence(struct xmpp_stanza *stanza, struct xep_muc *muc) {
    const struct jid *to_jid = xmpp_stanza_jid_to(stanza);
    if (jid_resource(to_jid) == NULL) {
        log_warn("MUC presence has no nickname, ignoring.");
        return;
    }

    const char *search_room = jid_local(to_jid);

    if (xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TYPE) != NULL
        && strcmp(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TYPE),
                  PRESENCE_TYPE_UNAVAILABLE) == 0) {
        debug("Leaving room");
        leave_room_presence(search_room, stanza, muc);
    } else {
        debug("Entering room");
        enter_room_presence(search_room, stanza, muc);
    }
}

static void enter_room_presence(const char *search_room,
                                struct xmpp_stanza *stanza,
                                struct xep_muc *muc) {
    struct xmpp_client *from_client = xmpp_stanza_client(stanza);
    const struct jid *from_jid = xmpp_client_jid(from_client);

    struct room *room = NULL;
    HASH_FIND_STR(muc->rooms, search_room, room);
    if (room == NULL) {
        // Room doesn't exist, create it
        debug("New room, creating");
        room = room_new(muc, search_room);
        HASH_ADD_KEYPTR(hh, muc->rooms, room->name, strlen(room->name), room);
    }

    char *tostr = jid_to_str(from_jid);
    char *fromstr = NULL;
    struct jid *tmpjid = jid_new_from_jid(room->jid);

    /* XEP-0045 Section 7.1.3: Broadcast presence of existing occupants to the
     * new occupant. */
    struct room_client *room_client;
    debug("Sending occupants to new user");
    DL_FOREACH(room->clients, room_client) {
        jid_set_resource(tmpjid, room_client->nickname);
        fromstr = jid_to_str(tmpjid);

        char msg[strlen(MSG_PRESENCE_ITEM)
                 + strlen(tostr)
                 + strlen(fromstr)];

        sprintf(msg, MSG_PRESENCE_ITEM, fromstr, tostr);
        check(client_socket_sendall(xmpp_client_socket(from_client), msg,
                                    strlen(msg)) > 0,
              "Error sending presence to new occupant.");
        free(fromstr);
        fromstr = NULL;
    }

    struct room_client *new_client = room_client_new(
            jid_resource(xmpp_stanza_jid_to(stanza)), from_client);
    check_mem(new_client);
    DL_APPEND(room->clients, new_client);

    xmpp_server_add_client_listener(from_client, client_disconnect, muc);

    // Send presence of the new occupant itself
    jid_set_resource(tmpjid, new_client->nickname);
    fromstr = jid_to_str(tmpjid);

    /* Define a new scope, so we don't have to worry about jumping over the
     * creation of the variable-length message array. */
    {
        char msg[strlen(MSG_PRESENCE_ITEM_SELF)
                 + strlen(fromstr)
                 + strlen(tostr)];
        sprintf(msg, MSG_PRESENCE_ITEM_SELF, fromstr, tostr);
        check(client_socket_sendall(xmpp_client_socket(from_client), msg,
                                    strlen(msg)) > 0,
              "Error sending self presence to new occupant.");
    }
    free(tostr);

    // Send presence of the new occupant to all occupants.
    debug("Sending presence of new user to all occupants.");
    struct room_client *room_client_tmp;
    DL_FOREACH_SAFE(room->clients, room_client, room_client_tmp) {
        if (room_client == new_client) {
            // We already sent the new client's presence back to it
            continue;
        }
        tostr = jid_to_str(xmpp_client_jid(room_client->client));

        char msg[strlen(MSG_PRESENCE_ITEM)
                 + strlen(fromstr)
                 + strlen(tostr)];

        sprintf(msg, MSG_PRESENCE_ITEM, fromstr, tostr);
        if (client_socket_sendall(xmpp_client_socket(room_client->client), msg,
                                  strlen(msg)) <= 0) {
            log_err("Sending presence to disconnected client?");
            xmpp_server_disconnect_client(room_client->client);
        }
        free(tostr);
        tostr = NULL;
    }

    free(fromstr);
    jid_del(tmpjid);
    debug("Done!");
    return;

error:
    if (room->clients == NULL) {
        HASH_DEL(muc->rooms, room);
        room_del(room);
    }
    if (tostr) {
        free(tostr);
    }
    if (fromstr) {
        free(fromstr);
    }
    if (tmpjid) {
        jid_del(tmpjid);
    }
    XML_StopParser(xmpp_client_parser(from_client), false);
}

static void leave_room_presence(const char *search_room,
                                struct xmpp_stanza *stanza,
                                struct xep_muc *muc) {
    struct xmpp_client *from_client = xmpp_stanza_client(stanza);

    struct room *room = NULL;
    HASH_FIND_STR(muc->rooms, search_room, room);
    if (room == NULL) {
        log_warn("Trying to leave a nonexistent room");
        return;
    }

    struct room_client *room_client = NULL;
    DL_FOREACH(room->clients, room_client) {
        if (room_client->client == from_client) {
            break;
        }
    }
    if (room_client == NULL) {
        log_warn("Non member trying to leave room.");
        return;
    }

    if (!leave_room(muc, room, room_client)) {
        XML_StopParser(xmpp_client_parser(from_client), false);
    }
}

static void client_disconnect(struct xmpp_client *client, void *data) {
    struct xep_muc *muc = (struct xep_muc*)data;

    struct room *room, *room_tmp;
    HASH_ITER(hh, muc->rooms, room, room_tmp) {
        struct room_client *room_client, *room_client_tmp;
        DL_FOREACH_SAFE(room->clients, room_client, room_client_tmp) {
            if (room_client->client == client) {
                leave_room(muc, room, room_client);
            }
        }
    }
}

static bool leave_room(struct xep_muc *muc, struct room *room,
                       struct room_client *room_client) {
    bool rv = true;

    DL_DELETE(room->clients, room_client);

    struct jid *tmpjid = jid_new_from_jid(room->jid);
    jid_set_resource(tmpjid, room_client->nickname);

    char *fromstr = jid_to_str(tmpjid);
    room_client_del(room_client);

    char *tostr = jid_to_str(xmpp_client_jid(room_client->client));

    // Send the leave presence back to the client
    char msg_self[strlen(MSG_PRESENCE_EXIT_ITEM_SELF)
                  + strlen(fromstr)
                  + strlen(tostr)];
    sprintf(msg_self, MSG_PRESENCE_EXIT_ITEM_SELF, fromstr, tostr);
    if (client_socket_sendall(xmpp_client_socket(room_client->client),
                              msg_self, strlen(msg_self)) <= 0) {
        log_err("Error sending self exit presence to client");
        rv = false;
    }
    free(tostr);

    // Send the leave presence to other occupants in the room
    struct room_client *other_client, *other_client_tmp;
    DL_FOREACH_SAFE(room->clients, other_client, other_client_tmp) {
        tostr = jid_to_str(xmpp_client_jid(other_client->client));
        char msg[strlen(MSG_PRESENCE_EXIT_ITEM)
                 + strlen(fromstr)
                 + strlen(tostr)];
        sprintf(msg, MSG_PRESENCE_EXIT_ITEM, fromstr, tostr);
        if (client_socket_sendall(xmpp_client_socket(other_client->client),
                                  msg, strlen(msg)) <= 0) {
            log_err("Sending exit presence to disconnected client?");
            xmpp_server_disconnect_client(other_client->client);
        }
        free(tostr);
        tostr = NULL;
    }
    free(fromstr);

    // If the room is empty now, delete it
    if (room->clients == NULL) {
        HASH_DEL(muc->rooms, room);
        room_del(room);
    }
    return rv;
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
                                           struct xmpp_client *client) {
    struct room_client *room_client = calloc(1, sizeof(*room_client));
    check_mem(room_client);

    room_client->nickname = strdup(nickname);
    check_mem(room_client->nickname);

    room_client->client = client;
    return room_client;
}

static void room_client_del(struct room_client *room_client) {
    free(room_client->nickname);
    free(room_client);
}

static void iq_start(void *data, const char *name, const char **attrs) {
    struct muc_tmp *muc_tmp = (struct muc_tmp *)data;
    struct xmpp_stanza *stanza = muc_tmp->stanza;
    struct xmpp_client *client = xmpp_stanza_client(stanza);

    if (strcmp(name, XMPP_IQ_DISCO_QUERY_INFO) == 0) {
        log_info("MUC Info Query Start");
        XML_SetElementHandler(xmpp_client_parser(client), xmpp_error_start,
                              disco_query_info_end);

    } else if (strcmp(name, XMPP_IQ_DISCO_QUERY_ITEMS) == 0) {
        log_info("MUC Items Query Start");
        XML_SetElementHandler(xmpp_client_parser(client), xmpp_error_start,
                              disco_query_items_end);

    } else {
        log_info("Unhandled MUC IQ");
        xmpp_send_service_unavailable(stanza);
    }
}

static void disco_query_info_end(void *data, const char *name) {
    struct muc_tmp *muc_tmp = (struct muc_tmp*)data;
    struct xmpp_stanza *stanza = muc_tmp->stanza;
    struct xmpp_client *client = xmpp_stanza_client(stanza);

    char msg[strlen(MSG_DISCO_QUERY_INFO)
             + strlen(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID))];

    log_info("MUC Info Query IQ End");
    check(strcmp(name, XMPP_IQ_DISCO_QUERY_INFO) == 0, "Unexpected stanza");

    snprintf(msg, sizeof(msg), MSG_DISCO_QUERY_INFO,
             xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID));
    check(client_socket_sendall(xmpp_client_socket(client), msg,
                                strlen(msg)) > 0,
          "Error sending info query IQ message");

    XML_SetElementHandler(xmpp_client_parser(client),
                          xmpp_error_start, muc_stanza_end);
    return;

error:
    XML_StopParser(xmpp_client_parser(client), false);
}

static void disco_query_items_end(void *data, const char *name) {
    struct muc_tmp *muc_tmp = (struct muc_tmp*)data;
    struct xmpp_stanza *stanza = muc_tmp->stanza;
    struct xmpp_client *client = xmpp_stanza_client(stanza);
    struct xep_muc *muc = muc_tmp->muc;

    log_info("MUC Items Query IQ End");
    check(strcmp(name, XMPP_IQ_DISCO_QUERY_ITEMS) == 0, "Unexpected stanza");

    UT_string items;
    utstring_init(&items);

    struct room *room;
    for (room = muc->rooms; room != NULL; room = room->hh.next) {
        char *strjid = jid_to_str(room->jid);
        utstring_printf(&items, QUERY_ITEMS_ITEM, strjid,
                        jid_local(room->jid));
        free(strjid);
    }

    UT_string msg;
    utstring_init(&msg);
    utstring_printf(&msg, MSG_DISCO_QUERY_ITEMS,
                    xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID),
                    utstring_body(&items));
    utstring_done(&items);

    check(client_socket_sendall(xmpp_client_socket(client),
                                utstring_body(&msg), utstring_len(&msg)) > 0,
          "Error sending items query IQ message");
    utstring_done(&msg);

    XML_SetElementHandler(xmpp_client_parser(client),
                          xmpp_error_start, muc_stanza_end);
    return;

error:
    utstring_done(&msg);
    XML_StopParser(xmpp_client_parser(client), false);
}
