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

XMP3_PATH = os.path.join(os.path.dirname(os.path.realpath(__file__)),
                         '../build/xmp3')

XMP3_ADDRESS = ('127.0.0.1', 5222)
TIMEOUT = 1

LOCAL = 'user1'
DOMAIN = 'localhost'
RESOURCE = 'resource1'

class NoSSLTests(unittest.TestCase):
    def setUp(self):
        log_fd, log_path = tempfile.mkstemp('.log', 'xmp3_test_')
        print('Logging to:', log_path)
        self.xmp3 = subprocess.Popen([XMP3_PATH, '-n'], stdout=log_fd,
                                     stderr=subprocess.STDOUT)

        self.sock = socket.create_connection(XMP3_ADDRESS, TIMEOUT)

    def tearDown(self):
        self.xmp3.terminate()
        self.xmp3.wait()

    def send_msg(self, msg):
        self.sock.sendall(bytes(msg, 'utf-8'))

    def recv_msg(self):
        rv = []
        while True:
            try:
                rv.append(str(self.sock.recv(102400), 'utf-8'))
            except socket.timeout:
                break
        return ''.join(rv)

    # TODO: Use something like: http://pypi.python.org/pypi/unittest-xml
    def testPidginConnect(self):
        self.send_msg("<?xml version='1.0' ?>")
        self.send_msg("<stream:stream to='localhost'"
                                    " xmlns='jabber:client'"
                                    " xmlns:stream='http://etherx.jabber.org/streams'"
                                    " version='1.0'>")

        # <stream:stream from='localhost' id='foobarx' version='1.0'
        #                xml:lang='en' xmlns='jabber:client'
        #                xmlns:stream='http://etherx.jabber.org/streams'>
        #     <stream:features>
        #         <mechanisms xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>
        #             <mechanism>PLAIN</mechanism>
        #         </mechanisms>
        #     </stream:features>
        msg = self.recv_msg()
        self.assertTrue(msg.startswith('<stream:stream'))
        self.assertIn("from='localhost'", msg)
        self.assertRegex(msg, "id='.*'")
        self.assertIn("xmlns='jabber:client'", msg)
        self.assertIn("xmlns:stream='http://etherx.jabber.org/streams'", msg)
        self.assertIn("version='1.0'", msg)
        self.assertIn("<stream:features>", msg)
        self.assertIn("<mechanisms", msg)
        self.assertIn("xmlns='urn:ietf:params:xml:ns:xmpp-sasl'", msg)
        self.assertIn("<mechanism>PLAIN</mechanism>", msg)
        self.assertTrue(msg.endswith('>'))

        self.send_msg("<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl'"
                           " mechanism='PLAIN'"
                           " xmlns:ga='http://www.google.com/talk/protocol/auth'"
                           " ga:client-uses-full-bind-result='true'>"
                           "{}</auth>".format(base64.b64encode(bytes('\0'.join(
                                (LOCAL, DOMAIN, RESOURCE)), 'utf-8'))))

        # <success xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/>
        msg = self.recv_msg()
        self.assertTrue(msg.startswith('<success'))
        self.assertIn("xmlns='urn:ietf:params:xml:ns:xmpp-sasl'", msg)
        self.assertTrue(msg.endswith('/>'))

        self.send_msg("<stream:stream to='localhost'"
                                    " xmlns='jabber:client'"
                                    " xmlns:stream='http://etherx.jabber.org/streams'"
                                    " version='1.0'>")
        msg = self.recv_msg()

        # <stream:stream from='localhost' id='foobarx' version='1.0'
        #                xml:lang='en' xmlns='jabber:client'
        #                xmlns:stream='http://etherx.jabber.org/streams'>
        self.assertTrue(msg.startswith('<stream:stream'))
        self.assertIn("from='localhost'", msg)
        self.assertRegex(msg, "id='.*'")
        self.assertIn("xmlns='jabber:client'", msg)
        self.assertIn("xmlns:stream='http://etherx.jabber.org/streams'", msg)
        self.assertIn("version='1.0'", msg)
        self.assertTrue(msg.endswith('>'))

        # <stream:features>
        #     <bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'/>
        #     <session xmlns='urn:ietf:params:xml:ns:xmpp-session'/>
        # </stream:features>
        self.assertIn("<stream:features>", msg)
        self.assertIn("<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'/>", msg)
        self.assertIn("<session xmlns='urn:ietf:params:xml:ns:xmpp-session'/>", msg)



def main(argv):
    print('Using XMP3 Path:', XMP3_PATH)
    unittest.main()

if __name__ == '__main__':
    sys.exit(main(sys.argv))
