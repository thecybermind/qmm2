# QMM2 - Q3 MultiMod 2
# Copyright 2025
# https://github.com/thecybermind/qmm2/
# 3-clause BSD license: https://opensource.org/license/bsd-3-clause
# Created By: Kevin Masterson < cybermind@gmail.com >

# QMM2 Makefile

CC := g++

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

SRC := $(wildcard $(SRC_DIR)/*.cpp)

OBJ_DIR_REL    := $(OBJ_DIR)/release
OBJ_DIR_REL_32 := $(OBJ_DIR_REL)/x86
OBJ_DIR_REL_64 := $(OBJ_DIR_REL)/x86_64
OBJ_DIR_DBG    := $(OBJ_DIR)/debug
OBJ_DIR_DBG_32 := $(OBJ_DIR_DBG)/x86
OBJ_DIR_DBG_64 := $(OBJ_DIR_DBG)/x86_64

BIN_DIR_REL    := $(BIN_DIR)/release
BIN_DIR_REL_32 := $(BIN_DIR_REL)/x86
BIN_DIR_REL_64 := $(BIN_DIR_REL)/x86_64
BIN_DIR_DBG    := $(BIN_DIR)/debug
BIN_DIR_DBG_32 := $(BIN_DIR_DBG)/x86
BIN_DIR_DBG_64 := $(BIN_DIR_DBG)/x86_64

BIN_REL_32 := $(BIN_DIR_REL_32)/qmm2.so
BIN_REL_64 := $(BIN_DIR_REL_64)/qmm2_x86_64.so
BIN_DBG_32 := $(BIN_DIR_DBG_32)/qmm2.so
BIN_DBG_64 := $(BIN_DIR_DBG_64)/qmm2_x86_64.so

OBJ_REL_32 := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR_REL_32)/%.o)
OBJ_REL_64 := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR_REL_64)/%.o)
OBJ_DBG_32 := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR_DBG_32)/%.o)
OBJ_DBG_64 := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR_DBG_64)/%.o)

CPPFLAGS := -MMD -MP -I ./include -isystem ../qmm_sdks
CFLAGS   := -Wall -pipe -fPIC -std=c++17
LDFLAGS  := -shared -fPIC
LDLIBS   :=

REL_CPPFLAGS := $(CPPFLAGS)
DBG_CPPFLAGS := $(CPPFLAGS) -D_DEBUG

REL_CFLAGS_32 := $(CFLAGS) -m32 -O2 -ffast-math -falign-loops=2 -falign-jumps=2 -falign-functions=2 -fno-strict-aliasing -fstrength-reduce -Werror
REL_CFLAGS_64 := $(CFLAGS) -O2 -ffast-math -falign-loops=2 -falign-jumps=2 -falign-functions=2 -fno-strict-aliasing -fstrength-reduce -Werror
DBG_CFLAGS_32 := $(CFLAGS) -m32 -g -pg
DBG_CFLAGS_64 := $(CFLAGS) -g -pg

REL_LDFLAGS_32 := $(LDFLAGS) -m32
REL_LDFLAGS_64 := $(LDFLAGS)
DBG_LDFLAGS_32 := $(LDFLAGS) -m32 -g -pg
DBG_LDFLAGS_64 := $(LDFLAGS) -g -pg

REL_LDLIBS := $(LDLIBS)
DBG_LDLIBS := $(LDLIBS)

.PHONY: all all32 all64 release debug clean

all: release debug
all32: release32 debug32
all64: release64 debug64
release: release32 release64
release32: $(BIN_REL_32)
release64: $(BIN_REL_64)
debug: debug32 debug64
debug32: $(BIN_DBG_32)
debug64: $(BIN_DBG_64)

$(BIN_REL_32): $(OBJ_REL_32)
	mkdir -p $(@D)
	$(CC) $(REL_LDFLAGS_32) -o $@ $(LDLIBS) $^

$(BIN_REL_64): $(OBJ_REL_64)
	mkdir -p $(@D)
	$(CC) $(REL_LDFLAGS_64) -o $@ $(LDLIBS) $^

$(BIN_DBG_32): $(OBJ_DBG_32)
	mkdir -p $(@D)
	$(CC) $(DBG_LDFLAGS_32) -o $@ $(LDLIBS) $^

$(BIN_DBG_64): $(OBJ_DBG_64)
	mkdir -p $(@D)
	$(CC) $(DBG_LDFLAGS_64) -o $@ $(LDLIBS) $^

$(OBJ_DIR_REL_32)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(@D)
	$(CC) $(REL_CPPFLAGS) $(REL_CFLAGS_32) -c $< -o $@

$(OBJ_DIR_REL_64)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(@D)
	$(CC) $(REL_CPPFLAGS) $(REL_CFLAGS_64) -c $< -o $@

$(OBJ_DIR_DBG_32)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(@D)
	$(CC) $(DBG_CPPFLAGS) $(DBG_CFLAGS_32) -c $< -o $@

$(OBJ_DIR_DBG_64)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(@D)
	$(CC) $(DBG_CPPFLAGS) $(DBG_CFLAGS_64) -c $< -o $@

clean:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR)

-include $(OBJ_REL_32:.o=.d)
-include $(OBJ_REL_64:.o=.d)
-include $(OBJ_DBG_32:.o=.d)
-include $(OBJ_DBG_64:.o=.d)
