/**
 * xmp3 - XMPP Proxy
 * xmp3_xml.{c,h} - Functions on top of Expat to support mutliple handlers.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <expat.h>

void xmp3_xml_init_parser(XML_Parser parser);

void xmp3_xml_add_handlers(XML_Parser parser,
                           XML_StartElementHandler start,
                           XML_EndElementHandler end,
                           XML_CharacterDataHandler chardata,
                           void *data);

void* xmp3_xml_get_current_user_data(XML_Parser parser);

/** This should only be called from inside a handler. */
void xmp3_xml_replace_handlers(XML_Parser parser,
                               XML_StartElementHandler start,
                               XML_EndElementHandler end,
                               XML_CharacterDataHandler chardata,
                               void *data);

/** This should only be called from inside a handler. */
void xmp3_xml_replace_start_handler(XML_Parser parser,
                                    XML_StartElementHandler start);

/** This should only be called from inside a handler. */
void xmp3_xml_replace_end_handler(XML_Parser parser,
                                  XML_EndElementHandler end);

/** This should only be called from inside a handler. */
void xmp3_xml_replace_chardata_handler(XML_Parser parser,
                                       XML_CharacterDataHandler chardata);

/** This should only be called from inside a handler. */
void xmp3_xml_replace_user_data(XML_Parser parser, void *data);

/** This should only be called from inside a handler. */
void xmp3_xml_remove_handler(XML_Parser parser);
