#
# Module makefile for the full client
#
DIR := src/tools

TOOLS_SRC :=

TS_EXE_SRC := $(DIR)/timeserver.c

GETTIME_EXE_SRC := $(DIR)/gettime.c

READ_TSDUMPS_EXE_SRC := $(DIR)/read_tsdumps.c $(DIR)/matchadd_kernel_events.c
