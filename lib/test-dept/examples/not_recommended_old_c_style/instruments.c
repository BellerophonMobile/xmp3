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

#include "instruments.h"

#include <stdio.h>

static const char* prefix;

static char return_string_buffer[255];

static const char *with_prefix(const char *instrument) {
  sprintf(return_string_buffer, "%s: %s", prefix, instrument);
  return return_string_buffer;
}

const char *bar() {
  return with_prefix("basoon");
}

const char *fie() {
  return with_prefix("trumpet");
}

void set_printing_prefix(const char* printing_prefix) {
  prefix = printing_prefix;
}
