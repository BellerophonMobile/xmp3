#!/usr/bin/env python
# Copyright (c) 2012 Tom Wambold <tom5760@gmail.com>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
'''
    xmp3 - XMPP Proxy
    wscript - Waf build script, see http://waf.googlecode.com for docs
    Copyright (c) 2012 Drexel University
'''

import os.path
import platform

import waflib
import waflib.Tools.waf_unit_test

# Hack to control when the unit tests are run
run_tests = False

def options(ctx):
    ctx.load('compiler_c')
    ctx.load('waf_unit_test')

    opts = ctx.add_option_group('XMP3 Options')
    opts.add_option('--debug', action='store_true',
                    help='Build with debugging flags (default optimized).')
    opts.add_option('--muc-module', action='store_true',
                    help='Build the MUC component as an external module.')

def configure(ctx):
    ctx.load('compiler_c')
    ctx.load('waf_unit_test')

    # Check if cross-compiling
    is_android = 'android' in ctx.env.CC[0]

    ctx.env.muc_module = ctx.options.muc_module
    if ctx.env.muc_module:
        ctx.env.DEFINES += ['MUC_MODULE']

    # Mac OSX's uuid stuff is built into its libc
    if is_android or platform.system() != 'Darwin':
        ctx.check_cc(lib='uuid')

    ctx.check_cc(lib='m')
    ctx.check_cc(lib='dl')
    ctx.check_cc(lib='expat')
    ctx.check_cc(lib='crypto')
    ctx.check_cc(lib='ssl', use='CRYPTO')
    ctx.check_cc(lib='ev')

    if ctx.env.CC_NAME == 'gcc':
        ctx.env.CFLAGS += ['-std=gnu99', '-Wall', '-Wextra', '-Werror',
                           '-Wno-unused-parameter', '-Wno-strict-aliasing']

        if platform.system() == 'Darwin':
            ctx.env.CFLAGS += ['-Wno-deprecated-declarations']

        ctx.env.LINKFLAGS_DYNAMIC += ['-export-dynamic']

        if ctx.options.debug:
            ctx.env.CFLAGS += ['-O0', '-g']
        else:
            ctx.env.CFLAGS += ['-O3']
            ctx.env.DEFINES += ['NDEBUG']

def build(ctx):
    ctx.add_post_fun(waflib.Tools.waf_unit_test.summary)

    libxmp3 = ctx.stlib(
        target = 'xmp3',
        name = 'libxmp3',
        includes = [
            'src',
            'deps/uthash/src',
            'deps/inih',
            'deps/tj-tools/src',
        ],
        use = ['DYNAMIC', 'M', 'DL', 'EXPAT', 'SSL', 'CRYPTO', 'UUID', 'EV'],
        source = [
            'deps/inih/ini.c',
            'deps/tj-tools/src/tj_searchpathlist.c',
            'deps/tj-tools/src/tj_solibrary.c',
            'src/client_socket.c',
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

    ctx.program(
        target = 'xmp3',
        source = [
            'src/main.c',
        ],
        use = ['libxmp3'],
    )

    # Optionally build the MUC component as a module.
    if ctx.env.muc_module:
        ctx.shlib(
            target = 'xep_muc',
            includes = libxmp3.includes,
            source = ['src/xep_muc.c'],
        )
    else:
        libxmp3.source.append('src/xep_muc.c')

    ctx.shlib(
        target = 'xmp3_multicast',
        includes = libxmp3.includes,
        source = ['src/xmp3_multicast.c'],
    )

    # Unit tests
    ctx.stlib(
        target = 'cmockery',
        defines = ['HAVE_MALLOC_H'],
        includes = ['deps/cmockery/src/google'],
        export_includes = ['deps/cmockery/src/google'],
        source = ['deps/cmockery/src/cmockery.c'],
    )

    _make_test(ctx, 'utils', extra_use=['UUID'])
    _make_test(ctx, 'jid', ['src/utils.c'], ['UUID'])
    _make_test(ctx, 'xmpp_stanza', ['src/xmpp_parser.c', 'src/utils.c'],
               ['UUID', 'EXPAT'])
    _make_test(ctx, 'xmpp_parser', ['src/xmpp_stanza.c', 'src/utils.c'],
               ['UUID', 'EXPAT']);

def test(ctx):
    global run_tests
    run_tests = True
    waflib.Options.options.all_tests = True
    waflib.Options.commands = ['build'] + waflib.Options.commands

class Test(waflib.Build.BuildContext):
    'Context class for our unit testing command.'
    cmd = 'test'
    fun = 'test'

def _make_test(ctx, target, extra_source=None, extra_use=None):
    if extra_source is None:
        extra_source = []

    if extra_use is None:
        extra_use = []

    ctx.program(
        features = 'test',
        target = target + '_test',
        use = ['cmockery'] + extra_use,
        includes = [
            'src',
            'deps/uthash/src',
            'deps/inih',
            'deps/tj-tools/src',
            'deps/test-dept/src',
            ],
        #defines = ['UNIT_TESTING'],
        source = [
            #'src/{0}.c'.format(target),
            'test/{0}_test.c'.format(target),
        ] + extra_source,
    )

def _test_runnable_status(self):
    'Control when unit tests are run'
    global run_tests
    if not run_tests:
        return waflib.Task.SKIP_ME
    else:
        return old_runnable_status(self)

# Monkey-patch this function to take over control of when unit-tests are run.
# Normally, they are run every time you build.
old_runnable_status = waflib.Tools.waf_unit_test.utest.runnable_status
waflib.Tools.waf_unit_test.utest.runnable_status = _test_runnable_status
