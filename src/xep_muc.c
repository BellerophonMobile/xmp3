/**
 * xmp3 - XMPP Proxy
 * xep_muc.{c,h} - Implements XEP-0045 Multi-User Chat
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include "xep_muc.h"

#include "uthash.h"
#include "utstring.h"
#include "utlist.h"

#include "log.h"
#include "utils.h"
#include "xmpp_core.h"
#include "xmpp_im.h"

const struct jid MUC_JID = { "*", "conference.localhost", "*" };

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
    // TODO: For now, we just use the local part of the JID as the name.
    //char *name;

    /** The JID of the room. */
    struct jid jid;

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
};

// Forward declarations
static void muc_stanza_end(void *data, const char *name);

static void handle_message(struct xmpp_stanza *stanza, struct xep_muc *muc);
static void message_client_start(void *data, const char *name,
                                 const char **attrs);
static void message_client_end(void *data, const char *name);
static void message_client_data(void *data, const char *s, int len);

static void handle_presence(struct xmpp_stanza *stanza, struct xep_muc *muc);
static void enter_room(char *search_room, struct xmpp_stanza *stanza,
                       struct xep_muc *muc);
static void leave_room(char *search_room, struct xmpp_stanza *stanza,
                       struct xep_muc *muc);

static void iq_start(void *data, const char *name, const char **attrs);
static void disco_query_info_end(void *data, const char *name);
static void disco_query_items_end(void *data, const char *name);

struct xep_muc* xep_muc_new() {
    struct xep_muc *muc = calloc(1, sizeof(*muc));
    check_mem(muc);
    return muc;
}

void xep_muc_del(struct xep_muc *muc) {
    free(muc);
}

bool xep_muc_stanza_handler(struct xmpp_stanza *stanza, void *data) {
    struct xep_muc *muc = (struct xep_muc*)data;
    struct xmpp_client *client = stanza->from_client;

    log_info("MUC stanza");

    struct muc_tmp *muc_tmp = calloc(1, sizeof(*muc_tmp));
    check_mem(muc_tmp);
    muc_tmp->stanza = stanza;
    muc_tmp->muc = muc;

    XML_SetElementHandler(client->parser, xmpp_ignore_start, muc_stanza_end);
    XML_SetUserData(client->parser, muc_tmp);

    if (strcmp(stanza->ns_name, XMPP_MESSAGE) == 0) {
        log_info("MUC Message");
        handle_message(stanza, muc);
        return true;
    } else if (strcmp(stanza->ns_name, XMPP_PRESENCE) == 0) {
        log_info("MUC Presence");
        handle_presence(stanza, muc);
        return true;
    } else if (strcmp(stanza->ns_name, XMPP_IQ) == 0) {
        log_info("MUC IQ");
        XML_SetStartElementHandler(client->parser, iq_start);
        return true;
    } else {
        log_warn("Unknown MUC stanza");
    }

    return false;
}

static void muc_stanza_end(void *data, const char *name) {
    struct muc_tmp *muc_tmp = (struct muc_tmp *)data;
    struct xmpp_stanza *stanza = muc_tmp->stanza;
    struct xmpp_client *client = stanza->from_client;

    if (strcmp(name, stanza->ns_name) != 0) {
        return;
    }

    log_info("MUC stanza end");
    XML_SetUserData(client->parser, stanza);
    free(muc_tmp);

    xmpp_core_stanza_end(stanza, name);
}

static void handle_message(struct xmpp_stanza *stanza, struct xep_muc *muc) {
    struct xmpp_client *from_client = stanza->from_client;

    if (stanza->type == NULL
        || strcmp(stanza->type, MESSAGE_TYPE_GROUPCHAT) != 0) {
        log_warn("MUC message stanza type other than groupchat, ignoring.");
        return;
    }

    struct room *room = NULL;
    HASH_FIND_STR(muc->rooms, stanza->to_jid.local, room);
    if (room == NULL) {
        log_warn("MUC message to non-existent room, ignoring.");
        return;
    }

    struct room_client *room_client;
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

    XML_SetElementHandler(from_client->parser, message_client_start,
                          message_client_end);
    XML_SetCharacterDataHandler(from_client->parser, message_client_data);

    /* We need the "from" field of the stanza we send to be from the nickname
     * of the user who sent it. */
    char *fromstr = stanza->from;
    room->jid.resource = room_client->nickname;
    stanza->from = jid_to_str(&room->jid);
    char* msg = create_start_tag(stanza);
    free(stanza->from);
    stanza->from = fromstr;
    room->jid.resource = NULL;

    DL_FOREACH(room->clients, room_client) {
        if (sendall(room_client->client->socket, msg, strlen(msg)) <= 0) {
            log_err("Error sending MUC message start tag to destination.");
        }
    }

    struct muc_tmp *muc_tmp = XML_GetUserData(from_client->parser);
    muc_tmp->room = room;
}

static void message_client_start(void *data, const char *name,
                                 const char **attrs) {
    struct muc_tmp *muc_tmp = (struct muc_tmp*)data;
    struct xmpp_stanza *stanza = muc_tmp->stanza;
    struct room *room = muc_tmp->room;

    struct room_client *room_client;
    DL_FOREACH(room->clients, room_client) {
        if (sendxml(stanza->from_client->parser,
                    room_client->client->socket) <= 0) {
            log_err("Error sending MUC message to destination.");
        }
    }
}

static void message_client_end(void *data, const char *name) {
    struct muc_tmp *muc_tmp = (struct muc_tmp*)data;
    struct xmpp_stanza *stanza = muc_tmp->stanza;
    struct room *room = muc_tmp->room;

    /* For self-closing tags "<foo/>" we get an end event without any data to
     * send.  We already sent the end tag with the start tag. */
    if (XML_GetCurrentByteCount(stanza->from_client->parser) > 0) {
        struct room_client *room_client;
        DL_FOREACH(room->clients, room_client) {
            if (sendxml(stanza->from_client->parser,
                        room_client->client->socket) <= 0) {
                log_err("Error sending message to destination.");
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
    struct room *room = muc_tmp->room;

    struct room_client *room_client;
    DL_FOREACH(room->clients, room_client) {
        if (sendxml(stanza->from_client->parser,
                    room_client->client->socket) <= 0) {
            log_err("Error sending message to destination.");
        }
    }
}

static void handle_presence(struct xmpp_stanza *stanza, struct xep_muc *muc) {
    if (stanza->to_jid.resource == NULL) {
        log_warn("MUC presence has no nickname, ignoring.");
        return;
    }

    char *search_room = stanza->to_jid.local;

    if (stanza->type != NULL
        && strcmp(stanza->type, PRESENCE_TYPE_UNAVAILABLE) == 0) {
        debug("Leaving room");
        leave_room(search_room, stanza, muc);
    } else {
        debug("Entering room");
        enter_room(search_room, stanza, muc);
    }
}

static void enter_room(char *search_room, struct xmpp_stanza *stanza,
                       struct xep_muc *muc) {
    struct xmpp_client *from_client = stanza->from_client;

    struct room *room = NULL;
    HASH_FIND_STR(muc->rooms, search_room, room);
    if (room == NULL) {
        // Room doesn't exist, create it
        debug("New room, creating");
        room = calloc(1, sizeof(*room));
        check_mem(room);
        STRDUP_CHECK(room->jid.local, search_room);
        STRDUP_CHECK(room->jid.domain, MUC_JID.domain);
        HASH_ADD_KEYPTR(hh, muc->rooms, room->jid.local,
                        strlen(room->jid.local), room);
    }

    char *tostr = jid_to_str(&from_client->jid);
    char *fromstr;
    struct jid tmpjid = {search_room, MUC_JID.domain, NULL};

    /* XEP-0045 Section 7.1.3: Broadcast presence of existing occupants to the
     * new occupant. */
    struct room_client *room_client;
    debug("Sending occupants to new user");
    DL_FOREACH(room->clients, room_client) {
        tmpjid.resource = room_client->nickname;
        fromstr = jid_to_str(&tmpjid);

        char msg[strlen(MSG_PRESENCE_ITEM)
                 + strlen(tostr)
                 + strlen(fromstr)];

        snprintf(msg, sizeof(msg), MSG_PRESENCE_ITEM, fromstr, tostr);
        check(sendall(from_client->socket, msg, strlen(msg)) > 0,
              "Error sending presence to new occupant.");
        free(fromstr);
    }

    struct room_client *new_client = calloc(1, sizeof(*new_client));
    check_mem(new_client);
    STRDUP_CHECK(new_client->nickname, stanza->to_jid.resource);
    new_client->client = stanza->from_client;
    DL_APPEND(room->clients, new_client);

    // Send presence of the new occupant itself
    tmpjid.resource = new_client->nickname;
    fromstr = jid_to_str(&tmpjid);

    /* Define a new scope, so we don't have to worry about jumping over the
     * creation of the variable-length message array. */
    {
        char msg[strlen(MSG_PRESENCE_ITEM_SELF)
                 + strlen(fromstr)
                 + strlen(tostr)];
        snprintf(msg, sizeof(msg), MSG_PRESENCE_ITEM_SELF, fromstr, tostr);
        check(sendall(from_client->socket, msg, strlen(msg)) > 0,
              "Error sending self presence to new occupant.");
    }
    free(tostr);

    // Send presence of the new occupant to all occupants.
    debug("Sending presence of new user to all occupants.");
    DL_FOREACH(room->clients, room_client) {
        if (room_client == new_client) {
            // We'll send the client's own presence back to it later.
            continue;
        }
        tostr = jid_to_str(&room_client->client->jid);

        char msg[strlen(MSG_PRESENCE_ITEM)
                 + strlen(fromstr)
                 + strlen(tostr)];

        snprintf(msg, sizeof(msg), MSG_PRESENCE_ITEM, fromstr, tostr);
        if (sendall(room_client->client->socket, msg, strlen(msg)) <= 0) {
            log_err("Sending presence to disconnected client?");
        }
        free(tostr);
    }
    free(fromstr);

    debug("Done!");
    return;

error:
    XML_StopParser(from_client->parser, false);
}

static void leave_room(char *search_room, struct xmpp_stanza *stanza,
                       struct xep_muc *muc) {
    struct xmpp_client *from_client = stanza->from_client;

    struct room *room = NULL;
    HASH_FIND_STR(muc->rooms, search_room, room);
    if (room == NULL) {
        log_warn("Trying to leave a nonexistent room");
        return;
    }

    struct room_client *room_client;
    DL_FOREACH(room->clients, room_client) {
        if (room_client->client == from_client) {
            DL_DELETE(room->clients, room_client);
            break;
        }
    }

    room->jid.resource = room_client->nickname;
    char *fromstr = jid_to_str(&room->jid);
    room->jid.resource = NULL;
    free(room_client->nickname);
    free(room_client);

    char *tostr = jid_to_str(&from_client->jid);

    // Send the leave presence back to the client
    char msg_self[strlen(MSG_PRESENCE_EXIT_ITEM_SELF)
                  + strlen(fromstr)
                  + strlen(tostr)];
    snprintf(msg_self, sizeof(msg_self), MSG_PRESENCE_EXIT_ITEM_SELF,
             fromstr, tostr);
    check(sendall(from_client->socket, msg_self, strlen(msg_self)) > 0,
          "Error sending self exit presence to client");
    free(tostr);

    // Send the leave presence to other occupants in the room
    struct room_client *other_client;
    DL_FOREACH(room->clients, other_client) {
        tostr = jid_to_str(&other_client->client->jid);
        char msg[strlen(MSG_PRESENCE_EXIT_ITEM)
                 + strlen(fromstr)
                 + strlen(tostr)];
        snprintf(msg, sizeof(msg), MSG_PRESENCE_EXIT_ITEM, fromstr, tostr);
        if (sendall(other_client->client->socket, msg, strlen(msg)) <= 0) {
            log_err("Sending exit presence to disconnected client?");
        }
        free(tostr);
    }
    free(fromstr);

    // If the room is empty now, delete it
    if (room->clients == NULL) {
        HASH_DEL(muc->rooms, room);
        free(room->jid.local);
        free(room->jid.domain);
        free(room);
    }
    return;

error:
    XML_StopParser(from_client->parser, false);
}

static void iq_start(void *data, const char *name, const char **attrs) {
    struct muc_tmp *muc_tmp = (struct muc_tmp *)data;
    struct xmpp_stanza *stanza = muc_tmp->stanza;
    struct xmpp_client *client = stanza->from_client;

    if (strcmp(name, XMPP_IQ_DISCO_QUERY_INFO) == 0) {
        log_info("MUC Info Query Start");
        XML_SetElementHandler(client->parser, xmpp_error_start,
                              disco_query_info_end);

    } else if (strcmp(name, XMPP_IQ_DISCO_QUERY_ITEMS) == 0) {
        log_info("MUC Items Query Start");
        XML_SetElementHandler(client->parser, xmpp_error_start,
                              disco_query_items_end);

    } else {
        log_info("Unhandled MUC IQ");
        xmpp_send_service_unavailable(stanza);
    }
}

static void disco_query_info_end(void *data, const char *name) {
    struct muc_tmp *muc_tmp = (struct muc_tmp*)data;
    struct xmpp_stanza *stanza = muc_tmp->stanza;
    struct xmpp_client *client = stanza->from_client;

    char msg[strlen(MSG_DISCO_QUERY_INFO) + strlen(stanza->id)];

    log_info("MUC Info Query IQ End");
    check(strcmp(name, XMPP_IQ_DISCO_QUERY_INFO) == 0, "Unexpected stanza");

    snprintf(msg, sizeof(msg), MSG_DISCO_QUERY_INFO, stanza->id);
    check(sendall(client->socket, msg, strlen(msg)) > 0,
          "Error sending info query IQ message");

    XML_SetElementHandler(client->parser, xmpp_error_start, muc_stanza_end);
    return;

error:
    XML_StopParser(client->parser, false);
}

static void disco_query_items_end(void *data, const char *name) {
    struct muc_tmp *muc_tmp = (struct muc_tmp*)data;
    struct xmpp_stanza *stanza = muc_tmp->stanza;
    struct xmpp_client *client = stanza->from_client;
    struct xep_muc *muc = muc_tmp->muc;

    log_info("MUC Items Query IQ End");
    check(strcmp(name, XMPP_IQ_DISCO_QUERY_ITEMS) == 0, "Unexpected stanza");

    UT_string items;
    utstring_init(&items);

    struct room *room;
    for (room = muc->rooms; room != NULL; room = room->hh.next) {
        char *strjid = jid_to_str(&room->jid);
        utstring_printf(&items, QUERY_ITEMS_ITEM, strjid, room->jid.local);
        free(strjid);
    }

    UT_string msg;
    utstring_init(&msg);
    utstring_printf(&msg, MSG_DISCO_QUERY_ITEMS, stanza->id,
                    utstring_body(&items));
    utstring_done(&items);

    check(sendall(client->socket, utstring_body(&msg), utstring_len(&msg)) > 0,
          "Error sending items query IQ message");
    utstring_done(&msg);

    XML_SetElementHandler(client->parser, xmpp_error_start, muc_stanza_end);
    return;

error:
    utstring_done(&msg);
    XML_StopParser(client->parser, false);
}
