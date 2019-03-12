# ========================================================================================
# Makefile for lbgpsdo
# ========================================================================================

# ========================================================================================
# Compile flags

CC = gcc
COPT = -O2
CFLAGS = -Wall -Wextra -Wpedantic -Werror -std=gnu11 -D_GNU_SOURCE
CFLAGS += -D BUILD_VERSION="\"$(shell git describe --dirty --always)\""	\
		-D BUILD_DATE="\"$(shell date '+%Y-%m-%d_%H:%M:%S')\""

BIN = lbgpsdo

# ========================================================================================
# Source files

SRCDIR = .

SRC = $(SRCDIR)/main.c

# ========================================================================================
# External Libraries

LIBSDIR = 

LIBS = -ludev

# ========================================================================================
# Makerules

all:
	$(CC) $(COPT) $(CFLAGS) $(SRC) -o $(BIN) $(LIBSDIR) $(LIBS)

debug: COPT = -Og -ggdb -fno-omit-frame-pointer -D__DEBUG
debug: all

clean:
	rm -fv $(BIN)