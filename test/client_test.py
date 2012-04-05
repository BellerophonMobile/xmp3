#!/usr/bin/env python3
# Copyright (c) 2012 Tom Wambold <tom5760@gmail.com>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
'''
    Tests an XMPP server by sending stanzas and asserting on responses.
'''

import os
import os.path
import subprocess
import base64
import sys
import tempfile
import unittest
import socket
import xml.etree.ElementTree
import io
import time

XMP3_PATH = os.path.join(os.path.dirname(os.path.realpath(__file__)),
                         '../build/xmp3')

XMP3_ADDRESS = ('127.0.0.1', 5222)
TIMEOUT = 1

# From: https://github.com/codeinthehole/unittest-xml/
# Modifications by Tom Wambold
class XMLAssertions(object):
    _namespaces = {
        'stream': 'http://etherx.jabber.org/streams',
        'xmpp-sasl': 'urn:ietf:params:xml:ns:xmpp-sasl',
        'xmpp-bind': 'urn:ietf:params:xml:ns:xmpp-bind',
        'xmpp-session': 'urn:ietf:params:xml:ns:xmpp-session',
        'disco-info': 'http://jabber.org/protocol/disco#info',
        'disco-items': 'http://jabber.org/protocol/disco#items',
        'roster': 'jabber:iq:roster',
    }

    @staticmethod
    def _get_doc(xml_str):
        parse = xml.etree.ElementTree.iterparse(io.StringIO(xml_str),
                                                ['start'])
        rv = xml.etree.ElementTree.Element('doc')
        doc = next(parse)[1]
        rv.append(doc)
        try:
            for n in parse:
                pass
        except xml.etree.ElementTree.ParseError:
            pass
        return rv

    def assertXPathNodeCount(self, xml_str, num, xpath, check=None):
        doc = XMLAssertions._get_doc(xml_str)
        count = len(doc.findall(xpath, XMLAssertions._namespaces))
        if check is None:
            self.assertEqual(num, count)
        else:
            check(num, count)

    def assertXPathNodeText(self, xml_str, expected, xpath):
        doc = XMLAssertions._get_doc(xml_str)
        self.assertEqual(expected, doc.findtext(xpath, None,
                                                XMLAssertions._namespaces))

    def assertXPathNodeAttributes(self, xml_str, attributes, xpath):
        doc = XMLAssertions._get_doc(xml_str)
        ele = doc.find(xpath, XMLAssertions._namespaces)
        for attribute, value in attributes.items():
            self.assertTrue(attribute in ele.attrib)
            if value is not None:
                self.assertEqual(value, ele.attrib[attribute])

class NoSSLTests(unittest.TestCase, XMLAssertions):
    def setUp(self):
        log_fd, log_path = tempfile.mkstemp('.log', 'xmp3_test_')
        print('Logging to:', log_path)
        self.xmp3 = subprocess.Popen([XMP3_PATH, '-n'], stdout=log_fd,
                                     stderr=subprocess.STDOUT)
        time.sleep(2)

    def tearDown(self):
        self.xmp3.terminate()
        self.xmp3.wait()

    def testPidginConnect(self):
        user1 = Pidgin('user1', 'resource1', 'password1')

        # <stream:stream from='localhost' id='foobarx' version='1.0'
        #                xml:lang='en' xmlns='jabber:client'
        #                xmlns:stream='http://etherx.jabber.org/streams'>
        #     <stream:features>
        #         <mechanisms xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>
        #             <mechanism>PLAIN</mechanism>
        #         </mechanisms>
        #     </stream:features>
        msg = user1.initial_stream_header()
        self.assertXPathNodeCount(msg, 1, 'stream:stream')
        self.assertXPathNodeAttributes(msg,
                {'from': None, 'id': None, 'version': None}, 'stream:stream')

        self.assertXPathNodeCount(msg, 1, 'stream:stream/stream:features')
        self.assertXPathNodeCount(msg, 1, 'stream:stream/stream:features/xmpp-sasl:mechanisms')
        self.assertXPathNodeCount(msg, 1, 'stream:stream/stream:features/xmpp-sasl:mechanisms',
                                  self.assertLessEqual)
        self.assertXPathNodeText(msg, 'PLAIN', 'stream:stream/stream:features/xmpp-sasl:mechanisms/xmpp-sasl:mechanism')

        # <success xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/>
        msg = user1.sasl_plain_auth()
        self.assertXPathNodeCount(msg, 1, 'xmpp-sasl:success')

        # <stream:stream from='localhost' id='foobarx' version='1.0'
        #                xml:lang='en' xmlns='jabber:client'
        #                xmlns:stream='http://etherx.jabber.org/streams'>
        #     <stream:features>
        #         <bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'/>
        #         <session xmlns='urn:ietf:params:xml:ns:xmpp-session'/>
        #     </stream:features>
        msg = user1.after_sasl_stream_header()
        self.assertXPathNodeCount(msg, 1, 'stream:stream')
        self.assertXPathNodeAttributes(msg,
                {'from': None, 'id': None, 'version': None}, 'stream:stream')

        self.assertXPathNodeCount(msg, 1, 'stream:stream/stream:features')
        self.assertXPathNodeCount(msg, 1, 'stream:stream/stream:features/xmpp-bind:bind')
        self.assertXPathNodeCount(msg, 1, 'stream:stream/stream:features/xmpp-session:session')

        # <iq id='purple6aec7129' type='result'>
        #     <bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'>
        #         <jid>user1@localhost/resource1</jid>
        #     </bind>
        # </iq>
        msg = user1.resource_bind_iq()
        self.assertXPathNodeCount(msg, 1, 'iq')
        self.assertXPathNodeAttributes(msg, {'id': 'purple6aec7129',
                                       'type': 'result'}, 'iq')

        self.assertXPathNodeCount(msg, 1, 'iq/xmpp-bind:bind')
        self.assertXPathNodeCount(msg, 1, 'iq/xmpp-bind:bind/xmpp-bind:jid')
        self.assertXPathNodeText(msg, 'user1@localhost/resource1', 'iq/xmpp-bind:bind/xmpp-bind:jid')

        # <iq id='purple6aec712a' from='localhost' to='user1@localhost/resource1' type='result'/>
        msg = user1.session_start()
        self.assertXPathNodeCount(msg, 1, 'iq')
        self.assertXPathNodeAttributes(msg, {'id': 'purple6aec712a',
            'type': 'result'}, 'iq')

        # <iq id='purple6aec712b' from='localhost' to='user1@localhost/resource1' type='result'>
        #     <query xmlns='http://jabber.org/protocol/disco#items'/>
        # </iq>
        msg = user1.disco_items_query()
        self.assertXPathNodeCount(msg, 1, 'iq')
        self.assertXPathNodeCount(msg, 1, 'iq/disco-items:query')

        # <iq id='purple6aec712c' from='localhost' to='user1@localhost/resource1' type='result'>
        #     <query xmlns='http://jabber.org/protocol/disco#info'>
        #         <identity category='server' type='im' name='xmp3'/>
        #         <feature var='http://jabber.org/protocol/disco#info'/>
        #         <feature var='http://jabber.org/protocol/disco#items'/>
        #     </query>
        # </iq>
        msg = user1.disco_info_query()
        self.assertXPathNodeCount(msg, 1, 'iq')
        self.assertXPathNodeCount(msg, 1, 'iq/disco-info:query')
        self.assertXPathNodeCount(msg, 3, 'iq/disco-info:query/*', self.assertLessEqual)

        # TODO: XMP3 currently returns an error for this
        msg = user1.iq_vcard_get()

        # <iq id='purple6aec712e' from='localhost' to='user1@localhost/resource1' type='result'>
        #     <query xmlns='jabber:iq:roster'/>
        # </iq>
        msg = user1.iq_roster_get()
        self.assertXPathNodeCount(msg, 1, 'iq')
        self.assertXPathNodeCount(msg, 1, 'iq/roster:query')

        # TODO: XMP3 does not respond to this at all
        user1.iq_bytestream_query()
        user1.initial_presence()

    def testPidginOneToOne(self):
        user2 = Pidgin('user2', 'resource2', 'password2')
        user1 = Pidgin('user3', 'resource3', 'password3')

        user1.connect_no_ssl()
        user2.connect_no_ssl()


class Client(object):
    def __init__(self):
        self.sock = socket.create_connection(XMP3_ADDRESS)
        self.sock.settimeout(TIMEOUT)

    def __del__(self):
        self.sock.close()

    def send_msg(self, msg):
        self.sock.sendall(bytes(msg, 'utf-8'))

    def recv_msg(self):
        rv = []
        while True:
            try:
                data = str(self.sock.recv(102400), 'utf-8')
                if len(data) == 0:
                    break
                rv.append(data)
            except socket.timeout:
                break
        msg = ''.join(rv)
        #print(msg)
        #print()
        return msg

class Pidgin(Client):
    def __init__(self, user, resource, password):
        super().__init__()
        self.user = user
        self.resource = resource
        self.password = password

    def connect_no_ssl(self):
        self.initial_stream_header()
        self.sasl_plain_auth()
        self.after_sasl_stream_header()
        self.resource_bind_iq()
        self.session_start()
        self.disco_items_query()
        self.disco_info_query()
        self.iq_vcard_get()
        self.iq_roster_get()
        self.iq_bytestream_query()
        self.initial_presence()

    def initial_stream_header(self):
        self.send_msg("<?xml version='1.0' ?>")
        self.send_msg("<stream:stream to='localhost'"
                                    " xmlns='jabber:client'"
                                    " xmlns:stream='http://etherx.jabber.org/streams'"
                                    " version='1.0'>")
        return self.recv_msg()

    def sasl_plain_auth(self):
        self.send_msg("<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl'"
                           " mechanism='PLAIN'"
                           " xmlns:ga='http://www.google.com/talk/protocol/auth'"
                           " ga:client-uses-full-bind-result='true'>"
                           "{}</auth>".format(str(base64.b64encode(bytes(
                               '\0'.join(['', self.user, self.password]),
                               'utf-8')), 'utf-8')))
        return self.recv_msg()

    def after_sasl_stream_header(self):
        self.send_msg("<stream:stream to='localhost'"
                                    " xmlns='jabber:client'"
                                    " xmlns:stream='http://etherx.jabber.org/streams'"
                                    " version='1.0'>")
        return self.recv_msg()

    def resource_bind_iq(self):
        self.send_msg("<iq type='set' id='purple6aec7129'>"
                          "<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'>"
                               "<resource>{0}</resource>"
                          "</bind></iq>".format(self.resource))
        return self.recv_msg()

    def session_start(self):
        self.send_msg("<iq type='set' id='purple6aec712a'>"
                          "<session xmlns='urn:ietf:params:xml:ns:xmpp-session'/>"
                      "</iq>")
        return self.recv_msg()

    def disco_items_query(self):
        self.send_msg("<iq type='get' id='purple6aec712b' to='localhost'>"
                          "<query xmlns='http://jabber.org/protocol/disco#items'/>"
                      "</iq>")
        return self.recv_msg()

    def disco_info_query(self):
        self.send_msg("<iq type='get' id='purple6aec712c' to='localhost'>"
                          "<query xmlns='http://jabber.org/protocol/disco#info'/>"
                      "</iq>")
        return self.recv_msg()

    def iq_vcard_get(self):
        self.send_msg("<iq type='get' id='purple6aec712d'>"
                          "<vCard xmlns='vcard-temp'/>"
                      "</iq>")
        return self.recv_msg()

    def iq_roster_get(self):
        self.send_msg("<iq type='get' id='purple6aec712e'>"
                          "<query xmlns='jabber:iq:roster'/>"
                      "</iq>")
        return self.recv_msg()

    def iq_bytestream_query(self):
        self.send_msg("<iq type='get' id='purple6aec712f' to='proxy.eu.jabber.org'>"
                          "<query xmlns='http://jabber.org/protocol/bytestreams'/>"
                      "</iq>")
        # TODO: XMP3 does not respond to this at all
        return

    def initial_presence(self):
        self.send_msg("<presence>"
                          "<priority>1</priority>"
                          "<c xmlns='http://jabber.org/protocol/caps' node='http://pidgin.im/' hash='sha-1' ver='lV6i//bt2U8Rm0REcX8h4F3Nk3M=' ext='voice-v1 camera-v1 video-v1'/>"
                          "<x xmlns='vcard-temp:x:update'/>"
                      "</presence>")
        # XMP3 does not respond to this yet
        return



def main(argv):
    print('Using XMP3 Path:', XMP3_PATH)
    unittest.main()

if __name__ == '__main__':
    sys.exit(main(sys.argv))
