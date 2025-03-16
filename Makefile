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

OBJ_DIR_REL := $(OBJ_DIR)/release
OBJ_DIR_DBG := $(OBJ_DIR)/debug

BIN_REL := $(BIN_DIR)/qmm2.so
BIN_DBG := $(BIN_DIR)/qmm2_d.so

OBJ_REL := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR_REL)/%.o)
OBJ_DBG := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR_DBG)/%.o)

CPPFLAGS := -MMD -MP -I ./include -isystem ../qmm_sdks
CFLAGS   := -Wall -pipe -m32 -fPIC
LDFLAGS  := -m32 -shared -fPIC
LDLIBS   :=

REL_CPPFLAGS := $(CPPFLAGS)
DBG_CPPFLAGS := $(CPPFLAGS)

REL_CFLAGS := $(CFLAGS) -O2 -ffast-math -falign-loops=2 -falign-jumps=2 -falign-functions=2 -fno-strict-aliasing -fstrength-reduce 
DBG_CFLAGS := $(CFLAGS) -g -pg 

REL_LDFLAGS := $(LDFLAGS)
DBG_LDFLAGS := $(LDFLAGS) -g -pg

REL_LDLIBS := $(LDLIBS)
DBG_LDLIBS := $(LDLIBS)

.PHONY: all clean

debug: $(BIN_DBG)

release: $(BIN_REL)

all: release debug

$(BIN_REL): $(OBJ_REL) | $(BIN_DIR)
	$(CC) $(REL_LDFLAGS) -o $@ $(LDLIBS) $^

$(BIN_DBG): $(OBJ_DBG) | $(BIN_DIR)
	$(CC) $(DBG_LDFLAGS) -o $@ $(LDLIBS) $^

$(BIN_DIR) $(OBJ_DIR) $(OBJ_DIR_REL) $(OBJ_DIR_DBG):
	mkdir -p $@

$(OBJ_DIR_REL)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR_REL)
	$(CC) $(REL_CPPFLAGS) $(REL_CFLAGS) -c $< -o $@

$(OBJ_DIR_DBG)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR_DBG)
	$(CC) $(DBG_CPPFLAGS) $(DBG_CFLAGS) -c $< -o $@

clean:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR)

-include $(OBJ:.o=.d)
