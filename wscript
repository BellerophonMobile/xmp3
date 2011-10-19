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

def build(bld):
    bld.program(
        target = 'xmp3',
        source = [
            'src/event.c',
            'src/main.c',
            'src/utils.c',
            'src/xmpp.c',
            'src/xmpp_auth.c',
            'src/xmpp_common.c',
        ],
        use = ['EXPAT'],
    )
