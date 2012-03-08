#-----------------------------------------------------------------------
# Copyright (c) 2011 Joe Kopena <tjkopena@gmail.com>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#-----------------------------------------------------------------------

INCS=-Isrc/
CFLAGS+=-g -Wall

TESTS=test-tj_buffer   \
      test-tj_template \
      test-tj_heap

TEST_SRCS=$(addsuffix .c,$(addprefix test/,$(TESTS)))
TEST_TGTS=$(addprefix bin/,$(TESTS))

all: dirs   $(TEST_TGTS)
#$(addprefix bin/,$(TESTS))

dirs:
	@if [ ! -e obj ]; then mkdir -p obj; fi
	@if [ ! -e bin ]; then mkdir -p bin; fi

#-----------------------------------------------------------------------

bin/test-tj_buffer: test/test-tj_buffer.c src/tj_buffer.c src/tj_buffer.h
	$(CC) $(CFLAGS) -o $@ $^ $(INCS)

bin/test-tj_template: test/test-tj_template.c src/tj_template.c src/tj_buffer.c\
                      src/tj_template.h src/tj_buffer.h
	$(CC) $(CFLAGS) -o $@ $^ $(INCS)

bin/test-tj_heap: test/test-tj_heap.c src/tj_heap.h
	$(CC) $(CFLAGS) -o $@ $^ $(INCS)


#-----------------------------------------------------------------------
.PHONY: clean doxygen attribute

clean:
	if [ -e bin ]; then rm -rf bin; fi
	if [ -e obj ]; then rm -rf obj; fi
	if [ -e doxygen ]; then rm -rf doxygen; fi
	find . -name '*~' -delete

doxygen:
	doxygen doxygen.config

attribute:
	@for i in $(shell find src/ -name '*.c') \
                  $(shell find src/ -name '*.h'); do \
           head -n 2 $$i | grep Copyright > /dev/null; \
           if [ $$? = 1 ]; then \
             cat mit-license.txt $$i > lic-tmp; \
             mv lic-tmp $$i; \
             echo Added license to $$i; \
           fi; done
