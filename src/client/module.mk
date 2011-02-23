#
# Module makefile for the full client
#
DIR := src/client

CLIENT_SRC := $(DIR)/interactive.c \
	$(DIR)/interactive_func.c \
	$(DIR)/interactive_table.c \
	$(DIR)/parse.c \
	$(DIR)/parse_func.c \
	$(DIR)/parse_table.c \
	$(DIR)/results_display.c \
	$(DIR)/results_manager.c \
	$(DIR)/signals.c \
	$(DIR)/utils.c

CLIENT_EXE_SRC := $(DIR)/xdd.c