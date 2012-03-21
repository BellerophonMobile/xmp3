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

/* A simple test case that uses prefixed assertions selected
 * by a CPPFLAGS flag in the Makefile */

#include "car.h"

#include <test-dept.h>

void test_number_of_wheels() {
  test_dept_assert_equals(4, number_of_wheels());
}

void test_drive_short() {
  const int can_drive_this_many_miles = miles_per_tank();
  test_dept_assert_true(can_drive(can_drive_this_many_miles));
}

void test_drive_long() {
  const int can_drive_this_many_miles = miles_per_tank();
  const int too_long_drive = can_drive_this_many_miles + 1;
  test_dept_assert_false(can_drive(too_long_drive));
}
