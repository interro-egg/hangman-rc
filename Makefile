# This makefile should be run from the root of the project.

CC ?= gcc
LD ?= gcc
FORMATTER ?= clang-format

SRC := src

PLAYER_SOURCES := $(wildcard $(SRC)/player/*.c)
PLAYER_HEADERS := $(PLAYER_SOURCES:.c=.h)
PLAYER_OBJECTS := $(PLAYER_SOURCES:.c=.o)
PLAYER_EXEC := player

GS_SOURCES := $(wildcard $(SRC)/GS/*.c)
GS_HEADERS := $(GS_SOURCES:.c=.h)
GS_OBJECTS := $(GS_SOURCES:.c=.o)
GS_EXEC := GS

COMMON_SOURCES := $(wildcard $(SRC)/common/*.c)
COMMON_HEADERS := $(COMMON_SOURCES:.c=.h)
COMMON_OBJECTS := $(COMMON_SOURCES:.c=.o)

SOURCES := $(PLAYER_SOURCES) $(GS_SOURCES) $(COMMON_SOURCES)
OBJECTS := $(PLAYER_OBJECTS) $(GS_OBJECTS) $(COMMON_OBJECTS)
TARGET_EXECS := $(PLAYER_EXEC) $(GS_EXEC)

# Execution artifacts
ARTIFACTS := $(filter-out word_eng.txt, $(wildcard *.txt)) $(wildcard *.jpeg) $(wildcard *.jpg) $(wildcard *.png) $(wildcard *.gif) $(wildcard *.svg) GAMES SCORES

CFLAGS += -std=c17 -D_POSIX_C_SOURCE=200809L
CFLAGS += $(INCLUDES)

# Warnings
CFLAGS += -fdiagnostics-color=always -Wall -Werror -Wextra -Wcast-align -Wconversion -Wfloat-equal -Wformat=2 -Wnull-dereference -Wshadow -Wsign-conversion -Wswitch-default -Wswitch-enum -Wundef -Wunreachable-code -Wunused

# optional debug symbols: run make DEBUG=yes to activate them
ifeq ($(strip $(DEBUG)), yes)
  CFLAGS += -g
endif

# optional O3 optimization symbols: run make OPTIM=no to deactivate them
ifeq ($(strip $(OPTIM)), no)
  CFLAGS += -O0
else
  CFLAGS += -O3
endif

# convenience variables for extending compiler options (e.g. to add sanitizers)
CFLAGS += $(EXTRA_CFLAGS)
LDFLAGS += $(EXTRA_LDFLAGS)

.SECONDEXPANSION:

%: src/%/$$@.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

.PHONY: all clean fmt fmt-check

# must be the first target in the Makefile
# (the name "all" is not special, just a convention)
all: $(TARGET_EXECS)

$(PLAYER_EXEC): $(PLAYER_OBJECTS) $(COMMON_OBJECTS)
$(GS_EXEC): $(GS_OBJECTS) $(COMMON_OBJECTS)

clean: 
	rm -f $(TARGET_EXECS) $(OBJECTS)

clean-artifacts:
	rm -rf $(ARTIFACTS)

fmt: $(SOURCES) $(HEADERS)
	$(FORMATTER) -i $^

fmt-check: $(SOURCES) $(HEADERS)
	$(FORMATTER) --dry-run --Werror $^