_check_%:	%
	cp -rp $< $@

TEST_STAMPS=$(addsuffix /.tested,$(TEST_DIRS))
REL_SRC=../$(top_srcdir)/src
MAKE:=env PATH=$(REL_SRC):$$PATH $(MAKE) -I $(REL_SRC) CPPFLAGS=-I$(REL_SRC)
all:	$(TEST_DIRS)

%/.tested:	%
	$(MAKE) -C $< check

check:	$(TEST_STAMPS)
clean::
	rm -rf $(TEST_DIRS)
