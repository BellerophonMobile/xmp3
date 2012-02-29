#!/usr/bin/env python
'''
    xmp3 - XMPP Proxy
    wscript - Waf build script, see http://waf.googlecode.com for docs
    Copyright (c) 2012 Drexel University
'''

import platform

def options(ctx):
    ctx.load('compiler_c')

    opts = ctx.add_option_group('XMP3 Options')
    opts.add_option('--disable-ssl', action='store_true',
                    help='Disable SSL support.')

def configure(ctx):
    ctx.load('compiler_c')

    # System waf is running on: linux, darwin (Mac OSX), freebsd, windows, etc.
    ctx.env.target = platform.system().lower()

    # Mac OSX's uuid stuff is built into its libc
    if ctx.env.target != 'darwin':
        ctx.check_cc(lib='uuid')

    ctx.check_cc(lib='expat')

    ctx.env.disable_ssl = ctx.options.disable_ssl
    if not ctx.env.disable_ssl:
        ctx.check_cc(lib='crypto')
        ctx.check_cc(lib='ssl')

def build(ctx):
    libxmp3 = ctx.stlib(
        target = 'xmp3',
        name = 'libxmp3',
        includes = ['src', 'lib/uthash/src'],
        export_includes = ['src', 'lib/uthash/src'],
        use = ['uthash', 'EXPAT', 'CRYPTO', 'SSL'],
        source = [
            'src/client_socket.c',
            'src/event.c',
            'src/jid.c',
            'src/utils.c',
            'src/xep_muc.c',
            'src/xmp3_options.c',
            'src/xmpp_auth.c',
            'src/xmpp_client.c',
            'src/xmpp_core.c',
            'src/xmpp_im.c',
            'src/xmpp_parser.c',
            'src/xmpp_server.c',
            'src/xmpp_stanza.c',
        ],
    )

    # Mac's uuid stuff is built into its libc
    if ctx.env.target != 'darwin':
        libxmp3.use += ['UUID']

    ctx.program(
        target = 'xmp3',
        source = [
            'src/main.c',
        ],
        use = ['libxmp3'],
    )
