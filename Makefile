CC ?= cc
LD ?= ld
INCLUDE = 

SRC = a_string.c lexer.c ast.c ast_printer.c parser/parser.c parser/expr.c parser/stmt.c compiler/compiler.c compiler/expr.c compiler/stmt.c
OBJ = $(SRC:.c=.o)
HEADERS = common.h a_vector.h a_string.h lexer.h ast.h ast_printer.h parser/parser.h parser/parser_internal.h compiler/compiler.h compiler/compiler_internal.h

CFLAGS = -Wall -Wextra -pedantic
RELEASE_CFLAGS = -O2
DEBUG_CFLAGS = -D_A_STRING_DEBUG -O0 -ggdb3 -fsanitize=address
TARBALLFILES = Makefile LICENSE.md README.md 3rdparty $(SRC) $(HEADERS) main.c 

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

cbc: deps $(OBJ) $(HEADERS) main.o
	$(CC) $(CFLAGS) -o cbc main.o $(OBJ)

main.o: main.c common.h
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.c %.h common.h
	$(CC) -c $(CFLAGS) -o $@ $<

dep_uthash:
	mkdir -p 3rdparty/
	if [ ! -f 3rdparty/uthash.h ]; then \
		curl -fL -o 3rdparty/uthash.h https://raw.githubusercontent.com/troydhanson/uthash/refs/heads/master/src/uthash.h; \
	fi

deps: dep_uthash

tarball:
	mkdir -p cbc
	cp -r $(TARBALLFILES) cbc/
	tar czf cbc.tar.gz cbc
	rm -rf cbc

distclean: clean cleandeps

clean:
	rm -rf cbc cbc.tar.gz cbc $(OBJ) main.o

.PHONY: clean cleanall
