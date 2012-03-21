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

# An example project using test_dept in its cstub form without
# the simple automated stuff. This is not recommended!

CC=gcc

# The three rows below are only needed if test-dept is not installed
# in your environment. They refer relatively to the unpacked dist in ../..
#TEST_DEPT_BIN_PATH=../../src
#TEST_DEPT_INCLUDE_PATH=$(TEST_DEPT_BIN_PATH)
#CPPFLAGS=-I$(TEST_DEPT_INCLUDE_PATH) -I.
# If you use them, please also qualify the include-file with
# a prefix, like this: $(TEST_DEPT_INCLUDE_PATH)/test-dept.mk

# All test cases reside in TEST_SRCS
TEST_SRCS=sum_test.c file_ops_test.c instruments_test.c another_sum_test.c

# A file which declares all possible stub functions
TEST_DEPT_POSSIBLE_STUBS=stubs.h
TEST_DEPT_FUNCTION_SWITCH_HEADER=stub_headers.h

# Dependencies for each test binary. Should be self explanatory
sum_test another_sum_test:	dependency.o

# Since test cases named <prefix>_test.o expects a <prefix>.o to exist
# our special another_sum_test needs an another_sum.o which of course
# is identical to sum.o
another_sum.o: sum.o
	cp $< $@

check:	test_dept_run

clean:
	rm -f *.o *_test main_*.c *_stubs.* *~

include test-dept-cstubs.mk
