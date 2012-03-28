#
# Module makefile for the networking module
#
DIR := src/net

NET_SRC := $(DIR)/end_to_end.c \
	$(DIR)/end_to_end_init.c \
	$(DIR)/read_after_write.c \
	$(DIR)/net_utils.c
