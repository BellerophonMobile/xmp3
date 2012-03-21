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

#include "file_ops.h"

#include <string.h>
#include <stdio.h>

int count_users() {
  FILE *file = fopen("/etc/passwd", "r");
  char line[1024];
  int lines = 0;
  if (NULL == file)
    return 0;  
  while(fgets(line, sizeof(line), file) != NULL) {
    int len = strlen(line) - 1;
    if(line[len] == '\n') 
      line[len] = 0;
    lines++;
  }
  fclose(file);
  return lines;
}
