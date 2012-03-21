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
#include "dependency.h"

int calculate_sum(int lhs, int rhs) {
  return lhs + rhs;
}

int calculate_3sum(int s1, int s2, int s3) {
  return s1 + s2 + s3;
}

void *go_fish(size_t size) {
  static int storage[0];
  if (!create_file("/tmp/lockfile"))
    return NULL;
  return storage;
}

int *go_skiing() {
  static int storage[0];
  if (!(return_three() == 3))
    return NULL;
  return storage;
}
