#!/usr/bin/env python
'''
    xmp3 - XMPP Proxy
    wscript - Waf build script, see http://waf.googlecode.com for docs
    Copyright (c) 2011 Drexel University
'''

import waflib

# So you don't need to do ./waf configure if you are just using the defaults
waflib.Configure.autoconfig = True

def options(ctx):
    ctx.load('compiler_c')

    grp = ctx.add_option_group('xmp3 options')

    grp.add_option('--build-docs', action='store_true', default=False,
                   help='Build doxygen documentation (use during configure)')

def configure(ctx):
    ctx.env.BUILD_DOCS = ctx.options.build_docs

    ctx.load('compiler_c')

    if ctx.env.BUILD_DOCS:
        ctx.load('doxygen')

    # To compile for android...
    # grab libexpat.so from an android phone, and copy expat.h and
    # expat_external.h and put them in the root directory.
    # and comment out the next two lines (with the right paths)
    #ctx.env.INCLUDES += ['/home/tom/code/xmp3']
    #ctx.env.LIBPATH += ['/home/tom/code/xmp3']

    # Then run
    # CC="/opt/android-ndk/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-gcc --sysroot=/opt/android-ndk/platforms/android-9/arch-arm/" ./waf configure

    ctx.check_cc(lib='expat')
    ctx.check_cc(lib='crypto')
    ctx.check_cc(lib='ssl')

    ctx.env.CFLAGS += ['-std=gnu99', '-Wall', '-Werror']
    ctx.env.CFLAGS += ['-O0', '-ggdb']

def build(ctx):
    ctx.program(
        target = 'xmp3',
        source = [
            'src/client_socket.c',
            'src/event.c',
            'src/jid.c',
            'src/main.c',
            'src/utils.c',
            'src/xep_muc.c',
            'src/xmp3_options.c',
            'src/xmpp_auth.c',
            'src/xmpp_client.c',
            'src/xmpp_common.c',
            'src/xmpp_core.c',
            'src/xmpp_im.c',
            'src/xmpp_server.c',
            'src/xmpp_stanza.c',
        ],
        use = ['EXPAT', 'CRYPTO', 'SSL'],
    )

    ctx(
        features = 'doxygen',
        name = 'doxygen',
        doxyfile = 'doxyfile',
        posted = not ctx.env.BUILD_DOCS,
    )
