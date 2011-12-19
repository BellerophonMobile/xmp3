#!/usr/bin/env python
'''
    xmp3 - XMPP Proxy
    wscript - Waf build script, see http://waf.googlecode.com for docs
    Copyright (c) 2011 Drexel University
'''

def configure(ctx):
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

def build(ctx):
    ctx.stlib(
        target = 'xmp3',
        name = 'libxmp3',
        source = [
            'src/client_socket.c',
            'src/event.c',
            'src/jid.c',
            'src/utils.c',
            'src/xep_muc.c',
            'src/xmp3_options.c',
            'src/xmp3_xml.c',
            'src/xmpp_auth.c',
            'src/xmpp_client.c',
            'src/xmpp_common.c',
            'src/xmpp_core.c',
            'src/xmpp_im.c',
            'src/xmpp_server.c',
            'src/xmpp_stanza.c',
        ],
        export_includes = 'src',
    )

    ctx.program(
        target = 'xmp3',
        source = [
            'src/main.c',
        ],
        use = ['libxmp3', 'EXPAT', 'CRYPTO', 'SSL'],
    )

    ctx(
        features = 'doxygen',
        doxyfile = 'doxyfile',
        posted = not ctx.env.BUILD_DOCS,
    )
