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

$(TEST_MAINS):	%_test:	%_using_proxies.o %_proxies.o
$(TEST_MAINS_WITHOUT_PROXIES):	%_test_without_proxies:	%.o

%_replacement_symbols.txt:	%_undef_syms.txt %_accessible_functions.txt
	grep -f $^ | $(TEST_DEPT_RUNTIME_PREFIX)sym2repl >$@ || true

%_undef_syms.txt:        %.o
	$(NM) -p $< | awk '/ U / {print $$(NF-1) " " $$(NF) " "}' |\
                      sed 's/[^A-Za-z_0-9 ].*$$//' >$@ || true

%_accessible_functions.txt:	%_test_without_proxies
	$(OBJDUMP) -t $< | awk '$$3 == "F"||$$2 == "F" {print "U " $$NF " "}' |\
                           sed 's/@@.*$$/ /' >$@

%_proxies.s:	%.o %_test_main.o
	$(TEST_DEPT_RUNTIME_PREFIX)sym2asm $^ $(SYMBOLS_TO_ASM) $(NM) >$@


%_using_proxies.o:	%_replacement_symbols.txt %.o
	$(OBJCOPY) --redefine-syms=$^ $@

ifneq (,$(TEST_DEPT_INCLUDE_PATH))
TEST_DEPT_MAKEFILE_INCLUDE_PATH=$(TEST_DEPT_INCLUDE_PATH)/
endif
include $(TEST_DEPT_MAKEFILE_INCLUDE_PATH)test-dept-definitions.mk
