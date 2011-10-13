/**
 * xmp3 - XMPP Proxy
 * event.{c,h} - Simple event loop
 * Copyright (c) 2011 Drexel University
 */

#include "event.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/queue.h>
#include <sys/select.h>

/* Contains data for one callback function */
struct event_item {
    /* The file descriptor to monitor */
    int fd;

    /* Function to call when the descriptor fires */
    event_callback func;

    /* Miscelaneous data to send to the callback function */
    void *data;

    /* Linked-list entries to more event_items */
    LIST_ENTRY(event_item) list_entry;
};

/* Contains data about the whole event loop */
struct event_loop {
    /* File descriptors for select */
    fd_set readfds;

    /* For select, highest-numbered file descriptor in readfds plus 1 */
    int nfds;

    /* Linked-list of callback functions */
    LIST_HEAD(event_items, event_item) list_head;
};

struct event_loop* event_new_loop(void) {
    struct event_loop *loop = malloc(sizeof(struct event_loop));
    if (loop == NULL) {
        fprintf(stderr, "Error allocating event loop\n");
        abort();
    }

    loop->nfds = 0;
    LIST_INIT(&loop->list_head);

    return loop;
}

void event_del_loop(struct event_loop *loop) {
    // Free the memory for all the callback entries
    struct event_item *item;
    LIST_FOREACH(item, &loop->list_head, list_entry) {
        free(item);
    }
    free(loop);
}

void event_register_callback(struct event_loop *loop, int fd,
                             event_callback func, void *data) {
    struct event_item *item = malloc(sizeof(struct event_item));
    if (item == NULL) {
        fprintf(stderr, "Error allocating callback structure\n");
        abort();
    }

    // Add the item to the list
    item->fd = fd;
    item->func = func;
    item->data = data;

    LIST_INSERT_HEAD(&loop->list_head, item, list_entry);

    // Select needs nfds, one plus the highest numbered fd in readfds
    if (fd + 1 > loop->nfds) {
        loop->nfds = fd + 1;
    }
}

void event_deregister_callback(struct event_loop *loop, int fd) {
    // Find the entry in the list, and free it
    struct event_item *item;
    LIST_FOREACH(item, &loop->list_head, list_entry) {
        if (item->fd == fd) {
            LIST_REMOVE(item, list_entry);
            free(item);
            break;
        }
    }

    if (fd + 1 == loop->nfds) {
        // We need to find a new maximum
        LIST_FOREACH(item, &loop->list_head, list_entry) {
            if (item->fd + 1 > loop->nfds) {
                loop->nfds = item->fd + 1;
            }
        }
    }
}

void event_init_readfds(struct event_loop *loop) {
    FD_ZERO(&loop->readfds);

    // Add each file descriptor to the select set
    struct event_item *item;
    LIST_FOREACH(item, &loop->list_head, list_entry) {
        FD_SET(item->fd, &loop->readfds);
    }
}

void event_loop_start(struct event_loop *loop) {
    while (true) {
        event_init_readfds(loop);
        int num_ready = select(loop->nfds, &loop->readfds, NULL, NULL, NULL);

        if (num_ready == -1) {
            perror("Event loop select error");
            return;
        }

        if (num_ready == 0) {
            // If/when we do timeout-based callbacks, it would happen here.
            continue;
        }

        struct event_item *item;
        LIST_FOREACH(item, &loop->list_head, list_entry) {
            if (FD_ISSET(item->fd, &loop->readfds)) {
                item->func(loop, item->fd, item->data);
            }
        }
    }
}
