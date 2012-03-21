/* Copyright 2008--2011 Mattias Norrby
 * 
 * This file is part of Test Dept..
 * 
 * Test Dept. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Test Dept. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Test Dept..  If not, see <http://www.gnu.org/licenses/>.
 */

#include <test-dept.h>
#include "foo.h"
#include "sit.h"

void test_normal_fooify() {
  int unused = 8;
  foo(unused);
  assert_equals(3, three());
}

void test_external_variable() {
  assert_equals(7, ext);
  add_to_ext(2);
  assert_equals(9, ext);
}
