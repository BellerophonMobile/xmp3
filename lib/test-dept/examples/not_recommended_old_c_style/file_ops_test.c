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

#include "file_ops.h"

#include <stdlib.h>

FILE* unsuccessful_fopen(const char *path, const char *mode) {
  return NULL;
}

void test_more_than_zero_users() {
  assert_not_equals(0, count_users());
}

void test_cannot_open_file_results_in_zero_users() {
  test_dept_fopen_set(&unsuccessful_fopen);
  assert_equals(0, count_users());
}
