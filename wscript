#!/usr/bin/env python
'''
    xmp3 - XMPP Proxy
    wscript - Waf build script, see http://waf.googlecode.com for docs
    Copyright (c) 2011 Drexel University
'''

import waflib

# So you don't need to do ./waf configure if you are just using the defaults
waflib.Configure.autoconfig = True

def options(opt):
    opt.load('compiler_c')

def configure(conf):
    conf.load('compiler_c')

    # To compile for android...
    # grab libexpat.so from an android phone, and copy expat.h and
    # expat_external.h and put them in the root directory.
    # and comment out the next two lines (with the right paths)
    #conf.env.INCLUDES += ['/home/tom/code/xmp3']
    #conf.env.LIBPATH += ['/home/tom/code/xmp3']

    # Then run
    # CC="/opt/android-ndk/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-gcc --sysroot=/opt/android-ndk/platforms/android-9/arch-arm/" ./waf configure

    conf.check_cc(lib='expat')

    conf.env.CFLAGS += ['-std=gnu99', '-Wall']
    conf.env.CFLAGS += ['-O0', '-ggdb']

def build(bld):
    bld.program(
        target = 'xmp3',
        source = [
            'src/event.c',
            'src/main.c',
            'src/utils.c',
            'src/xmpp_auth.c',
            'src/xmpp.c',
            'src/xmpp_common.c',
            'src/xmpp_core.c',
            'src/xmpp_im.c',
        ],
        use = ['EXPAT'],
    )
