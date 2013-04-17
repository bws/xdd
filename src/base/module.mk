#
# Module makefile for the base module
#
DIR := src/base

BASE_SRC := $(DIR)/heartbeat.c \
	$(DIR)/io_buffers.c \
	$(DIR)/lockstep.c \
	$(DIR)/qthread.c \
	$(DIR)/qthread_cleanup.c \
	$(DIR)/qthread_init.c \
	$(DIR)/qthread_io.c \
	$(DIR)/qthread_io_for_os.c \
	$(DIR)/qthread_ttd_after_io_op.c \
	$(DIR)/qthread_ttd_before_io_op.c \
	$(DIR)/restart.c \
	$(DIR)/schedule.c \
	$(DIR)/target_cleanup.c \
	$(DIR)/target_init.c \
	$(DIR)/target_offset_table.c \
	$(DIR)/target_open.c \
	$(DIR)/target_pass.c \
	$(DIR)/target_pass_e2e_specific.c \
	$(DIR)/target_pass_qt_locator.c \
	$(DIR)/target_thread.c \
	$(DIR)/target_ttd_after_pass.c \
	$(DIR)/target_ttd_before_io_op.c \
	$(DIR)/target_ttd_before_pass.c \
	$(DIR)/verify.c
