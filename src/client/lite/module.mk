#
# Module makefile for the lite client
#
DIR := src/client/lite

CLIENT_LITE_SRC := $(DIR)/xdd-lite-client.c \
	$(DIR)/xdd-lite-forking-server.c \
	$(DIR)/xdd-lite-options.c \
	$(DIR)/xdd-lite-server.c 

XDD_LITE_EXE_SRC := $(DIR)/xdd-lite-main.c 
