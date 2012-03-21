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

void test_function_without_assertions() {
}

void test_function_with_assertions() {
  assert_true(1);
}

void test_assert_equals_with_integers() {
  assert_equals(0, 0);
  assert_equals(1, 1);
  assert_equals(-1, -1);
  assert_equals(65536, 65536);
  assert_equals(-65536, -65536); 
}

void test_assert_equals_with_floats() {
  assert_equals(0.0, 0.0);
  assert_equals(1.0, 1.0);
  assert_equals(-1.1, -1.1);
  assert_equals(-1.191919191, -1.191919191);
}

void test_for_loop_around_assertion() {
  int i, j;
  for (i =j = 0; i < 1000; i++, j++)
    assert_equals(i, j);
}

void test_assert_not_equals_with_integers() {
  assert_not_equals(1, 0);
  assert_not_equals(1, -1);
  assert_not_equals(65536, -65536);
}

void test_assert_equals_with_short() {
  short shorty = 7;
  short shorty2 = shorty;
  assert_equals(7, shorty);
  assert_equals(shorty2, shorty);
}

void test_assert_false() {
  assert_false(0);
}

void test_assert_equals() {
  int three = 3;
  char *pointer = (char *) &three;
  char *foo = pointer;
  assert_equals(3, three);
  assert_equals(pointer, foo);
}

void test_longer() {
  long l = 1L;
  long ll = 1LL;
  unsigned long ul = 1UL;
  unsigned long long ull = 1ULL;
  long double ld = 1;
  assert_equals(1L, l);
  assert_equals(1LL, ll);
  assert_equals(1UL, ul);
  assert_equals(1ULL, ull);
  assert_equals(ld, ld);
}

void test_fisk() {
  assert_equals(1, 1);
}

void test_assert_pointer_equals() {
  int three = 3;
  void *pointer = (void *) &three;
  char *foo = pointer;
  assert_pointer_equals(pointer, foo);
}

void test_assert_equals_char() {
  char a = 'A';
  assert_equals((char) 'A', a);
}

void test_assert_equals_float() {
  float pi = 3.1f;
  assert_equals(3.1f, pi);
}

void test_assert_equals_double() {
  double pi = 3.1;
  assert_equals(3.1, pi);
}

void test_assert_string_equals() {
  char* front = "242";
  assert_string_equals("242", front);
  unsigned char* Front = (unsigned char*) "242";
  assert_string_equals("242", Front);
}
