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

# A simple test project with one C file as SUT. This project has
# no stubs, but it is easily added using the TEST_DEPT_POSSIBLE_STUBS
# variable pointing to a header file stating prototypes for stub
# functions.

CC=gcc

# The three rows below are only needed if test-dept is not installed
# in your environment. They refer relatively to the unpacked dist in ../..
#TEST_DEPT_BIN_PATH=../../src
#TEST_DEPT_INCLUDE_PATH=$(TEST_DEPT_BIN_PATH)
#CPPFLAGS=-I$(TEST_DEPT_INCLUDE_PATH) -I.
# If you use them, please also qualify the last include-file with
# a prefix, like this: $(TEST_DEPT_INCLUDE_PATH)/test-dept.mk

override CPPFLAGS+=-DTEST_DEPT_USE_PREFIX

# In this simple project all TEST_SRCS need to have a corresponding
# C-file. For example, car_test.c _needs_ a car.c for it to work.
# (If you do not desire this, include test_dept_rudimentary.mk instead
# and specify the rules by yourself. All programs are still available)
TEST_SRCS=car_test.c

check:	test_dept_run
clean: 
	rm -f main_* *_test *.o *~ *_stubs.c

# Rules for making test binaries reside here
include test-dept.mk
