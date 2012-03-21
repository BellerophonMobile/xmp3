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
# As a special exception, you may use output from this script file
# that stems from this file without restriction as part of test cases.
# Specifically, if other programs use output from this script for
# constructing test-cases the output source code that stems from
# templates in this file does not by itself cause the resulting
# executable to be covered by the GNU General Public License.  This
# exception does not however invalidate any other reasons why the
# executable file might be covered by the GNU General Public License.

BEGIN { num_functions = 0 }
/ U / { functions[$NF]; num_functions = num_functions + 1 }
END {
  print    "	.section	\".text\""
  if (num_functions > 0) for (fun in functions) {
     proxy = fun "_test_dept_proxy"
     print ""
     print "	.global	" proxy
     print "	.type	" proxy ", #object"
     print "	.align	16"
     print proxy ":"
     proxy_ptr = proxy "_ptr"
     print "	set	" proxy "_ptr, %l0"
     print "	ld	[%l0], %l1"
     print "	jmp	%l1"
     print "	nop"
     print "	.size	" proxy ", .-" proxy
  }
  print    ""
  print    "	.section	\".data\""
  if (num_functions > 0) for (fun in functions) {
     proxy_ptr = fun "_test_dept_proxy_ptr"
     print "	.global " proxy_ptr
     print "	.type	" proxy_ptr ", #object"
     print "	.align	16"
     print "	.size	" proxy_ptr ", 8"
     print proxy_ptr ":"
     print "	.long	" fun
     print "	.long	" fun
     print ""
  }
  print    "	.align	16"
  print    "	.global	" proxy_ptrs_variable
  print    "	.type	" proxy_ptrs_variable ", #object"
  print    "	.size	" proxy_ptrs_variable ", " num_functions * 4 + 8
  print    "test_dept_proxy_ptrs:"
  if (num_functions > 0) for (fun in functions) {
     proxy_ptr = fun "_test_dept_proxy_ptr"
     print "	.long	" proxy_ptr
  }
  print    "	.long	0"
}
