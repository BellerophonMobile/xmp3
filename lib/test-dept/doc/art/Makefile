SOURCES=$(wildcard *.dia)
PNGS=$(patsubst %.dia,%.png,$(SOURCES))

%.png:	%.dia
	dia --export=$@ $<

all:	$(PNGS)
