# Copyright 2008 Mattias Norrby
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

CC=gcc

# The three rows below are only needed if test-dept is not installed
# in your environment. They refer relatively to the unpacked dist in ../..
#TEST_DEPT_BIN_PATH=../../src
#TEST_DEPT_INCLUDE_PATH=$(TEST_DEPT_BIN_PATH)
#CPPFLAGS=-I$(TEST_DEPT_INCLUDE_PATH) -I.
# If you use them, please also qualify the last include-file with
# a prefix, like this: $(TEST_DEPT_INCLUDE_PATH)/test-dept.mk

override CPPFLAGS+=-I.
TEST_SRCS=assertion_test.c prepost_test.c

check:	test_dept_run

clean: 
	rm -f *_main.c *_test *.o *~ *_stubs.c

#in this project, the rudimentary mk-file is included due to not
#wanting to require an prepost.c and assertion.c file that normall
#is required by the framework
include test-dept-rudimentary.mk
