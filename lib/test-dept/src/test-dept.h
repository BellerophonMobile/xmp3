/* Copyright 2008--2010 Mattias Norrby
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
 *
 * As a special exception, you may use this file as part of a free
 * testing framework without restriction.  Specifically, if other
 * files use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable,
 * this file does not by itself cause the resulting executable to be
 * covered by the GNU General Public License.  This exception does not
 * however invalidate any other reasons why the executable file might
 * be covered by the GNU General Public License.
 */

#ifndef _TEST_DEPT_H_
#define _TEST_DEPT_H_

#include <stdio.h>
#include <string.h>
#include <assert.h>

#define _test_dept_assert_condition(condition, textual_condition) do {\
  test_dept_tests_run += 1;\
  if (!(condition)) {\
    test_dept_test_failures += 1;\
    fprintf(stderr, "%s:%d: Failure: expected that %s\n",\
            __FILE__, __LINE__, textual_condition);\
    return;\
  }\
} while(0)

#ifdef TEST_DEPT_USE_PREFIX
# define test_dept_assert_true _test_dept_assert_true
# define test_dept_assert_false _test_dept_assert_false
# define test_dept_assert_equals _test_dept_assert_equals
# define test_dept_assert_string_equals _test_dept_assert_string_equals
# define test_dept_assert_pointer_equals _test_dept_assert_pointer_equals
# define test_dept_assert_not_equals _test_dept_assert_not_equals
# define test_dept_replace_function _test_dept_replace_function
# define test_dept_restore_function _test_dept_restore_function
# define test_dept_fail_test _test_dept_fail_test
# define test_dept_assert_equals_int _test_dept_use_assert_equals
# define test_dept_assert_equals_float _test_dept_use_assert_equals
# define test_dept_assert_equals_double _test_dept_use_assert_equals
# define test_dept_assert_equals_string _test_dept_use_assert_string_equals
#else
# define assert_true _test_dept_assert_true
# define assert_false _test_dept_assert_false
# define assert_equals _test_dept_assert_equals
# define assert_not_equals _test_dept_assert_not_equals
# define assert_pointer_equals _test_dept_assert_pointer_equals
# define assert_string_equals _test_dept_assert_string_equals
# define replace_function _test_dept_replace_function
# define restore_function _test_dept_restore_function
# define fail_test _test_dept_fail_test
# define assert_equals_int _test_dept_use_assert_equals
# define assert_equals_float _test_dept_use_assert_equals
# define assert_equals_double _test_dept_use_assert_equals
# define assert_equals_string _test_dept_use_assert_string_equals
#endif

#define _test_dept_fail_test(msg) do {\
    test_dept_test_failures += 1;\
    fprintf(stderr, "%s:%d: Failure: %s\n", __FILE__, __LINE__, msg);	\
    return;\
   } while (0)

#define _test_dept_use_assert_equals(a, b)				\
  _test_dept_fail_test("obsolete assertion. Use assert_equals(...) instead.")

#define _test_dept_use_assert_string_equals(a, b)			\
  _test_dept_fail_test("obsolete assertion. Use assert_string_equals(...) instead.")

#define _test_dept_assert_not_equals(a, b)\
 _test_dept_assert_condition((a) != (b), "(" # a ") != (" # b ")" )

#define _test_dept_assert_true(condition)	\
 _test_dept_assert_condition((condition), "(" # condition ")" )

#define _test_dept_assert_false(condition)\
 _test_dept_assert_condition(!(condition), "!(" # condition ")" )

#define _test_dept_assert_pointer_equals(exp, act) do {\
    const void *actual = (void *) act; \
    char msg[1024];\
    sprintf(msg, "%s == %p (was %p)", # act, exp, actual);	\
    _test_dept_assert_condition( (actual) == (void *) (exp), msg );	\
  } while (0)


#ifndef __STRICT_ANSI__
# define _test_dept_assert_string_equals_decl(act)	\
    typeof(""[0]) msg[TEST_DEPT_MAX_STRING_BUFFER];			\
    const typeof(&""[0]) actual = (typeof(&""[0])) (act);
#else
# define _test_dept_assert_string_equals_decl(act)	\
    char msg[TEST_DEPT_MAX_STRING_BUFFER];		\
    const char *actual = (char *) (act);
#endif

#define TEST_DEPT_MAX_COMPARISON 128
#define TEST_DEPT_MAX_STRING_BUFFER 1024
#define _test_dept_assert_string_equals(exp, act)                       \
  do {                                                                  \
    _test_dept_assert_string_equals_decl(act)				\
    if (strncmp(exp, actual, TEST_DEPT_MAX_COMPARISON) != 0) {		\
      if (strlen(exp) > TEST_DEPT_MAX_COMPARISON) {			\
        snprintf(msg, TEST_DEPT_MAX_STRING_BUFFER,			\
		 "assertion must have an expected string of max %d"	\
		 " characters", TEST_DEPT_MAX_COMPARISON);		\
	_test_dept_fail_test(msg);					\
      } else {								\
	snprintf(msg, TEST_DEPT_MAX_STRING_BUFFER,			\
		 "%s equals \"%s\" (was \"%s\")", # act, exp, actual);	\
      }									\
      _test_dept_assert_condition(strcmp(exp, actual) == 0, msg);	\
    }                                                                   \
  } while (0)

#define _test_dept_assert_equals_simple(a, b) \
 _test_dept_assert_condition((a) == (b), "(" # a ") == (" # b ")" )

#ifndef __STRICT_ANSI__
#define _test_dept_is_type(a, b)\
  __builtin_types_compatible_p(a, typeof(b))
#define _test_dept_assert_equals(exp, act)				\
  do {									\
    char msg[1024];							\
    char *fmt = NULL;							\
    if (_test_dept_is_type(char, exp))					\
      fmt = "%s == '%c' (was '%c')";					\
    if (_test_dept_is_type(unsigned char, exp))				\
      fmt = "%s == '%c' (was '%c')";					\
    else if (_test_dept_is_type(short, exp))				\
      fmt = "%s == %hd (was %hd)";					\
    else if (_test_dept_is_type(unsigned short, exp))			\
      fmt = "%s == %hu (was %hu)";					\
    else if (_test_dept_is_type(int, exp))				\
      fmt = "%s == %d (was %d)";					\
    else if (_test_dept_is_type(unsigned int, exp))			\
      fmt = "%s == %u (was %u)";					\
    else if (_test_dept_is_type(long, exp))				\
      fmt = "%s == %ld (was %ld)";					\
    else if (_test_dept_is_type(unsigned long, exp))			\
      fmt = "%s == %lu (was %lu)";					\
    else if (_test_dept_is_type(long long, exp))			\
      fmt = "%s == %lld (was %lld)";					\
    else if (_test_dept_is_type(unsigned long long, exp))		\
      fmt = "%s == %llu (was %llu)";					\
    else if (_test_dept_is_type(float, exp) || _test_dept_is_type(double, exp)) \
      fmt = "%s == %f (was %f)";					\
    else if (_test_dept_is_type(long double, exp))			\
      fmt = "%s == %Lf (was %Lf)";					\
    else if (__builtin_types_compatible_p(typeof("string"), typeof(exp))) \
      _test_dept_fail_test("Ambiguous assert. Use assert_string_equals(...)"	\
		" or assert_pointers_equal(...) instead");		\
    if (fmt) {								\
      const typeof(exp) actual = act;						\
      sprintf(msg, fmt, # act, exp, actual);				\
      _test_dept_assert_condition( actual == ( exp ), msg);		\
    } else								\
      _test_dept_assert_equals_simple(exp, act);			\
  } while (0)
#else
#define _test_dept_assert_equals(a, b)\
  _test_dept_assert_equals_simple(a, b);
#endif

void **test_dept_proxy_ptrs[2];

#define _test_dept_set_proxy(orig, repl)\
do {\
  void* original_function = (void *) orig;	\
  void* replacement_function = (void *) repl;	\
  int i;\
  for (i = 0; test_dept_proxy_ptrs[i] != NULL; i++) {\
      if (*(test_dept_proxy_ptrs[i] + 1) == original_function) {\
	  if (replacement_function)\
	    *test_dept_proxy_ptrs[i] = replacement_function;\
	  else\
	    *test_dept_proxy_ptrs[i] = *(test_dept_proxy_ptrs[i] + 1);\
	  break;\
	}\
    }\
  if (test_dept_proxy_ptrs[i] == NULL)\
    printf("%s, %d: warning, trying to replace unused function %s; generated proxy missing.\n", __FILE__, __LINE__, # orig);\
} while (0)

#ifndef __STRICT_ANSI__
#define _test_dept_replace_function(original_function, replacement_function)\
{\
  assert (replacement_function == NULL ||\
          __builtin_types_compatible_p(typeof(original_function),\
                                       typeof(replacement_function)));\
  _test_dept_set_proxy(original_function, replacement_function);\
}
#else
#define _test_dept_replace_function(original_function, replacement_function)\
{\
  _test_dept_set_proxy(original_function, replacement_function);\
}
#endif

#define _test_dept_restore_function(original_function)\
 _test_dept_set_proxy(original_function, NULL)


int test_dept_tests_run;
int test_dept_test_failures;

#endif
