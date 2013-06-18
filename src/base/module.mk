#
# Module makefile for the base module
#
DIR := src/base

BASE_SRC := $(DIR)/heartbeat.c \
	$(DIR)/io_buffers.c \
	$(DIR)/lockstep.c \
	$(DIR)/restart.c \
	$(DIR)/schedule.c \
	$(DIR)/target_cleanup.c \
	$(DIR)/target_init.c \
	$(DIR)/target_offset_table.c \
	$(DIR)/target_open.c \
	$(DIR)/target_pass.c \
	$(DIR)/target_pass_e2e_specific.c \
	$(DIR)/target_pass_wt_locator.c \
	$(DIR)/target_thread.c \
	$(DIR)/target_ttd_after_pass.c \
	$(DIR)/target_ttd_before_io_op.c \
	$(DIR)/target_ttd_before_pass.c \
	$(DIR)/verify.c \
	$(DIR)/worker_thread.c \
	$(DIR)/worker_thread_cleanup.c \
	$(DIR)/worker_thread_init.c \
	$(DIR)/worker_thread_io.c \
	$(DIR)/worker_thread_io_for_os.c \
	$(DIR)/worker_thread_ttd_after_io_op.c \
	$(DIR)/worker_thread_ttd_before_io_op.c \
	$(DIR)/xint_plan.c
