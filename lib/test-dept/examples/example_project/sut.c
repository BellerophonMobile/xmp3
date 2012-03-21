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

#include "sut.h"

#include <foo.h>
#include <bar.h>
#include <stdlib.h>

int fooify(int value) {
  int result = foo(value);
  const int unexpected = result <= 0;
  if (unexpected)
    return -1;
  return result;
}

float barify(float value) {
  float result = bar(value);
  const int unexpected = result > 1000;
  if (unexpected)
    return 0.3f;
  return result;
}

char *stringify(char value) {
  char *fie = malloc(2 * sizeof(char));
  if (fie == NULL)
    return "cannot_stringify";
  fie[0] = value;
  fie[1] = '\0';
  return fie;
}
