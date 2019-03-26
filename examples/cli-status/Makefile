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

LIBLBGPSDODIR = ../..

SRC = $(LIBLBGPSDODIR)/liblbgpsdo.c \
	$(SRCDIR)/main.c

# ========================================================================================
# External Libraries

LIBSDIR = $(LIBLBGPSDODIR)

LIBS = -ludev

# ========================================================================================
# Makerules

all:
	$(CC) $(COPT) $(CFLAGS) $(SRC) -o $(BIN) -I $(LIBSDIR) $(LIBS)

debug: COPT = -Og -ggdb -fno-omit-frame-pointer -D__DEBUG
debug: all

clean:
	rm -fv $(BIN)