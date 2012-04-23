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

def options(ctx):
    ctx.load('compiler_c')
    ctx.load('asm')

    opts = ctx.add_option_group('XMP3 Options')
    opts.add_option('--debug', action='store_true',
                    help='Build with debugging flags (default optimized).')

    opts.add_option('--test', action='store_true',
                    help='Build test programs')

    cross = ctx.add_option_group('Cross Compiling')
    cross.add_option('--cross-android', action='store_const', dest='cross',
                     const='android', default=None,
                     help='Cross compile for Android')
    cross.add_option('--cross-android-ndk', action='store',
                     default=os.getenv('ANDROID_NDK'),
                     help='Android NDK directory.')
    cross.add_option('--cross-android-level', action='store', type=int,
                     default=9, help='Android platform level to use.')
    cross.add_option('--cross-android-arch', action='store', default='armv6',
                     help='Android architecture to use (armv6, armv7).')

def configure(ctx):
    # System waf is running on: linux, darwin (Mac OSX), freebsd, windows, etc.
    if ctx.options.cross:
        ctx.env.target = ctx.options.cross
    else:
        ctx.env.target = platform.system().lower()

    # Check if cross-compiling
    if ctx.env.target == 'android':
        ctx.env.android_ndk = ctx.options.cross_android_ndk
        ctx.env.android_level = ctx.options.cross_android_level
        ctx.env.android_arch = ctx.options.cross_android_arch
        if ctx.options.cross_android_arch == 'armv7':
            arch = 'arm'
            ctx.env.CFLAGS += ['-march=armv7-a', '-mfloat-abi=softfp']
            ctx.env.LINKFLAGS += ['-Wl,--fix-cortex-a8']
        elif ctx.options.cross_android_arch == 'armv6':
            arch = 'arm'
        else:
            arch = ctx.options.cross_android_arch

        path = os.path.join(ctx.options.cross_android_ndk, 'toolchains',
                            'arm-linux-androideabi-4.4.3', 'prebuilt',
                            'linux-x86', 'bin')
        ctx.find_program('arm-linux-androideabi-gcc', var='CC',
                         path_list=[path])
        ctx.find_program('arm-linux-androideabi-g++', var='CXX',
                         path_list=[path])
        ctx.find_program('arm-linux-androideabi-as', var='AS',
                         path_list=[path])

        ctx.env.android_sysroot = os.path.join(
                ctx.options.cross_android_ndk, 'platforms',
                'android-{0}'.format( ctx.options.cross_android_level),
                'arch-{0}'.format(arch))

        ctx.env.CFLAGS += ['--sysroot=' + ctx.env.android_sysroot]
        ctx.env.LINKFLAGS += ['--sysroot=' + ctx.env.android_sysroot]

    ctx.load('compiler_c')

    ctx.env.arch = platform.machine()
    ctx.env.test = ctx.options.test

    # Mac OSX's uuid stuff is built into its libc
    if ctx.env.target != 'darwin':
        ctx.check_cc(lib='uuid')

    ctx.check_cc(lib='dl')
    ctx.check_cc(lib='expat')
    ctx.check_cc(lib='crypto')
    ctx.check_cc(lib='ssl', use='CRYPTO')

    if ctx.env.test:
        ctx.load('gas')
        ctx.find_program('nm')
        ctx.find_program('objdump')
        ctx.find_program('objcopy')
        ctx.find_program('as')
        ctx.find_program('grep')
        ctx.find_program('awk')
        ctx.find_program('sed')

    if ctx.env.CC_NAME == 'gcc':
        ctx.env.CFLAGS += ['-std=gnu99', '-Wall', '-Wextra', '-Werror',
                           '-Wno-unused-parameter']

        if ctx.env.target == 'darwin':
            ctx.env.CFLAGS += ['-Wno-deprecated-declarations']

        ctx.env.LINKFLAGS_DYNAMIC += ['-export-dynamic']

        if ctx.options.debug:
            ctx.env.CFLAGS += ['-O0', '-g']
        else:
            ctx.env.CFLAGS += ['-O3']
            ctx.env.DEFINES += ['NDEBUG']

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
        use = ['DYNAMIC', 'DL', 'EXPAT', 'CRYPTO', 'SSL', 'UUID'],
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

    ctx.program(
        target = 'xmp3',
        source = [
            'src/main.c',
        ],
        use = ['libxmp3'],
    )

    # Don't need to link modules with libxmp3, bind symbols at runtime.
    ctx.shlib(
        target = 'xep_muc',
        includes = libxmp3.includes,
        source = ['src/xep_muc.c'],
    )

    ctx.shlib(
        target = 'xmp3_multicast',
        includes = libxmp3.includes,
        source = ['src/xmp3_multicast.c'],
    )

def test(ctx):
    if not ctx.env.test:
        ctx.fatal('Run configure with "--test" option first.')

    make_test(ctx, 'jid', ['src/utils.c'], ['UUID'])
    make_test(ctx, 'utils', extra_use=['UUID'])
    make_test(ctx, 'xmpp_stanza', ['src/xmpp_parser.c', 'src/utils.c'],
              ['UUID', 'EXPAT'])
    make_test(ctx, 'xmpp_parser', ['src/xmpp_stanza.c', 'src/utils.c'],
              ['UUID', 'EXPAT']);


##################################################
# Should not need to change anything below here. #
##################################################

class TestContext(waflib.Build.BuildContext):
    cmd = 'test'
    fun = 'test'

def make_test(ctx, target, extra_source=None, extra_use=None):
    if extra_source is None:
        extra_source = []

    if extra_use is None:
        extra_use = []

    ctx.program(
        target = target + '_test',
        features = ['test-dept'],
        use = extra_use,
        includes = [
            'src',
            'lib/uthash/src',
            'lib/inih',
            'lib/tj-tools/src',
            'lib/test-dept/src',
            ],
        source = [
            'src/{0}.c'.format(target),
            'test/{0}_test.c'.format(target),
        ] + extra_source,
    )

@waflib.TaskGen.feature('test-dept')
@waflib.TaskGen.before('apply_link')
@waflib.TaskGen.after('process_source')
def test_dept(self):
    # Assumes sources in particular order, should be done better.
    mod_src = self.source[0]
    test_src = self.source[1]

    # Find the compilation task associated with compiling each file
    for t in self.compiled_tasks:
        if test_src in t.inputs:
            test_task = t
        elif mod_src in t.inputs:
            mod_task = t

    # We need to compile the main module, but not link it in with the rest of
    # the test program.  test-dept will extract symbols from it to build a
    # proxy object.
    self.compiled_tasks.remove(mod_task)

    # Make a node referencing the output directory
    dir_node = test_src.parent.get_bld()

    main_c = self.create_task('test_dept_main', test_task.outputs,
                              dir_node.make_node(self.target + '_main.c'))

    main_o = self.create_task('c', main_c.outputs,
        dir_node.make_node(self.target + '_without_proxies.o'))
    self.compiled_tasks.append(main_o)

    undef_syms = self.create_task('test_dept_undef_syms', mod_task.outputs,
        dir_node.make_node(self.target + '_undef_syms.txt'))

    tmp = mod_task.outputs[:]
    for t in self.compiled_tasks:
        tmp += t.outputs
    main_wo_proxies_prog = self.create_task('cprogram', tmp,
            dir_node.make_node(self.target + '_without_proxies'))

    accessible_funcs = self.create_task('test_dept_accessible_functions',
            main_wo_proxies_prog.outputs,
            dir_node.make_node(self.target + '_accessible_functions.txt'))

    replacement_symbols = self.create_task('test_dept_replacement_symbols',
            undef_syms.outputs + accessible_funcs.outputs,
            dir_node.make_node(self.target + '_replacement_symbols.txt'))

    using_proxies = self.create_task('test_dept_using_proxies',
            replacement_symbols.outputs + mod_task.outputs,
            dir_node.make_node(self.target + '_using_proxies.o'))
    self.compiled_tasks.append(using_proxies)

    proxies = self.create_task('test_dept_proxies',
            mod_task.outputs + main_o.outputs,
            dir_node.make_node(self.target + '_proxies.s'))

    self.create_compiled_task('asm', proxies.outputs[0])

@waflib.TaskGen.feature('test-dept')
@waflib.TaskGen.after('apply_link')
def test_dept_run(self):
    self.create_task('test_dept_execute', self.link_task.outputs
            ).set_run_after(self.link_task)

class test_dept_main(waflib.Task.Task):
    run_str = '${NM} -p ${SRC} | ../lib/test-dept/src/build_main_from_symbols > ${TGT}'

class test_dept_undef_syms(waflib.Task.Task):
    run_str = '${NM} -p ${SRC} | ${AWK} \'/ U / {print $(NF-1) " " $(NF) " "}\' | ${SED} \'s/[^A-Za-z_0-9 ].*$//\' > ${TGT} || true'

class test_dept_accessible_functions(waflib.Task.Task):
    run_str = '${OBJDUMP} -t ${SRC} | ${AWK} \'$3 == "F"||$2 == "F" {print "U " $NF " "}\' | ${SED} \'s/@@.*$/ /\' > ${TGT}'

class test_dept_replacement_symbols(waflib.Task.Task):
    # -f {undef_syms.txt} {accessible_functions.txt}
    run_str = '${GREP} -f ${SRC} | ../lib/test-dept/src/sym2repl > ${TGT} || true'

class test_dept_using_proxies(waflib.Task.Task):
    # --redefine-syms=replacement_symbols.txt *.o
    run_str = '${OBJCOPY} --redefine-syms ${SRC} ${TGT}'

class test_dept_proxies(waflib.Task.Task):
    run_str = '../lib/test-dept/src/sym2asm ${SRC} ../lib/test-dept/src/sym2asm_${arch}.awk nm > ${TGT}'

@waflib.Task.always_run
class test_dept_execute(waflib.Task.Task):
    run_str = '../lib/test-dept/src/test_dept ${SRC}'
