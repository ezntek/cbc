CC ?= cc
LD ?= ld
INCLUDE = 

SRC = src/a_string.c src/a_string_slice.c src/util.c src/error.c src/lexer.c src/lexer_types.c
OBJ = $(DEPS) $(SRC:.c=.o)
HEADERS = src/a_vector.h src/common.h $(SRC:.c=.h)

CFLAGS = -Wall -Wextra -pedantic
RELEASE_CFLAGS = -O2
DEBUG_CFLAGS = -D_A_STRING_DEBUG -O0 -ggdb3 -fsanitize=address
TARBALLFILES = Makefile LICENSE.md README.md 3rdparty src 

TARGET=debug

ifeq (,$(filter clean cleandeps,$(MAKECMDGOALS)))

# goodbye windowze™
ifeq ($(OS),Windows_NT)
$(error building on Windows is not supported.)
endif

ifeq (,$(shell command -v curl))
$(error curl is not installed on your system.)
endif

ifeq (,$(shell command -v qbe))
$(error qbe is not installed on your system.)
endif

ifeq ($(TARGET),debug)
CFLAGS += $(DEBUG_CFLAGS)
else
CFLAGS += $(RELEASE_CFLAGS)
endif

CFLAGS += $(INCLUDE)

endif

cbc: deps $(OBJ) $(HEADERS) src/main.o
	$(CC) $(CFLAGS) -o cbc src/main.o $(OBJ)

%.o: %.c %.h common.h
	$(CC) -c $(CFLAGS) -o $@ $<

tarball:
	mkdir -p cbc
	cp -r $(TARBALLFILES) cbc/
	tar czf cbc.tar.gz cbc
	rm -rf cbc

deps:

cleandeps:

distclean: clean cleandeps

clean:
	rm -rf cbc cbc.tar.gz cbc $(OBJ) main.o

.PHONY: clean cleanall
