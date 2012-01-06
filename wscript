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

    # To compile for android...
    # CC="/opt/android-ndk/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-gcc --sysroot=/opt/android-ndk/platforms/android-9/arch-arm/" ./waf configure

    if not ctx.env.use_local_expat:
        ctx.check_cc(lib='expat')

    if not ctx.env.use_local_openssl:
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
        use = ['uthash'],
    )

    if ctx.env.use_local_expat:
        libxmp3.use += ['expat']
    else:
        libxmp3.use += ['EXPAT']

    if ctx.env.use_local_openssl:
        libxmp3.use += ['crypto', 'ssl']
    else:
        libxmp3.use += ['CRYPTO', 'SSL']

    ctx.program(
        target = 'xmp3',
        source = [
            'src/main.c',
        ],
        use = ['libxmp3'],
    )
