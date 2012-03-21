# Copyright 2008--2010 Mattias Norrby
#
# This file is part of Test Dept..
#
# Test Dept. is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Test Dept. is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Test Dept..  If not, see <http://www.gnu.org/licenses/>.
#
# As a special exception, you may use this file as part of a free
# testing framework without restriction.  Specifically, if other files
# include this makefile for contructing test-cases this file does not
# by itself cause the resulting executable to be covered by the GNU
# General Public License.  This exception does not however invalidate
# any other reasons why the executable file might be covered by the
# GNU General Public License.

TEST_MAIN_SRCS=$(notdir $(patsubst %.c,%_main.c,$(TEST_SRCS)))
TEST_MAIN_OBJS=$(patsubst %.c,%.o,$(TEST_MAIN_SRCS))
TEST_MAINS=$(patsubst %_main.c,%,$(TEST_MAIN_SRCS))
TEST_OBJS=$(notdir $(patsubst %.c,%.o,$(TEST_SRCS)))

%_replacement_symbols.txt:	%_test.o
	$(NM) -p $< | $(TEST_DEPT_RUNTIME_PREFIX)csym2repl >$@

%_proxies.c: $(TEST_DEPT_POSSIBLE_STUBS) %_test.o 
	$(NM) -p $*_test.o |\
        $(TEST_DEPT_RUNTIME_PREFIX)build_c_proxies $< |\
        $(TEST_DEPT_RUNTIME_PREFIX)build_c_proxy >$@

$(TEST_DEPT_FUNCTION_SWITCH_HEADER): $(TEST_DEPT_POSSIBLE_STUBS)
	$(TEST_DEPT_RUNTIME_PREFIX)build_stub_headers $^ >$@

$(TEST_OBJS): %_test.o: %_test.c $(TEST_DEPT_FUNCTION_SWITCH_HEADER)

ifneq (,$(TEST_DEPT_INCLUDE_PATH))
TEST_DEPT_MAKEFILE_INCLUDE_PATH=$(TEST_DEPT_INCLUDE_PATH)/
endif
include $(TEST_DEPT_MAKEFILE_INCLUDE_PATH)test-dept-proxies.mk
