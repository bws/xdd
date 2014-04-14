#
# Module makefile for the full client
#
DIR := src/tools/utils

TOOLS_SRC :=

TS_EXE_SRC := $(DIR)/global_time.c $(DIR)/timeserver.c

GETTIME_EXE_SRC := $(DIR)/global_clock.c $(DIR)/global_time.c $(DIR)/gettime.c

READ_TSDUMPS_EXE_SRC := $(DIR)/read_tsdumps.c $(DIR)/matchadd_kernel_events.c

GETHOSTIP_EXE_SRC := $(DIR)/gethostip.c

GETFILESIZE_EXE_SRC := $(DIR)/getfilesize.c

TRUNCATE_EXE_SRC := $(DIR)/truncate.c
