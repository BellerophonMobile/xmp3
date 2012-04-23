XMP3 - XMPP Proxy
=================

 * Author: Tom Wambold <tom5760@gmail.com>
 * Copyright (c) 2012 Tom Wambold

Purpose
-------

XMP3 provides a lightweight (mostly) RFC compliant XMPP server.  The key
feature is the ability to hook into the server to receive/send XMPP stanzas.
This makes it easy to use XMP3 as a proxy to another system.  Otherwise XMP3
works well as a light weight XMPP server for general use, especially on
resource-constrained devices.

Most of this work is funded by the Naval Research Laboratory:
http://cs.itd.nrl.navy.mil/

Features
--------

 * Mostly RFC 6120, 6121 compliant XMPP implementation
 * Supports one-to-one messaging.
 * Supports XEP-0045 multi-user chat.
 * Pluggable authentication mechanisms:
    * See xmpp_server_set_auth_callback in xmpp_server.h

Planned Features
----------------

XMP3 does not currently, but should in a future version, support these
features:

 * Configurable roster implementations (file/database backed, etc).
 * Server dialback support.
 * RFC 6120 Server to Server communication.

Supported Platforms
-------------------

XMP3 is officially supported and tested on the following platforms:

 * Linux
    * Arch Linux kernel 3.3.2
    * Ubuntu 11.10
 * Mac OSX
    * 10.6 Snow Leopard
    * 10.7 Lion
 * Android
    * Android 2.3.3 - Cyanogenmod 7.1
        * LG Optimus V
        * HTC Nexus One
        * Samsung Galaxy Nexus
    * Android 4.0.3 - ASUS EEE Transformer

XMP3 has been tested with the following clients:

 * Pidgin 2.10.3 (libpurple 2.10.3)
 * iChat in OSX 10.6 and 10.7.
 * Xabber on Android

Requirements
------------

XMP3 requires the following third-party libraries:

 * Expat - For XML parsing
    * http://expat.sourceforge.net/
    * Tested with version 2.1.0
 * OpenSSL - For TLS support
    * http://www.openssl.org/
    * Tested with version 1.0.1

Building XMP3 requires Python (versions 2.6, 2.7, and 3 work).

Building and running the unit tests requires: as, nm, objdump, objcopy, grep,
awk, and sed.

XMP3 and the tests can be built with gcc or clang.

Compiling
---------

To build the default optimized build, without unit tests, run:

    ./waf configure build

To build with no optimizations and debugging symbols:

    ./waf configure --debug
    ./waf

To build and run the tests, run:

    ./waf configure --test
    ./waf test

Each build command will put output in the "build" directory.  To remove built
object files:

    ./waf clean

To completley remove all compiled files:

    ./waf distclean

See the help output of waf for more options:

    ./waf --help

To build the Doxygen documentation, just run "doxygen" in the root distribution
directory.

Running
-------

Waf will put the final binaries in the "build" directory.  By default, XMP3
will listen on localhost port 5222 for incoming connections.  It expects
OpenSSL keyfile and certificate to be in the current working directory, named
"server.pem" and "server.crt" respectively.  See the section "Generating
Certificates" for details.

To run XMP3 listening on localhost with no plugins or OpenSSL support, use the
"-n" or "--no-ssl" flag:

    ./xmp3 -n

To change the address XMP3 listens on, use the "-a" or "--address" flag:

    ./xmp3 -a 0.0.0.0 -n

Similarly, to change the port, use the "-p" or "--port" flag:

    ./xmp3 -p 6333 -n

To change the SSL key file use the "-k" or "--ssl-key" flag.  Change the SSL
certificate with "-c" or "--ssl-cert":

    ./xmp3 -k /path/to/key.pem -c /path/to/cert.crt


To load a config file (in order to load plugins), use the "-f", "--config"
option (see the section "Configuration File" for details):

    ./xmp3 -f /path/to/config.ini

See the help output for a full list of options, "-h" or "--help":

    ./xmp3 -h

Loadable Modules
----------------

XMP3 currently has two plugins in the source distribution, an implementation of
XEP-0045 Multi-User chat, and a simple multicast server-to-server forwarding to
illustrate hooking into XMP3.  Currently, a configuration file is neccessary to
load and configure these modules, see the "Configuration File" section for
details.

Configuration File
------------------

XMP3 supports using a simple ini-formatted configuration file for loading
external plugins (see the "Loadable Modules" section for details).  A commented
example can be found in the file "sample_config.ini" in the root of the XMP3
source distribution.  Note, command-line arguments override options set in a
configuration file.

The format is as follows.  Any options defined before the first section affect
the core XMP3 operation.  These correspond mostly to command-line arguments
(i.e. address, port).

Next, there is a special "[modules]" section which defines external modules to
be loaded.  These are in the form of (key, value) pairs, with the value being
the name of the shared library to load, and the key being an arbitrary name for
one instantiation of that module.  This way, it is possible to load multiple
instances of the same library.  The key is then used as the title for a section
to configure that instance of the module.  For example:

    [modules]
    mcast1 = libxmp3_multicast.so
    mcast2 = libxmp3_multicast.so

    [mcast1]
    address = 225.1.2.3
    port = 6100

    [mcast2]
    address = 225.6.5.4
    port = 8912

This section loads the multicast forwarder plugin twice, running on different
addresses and ports.

Generating Certificates
-----------------------

To generate a simple OpenSSL private key/certificate pair, use the following
commands:

    openssl genrsa -out server.pem 2048
    openssl req -new -x509 -key server.pem -out server.crt -days 99999

The files (server.pem, server.crt) can be named arbitrarily, but XMP3 defaults
to using those names.  Also, note that the certificate generation command uses
a high number for the expiration time, make sure this, and other openssl
options make sense for your deployment.

Development
-----------

The Doxygen documentation provides an overview of the source code.  To begin
implementing your own proxy, see "src/xmp3_multicast.c" for a basic example.
The Doxygen documentation also has a writeup of building an external module
under the "Related Pages" section.

Third-Party Libraries
---------------------

XMP3 relies on the following third-party libraries (most of them included in
the "lib" directory):

 * Expat - for XML parsing
    * http://expat.sourceforge.net/
 * OpenSSL - for SSL/TLS support
    * http://www.openssl.org/
 * inih - to parse the configuration file
    * http://inih.googlecode.com/
 * test-dept - for unit tests
    * http://test-dept.googlecode.com/
 * tj-tools - misc. utility functions (dynamic library loading, etc.)
    * http://tj-tools.googlecode.com/
 * uthash - hash tables, dynamic arrays, and dynamic string macros/functions
    * http://uthash.sourceforge.net/
 * libb64 - base64 decoding
    * http://libb64.sourceforge.net/
