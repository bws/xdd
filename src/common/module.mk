#
# Module makefile for the full client
#
DIR := src/common

COMMON_SRC := $(DIR)/access_pattern.c \
	$(DIR)/barrier.c \
	$(DIR)/datapatterns.c \
	$(DIR)/debug.c \
	$(DIR)/memory.c \
	$(DIR)/processor.c \
	$(DIR)/target_data.c \
	$(DIR)/timestamp.c \
	$(DIR)/xint_global_data.c \
	$(DIR)/xint_nclk.c
