#
# Module makefile for the lite client
#
DIR := src/bx

BX_SRC := $(DIR)/bx_buffer_queue.c \
	$(DIR)/bx_target_thread.c \
	$(DIR)/bx_user_interface.c \
	$(DIR)/bx_worker_queue.c \
	$(DIR)/bx_worker_thread.c 

BXT_EXE_SRC := $(DIR)/bx_test.c 
