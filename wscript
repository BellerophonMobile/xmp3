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
    conf.check_cc(lib='expat')

    conf.env.CFLAGS += ['-std=gnu99', '-Wall']
    conf.env.CFLAGS += ['-O0', '-ggdb']

    # Check out https://github.com/android/platform_external_expat
    # And grab libexpat.so from an android phone.
    #conf.env.INCLUDES += ['extern/expat_android/lib']
    #conf.env.LIBPATH += ['../extern/expat_android/lib']

def build(bld):
    bld.program(
        target = 'xmp3',
        source = [
            'src/event.c',
            'src/main.c',
            'src/utils.c',
            'src/xmpp.c',
            'src/xmpp_common.c',
            'src/xmpp_core.c',
            'src/xmpp_im.c',
        ],
        use = ['EXPAT'],
    )
