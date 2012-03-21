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
  print    "	.text"
  if (num_functions > 0) for (fun in functions) {
     proxy = fun "_test_dept_proxy"
     proxy_ptr = proxy "_ptr"
     print ".globl " proxy
     print proxy ":"
     print "	movq	" proxy_ptr "(%rip), %r11"
     print "	jmp	*%r11"
  }
  print    ""
  print    "	.data"
  print    "	.align 16"
  if (num_functions > 0) for (fun in functions) {
     proxy_ptr = fun "_test_dept_proxy_ptr"
     print ".globl " proxy_ptr
     print proxy_ptr ":"
     print "	.quad	" fun
     print "	.quad	" fun
  }
  print    ".globl " proxy_ptrs_variable
  print    "	.data"
  print    "	.align 8"
  print    proxy_ptrs_variable ":"
  if (num_functions > 0) for (fun in functions) {
     proxy_ptr = fun "_test_dept_proxy_ptr"
     print "	.quad	" proxy_ptr
  }
  print    "	.quad	0"
}
