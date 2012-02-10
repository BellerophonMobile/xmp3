#!/usr/bin/env python
'''
    xmp3 - XMPP Proxy
    wscript - Waf build script, see http://waf.googlecode.com for docs
    Copyright (c) 2011 Drexel University
'''

def options(ctx):
    pass

def configure(ctx):
    ctx.load('compiler_c')

    # Mac OSX's uuid stuff is built into its libc
    if ctx.env.system == 'linux':
        ctx.check_cc(lib='uuid')

    if not ctx.env.cross:
        ctx.check_cc(lib='expat')
        ctx.check_cc(lib='crypto')
        ctx.check_cc(lib='ssl')

def build(ctx):
    libxmp3 = ctx.stlib(
        target = 'xmp3',
        name = 'libxmp3',
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
        export_includes = 'src',
        use = ['uthash', 'EXPAT', 'CRYPTO', 'SSL'],
    )

    # Mac's uuid stuff is built into its libc
    if ctx.env.system in ('linux', 'android'):
        libxmp3.use += ['UUID']

    ctx.program(
        target = 'xmp3',
        source = [
            'src/main.c',
        ],
        use = ['libxmp3'],
    )
