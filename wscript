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
    opts.add_option('--debug', action='store_true',
                    help='Build with debugging flags (default optimized).')

def configure(ctx):
    ctx.load('compiler_c')

    # System waf is running on: linux, darwin (Mac OSX), freebsd, windows, etc.
    ctx.env.target = platform.system().lower()

    # Mac OSX's uuid stuff is built into its libc
    if ctx.env.target != 'darwin':
        ctx.check_cc(lib='uuid')

    ctx.check_cc(lib='dl')
    ctx.check_cc(lib='expat')
    ctx.check_cc(lib='crypto')
    ctx.check_cc(lib='ssl')

    if ctx.env.CC_NAME == 'gcc':
        ctx.env.CFLAGS += ['-std=gnu99', '-Wall', '-Wextra', '-Werror',
                           '-Wno-unused-parameter']

        if ctx.env.target == 'darwin':
            ctx.env.CFLAGS += ['-Wno-deprecated-declarations']

        ctx.env.LINKFLAGS_DYNAMIC += ['-export-dynamic']

        if ctx.options.debug:
            ctx.env.CFLAGS += ['-O0', '-g']
        else:
            ctx.env.CFLAGS += ['-O3', '-g']

def build(ctx):
    libxmp3 = ctx.stlib(
        target = 'xmp3',
        name = 'libxmp3',
        includes = [
            'src',
            'lib/uthash/src',
            'lib/inih',
            'lib/tj-tools/src',
        ],
        use = ['DYNAMIC', 'DL', 'EXPAT', 'CRYPTO', 'SSL'],
        source = [
            'lib/inih/ini.c',
            'lib/tj-tools/src/tj_searchpathlist.c',
            'lib/tj-tools/src/tj_solibrary.c',
            'src/client_socket.c',
            'src/event.c',
            'src/jid.c',
            'src/utils.c',
            'src/xmp3_module.c',
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
    libxmp3.export_includes = libxmp3.includes

    # Mac's uuid stuff is built into its libc
    if ctx.env.target != 'darwin':
        libxmp3.use += ['UUID']

    # Don't need to link modules with libxmp3, bind symbols at runtime.
    ctx.shlib(
        target = 'xep_muc',
        includes = libxmp3.includes,
        source = ['src/xep_muc.c'],
    )

    ctx.program(
        target = 'xmp3',
        source = [
            'src/main.c',
        ],
        use = ['libxmp3'],
    )
