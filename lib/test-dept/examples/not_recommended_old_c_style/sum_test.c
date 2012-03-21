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

#include "sum.h"

#include <test-dept.h>

#include "stub_headers.h"

int failing_create_file(const char* filename) {
  return 0;
}

int returning_two_instead_of_three() {
  return 2;
}

void test_calc() {
  assert_equals(3, calculate_sum(3, 0));
  assert_equals(4, calculate_sum(3, 1));
}

void test_calc2() {
  assert_equals(3, calculate_sum(3, 0));
}

void test_ok_dependency_function() {
  assert_not_equals(NULL, go_fish(2));
}

void test_failing_dependency_function() {
  test_dept_create_file_set(failing_create_file);
  assert_pointer_equals(NULL, go_fish(2));
}

void test_successful_skiing() {
  assert_not_equals(NULL, go_skiing());
}

void test_failing_three_function() {
  test_dept_return_three_set(returning_two_instead_of_three);
  assert_pointer_equals(NULL, go_skiing());
}
