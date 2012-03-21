/* Copyright 2008 Mattias Norrby
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

#include "instruments.h"

void test_foo() {
  assert_string_equals("instrument: basoon", bar());
}

void test_fie() {
  assert_string_equals("instrument: trumpet", fie());
}

void test_change_prefix() {
  assert_string_equals("instrument: basoon", bar());
  set_printing_prefix("paint");
  assert_string_equals("paint: basoon", bar());
}

void setup() {
  set_printing_prefix("instrument");
}
