/**
 * xmp3 - XMPP Proxy
 * xmp3_xml.{c,h} - Functions on top of Expat to support mutliple handlers.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <stdbool.h>

#include <expat.h>

struct xmp3_xml;

typedef void (*xmp3_xml_start_handler)(void *data, const char *name,
                                       const char **attrs,
                                       struct xmp3_xml *parser);

typedef void (*xmp3_xml_end_handler)(void *data, const char *name,
                                     struct xmp3_xml *parser);

typedef void (*xmp3_xml_data_handler)(void *data, const char *s, int len,
                                      struct xmp3_xml *parser);

struct xmp3_xml* xmp3_xml_new();

void xmp3_xml_del(struct xmp3_xml *parser);

enum XML_Status xmp3_xml_parse(struct xmp3_xml *parser, const char *s, int len,
                               int isFinal);

enum XML_Status xmp3_xml_stop_parser(struct xmp3_xml *parser, bool resumable);

int xmp3_xml_get_byte_count(struct xmp3_xml *parser);

const char* xmp3_xml_get_buffer(struct xmp3_xml *parser, int *offset,
                                int *size);

const char* xmp3_xml_get_error_str(struct xmp3_xml *parser);

void xmp3_xml_add_handlers(struct xmp3_xml *parser,
                           xmp3_xml_start_handler start,
                           xmp3_xml_end_handler end,
                           xmp3_xml_data_handler chardata,
                           void *data);

/** This should only be called from inside a handler. */
void xmp3_xml_replace_handlers(struct xmp3_xml *parser,
                               xmp3_xml_start_handler start,
                               xmp3_xml_end_handler end,
                               xmp3_xml_data_handler chardata,
                               void *data);

/** This should only be called from inside a handler. */
void xmp3_xml_replace_start_handler(struct xmp3_xml *parser,
                                    xmp3_xml_start_handler start);

/** This should only be called from inside a handler. */
void xmp3_xml_replace_end_handler(struct xmp3_xml *parser,
                                  xmp3_xml_end_handler end);

/** This should only be called from inside a handler. */
void xmp3_xml_replace_chardata_handler(struct xmp3_xml *parser,
                                       xmp3_xml_data_handler chardata);

/** This should only be called from inside a handler. */
void xmp3_xml_replace_user_data(struct xmp3_xml *parser, void *data);

/** This should only be called from inside a handler. */
void* xmp3_xml_get_user_data(struct xmp3_xml *parser);

/** This should only be called from inside a handler. */
void xmp3_xml_remove_handlers(struct xmp3_xml *parser);
