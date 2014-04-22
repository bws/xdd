/*
 * XDD - a data movement and benchmarking toolkit
 *
 * Copyright (C) 1992-2013 I/O Performance, Inc.
 * Copyright (C) 2009-2013 UT-Battelle, LLC
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */
#ifndef XINT_PROTOTYPES_H
#define XINT_PROTOTYPES_H

#include "xint_restart.h"
#include "xint_plan.h"
#include "xint_global_data.h"

/* XDD function prototypes */
// access_pattern.c
void	xdd_init_seek_list(target_data_t *p);
void	xdd_save_seek_list(target_data_t *p);
int32_t	xdd_load_seek_list(target_data_t *p);

// barrier.c
int32_t	xdd_init_barrier_chain(xdd_plan_t* planp);
void	xdd_init_barrier_occupant(xdd_occupant_t *bop, char *name, uint32_t type, void *datap);
void	xdd_destroy_all_barriers(xdd_plan_t* planp);
int32_t	xdd_init_barrier(xdd_plan_t* planp, struct xdd_barrier *bp, int32_t threads, char *barrier_name);
void	xdd_destroy_barrier(xdd_plan_t* planp, struct xdd_barrier *bp);
int32_t	xdd_barrier(struct xdd_barrier *bp, xdd_occupant_t *occupantp, char owner);

// datapatterns.c
void	xdd_datapattern_buffer_init(worker_data_t *wdp);
void	xdd_datapattern_fill(worker_data_t *wdp);

// debug.c
void	xdd_show_plan(xdd_plan_t *planp);
void	xdd_show_target_data(target_data_t *tdp);
void	xdd_show_global_data(void);
void	xdd_show_worker_data(worker_data_t *wdp);
void	xdd_show_task(xint_task_t *taskp);
void	xdd_show_occupant(xdd_occupant_t *op);
void	xdd_show_target_counters(xint_target_counters_t *tcp);
void 	xdd_show_e2e(xint_e2e_t *e2ep);
void 	xdd_show_e2e_header(xdd_e2e_header_t *e2ehp);
void 	xdd_show_tot(tot_t *totp);
void 	xdd_show_tot_entry(tot_t *totp, int i);
void 	xdd_show_ts_table(xint_timestamp_t *ts_tablep, int target_number);
void 	xdd_show_ts_header(xdd_ts_header_t *ts_hdrp, int target_number);
void    xdd_show_results_data(results_t *rp, char *dumptype, xdd_plan_t *planp);

// end_to_end.c
int32_t	xdd_e2e_src_send(worker_data_t *wdp);
int32_t	xdd_e2e_dest_receive(worker_data_t *wdp);
int32_t	xdd_e2e_dest_connection(worker_data_t *wdp);
int32_t	xdd_e2e_dest_receive_header(worker_data_t *wdp);
int32_t	xdd_e2e_dest_receive_data(worker_data_t *wdp);
int32_t	xdd_e2e_dest_receive_error(worker_data_t *wdp);
int32_t xdd_e2e_eof_source_side(worker_data_t *wdp);
int32_t xdd_e2e_eof_destination_side(worker_data_t *wdp);

// end_to_end_init.c
int32_t	xdd_e2e_target_init(target_data_t *tdp);
int32_t	xdd_e2e_worker_init(worker_data_t *wdp);
int32_t	xdd_e2e_src_init(worker_data_t *wdp);
int32_t	xdd_e2e_setup_src_socket(worker_data_t *wdp);
int32_t	xdd_e2e_dest_init(worker_data_t *wdp);
int32_t	xdd_e2e_setup_dest_socket(worker_data_t *wdp);
void	xdd_e2e_set_socket_opts(worker_data_t *wdp, int skt);
void	xdd_e2e_prt_socket_opts(int skt);
void	xdd_e2e_err(worker_data_t *wdp, char const *whence, char const *fmt, ...);
int32_t	xdd_sockets_init(void);

// global_clock.c
in_addr_t xdd_init_global_clock_network(char *hostname);
void	xdd_init_global_clock(nclk_t *nclkp);

// xint_global_data.c
xdd_global_data_t* xint_global_data_initialization(char *progname);

// global_time.c
void	globtim_err(char const *fmt, ...);
void	clk_initialize(in_addr_t addr, in_port_t port, int32_t bounce, nclk_t *nclkp);
void	clk_delta(in_addr_t addr, in_port_t port, int32_t bounce, nclk_t *nclkp);

// heartbeat.c
void *xdd_heartbeat(void *data);
void	xdd_heartbeat_legend(xdd_plan_t* planp, target_data_t *p);
void	xdd_heartbeat_values(target_data_t *p, int64_t bytes, int64_t ops, double elapsed);

// info_display.c
void	xdd_display_kmgt(FILE *out, long long int n, int block_size);
void	xdd_system_info(xdd_plan_t* planp, FILE *out);
void	xdd_options_info(xdd_plan_t* planp, FILE *out);
void	xdd_target_info(FILE *out, target_data_t *p);
void	xdd_memory_usage_info(xdd_plan_t* planp, FILE *out);
void	xdd_config_info(xdd_plan_t* planp);

// initialization.c
int32_t	xdd_initialization(int32_t argc,char *argv[], xdd_plan_t* planp);

// interactive.c
void 	*xdd_interactive(void *debugger);
int 	xdd_interactive_parse_command(int tokens, char *cmd, xdd_plan_t *planp);
void	xdd_interactive_usage(int32_t fullhelp);

// interactive_func.c
int 	xdd_interactive_exit(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp);
int 	xdd_interactive_help(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp);
int 	xdd_interactive_goto(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp);
int 	xdd_interactive_help(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp);
int 	xdd_interactive_run(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp);
int 	xdd_interactive_show(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp);
int 	xdd_interactive_step(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp);
int 	xdd_interactive_stop(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp);
int		xdd_interactive_ts_report(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp);
void	xdd_interactive_show_results(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp);
void	xdd_interactive_show_rwbuf(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp);
void	xdd_interactive_show_global_data(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp);
void	xdd_interactive_show_target_data(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp);
void	xdd_interactive_show_worker_state(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp);
void	xdd_interactive_display_state_info(worker_data_t *wdp);
void	xdd_interactive_show_worker_data(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp);
void	xdd_interactive_show_tot(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp);
void	xdd_interactive_show_print_tot(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp);
void	xdd_interactive_show_tot_display_fields(target_data_t *tdp, FILE *fp);
void	xdd_interactive_show_trace(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp);
void	xdd_interactive_show_barrier(int32_t tokens, char *cmdline, uint32_t flags, xdd_plan_t *planp);

// io_buffers.c
unsigned char *xdd_init_io_buffers(worker_data_t *wdp);

// lockstep.c
int32_t	xdd_lockstep(target_data_t *p);
int32_t	xdd_lockstep_init(target_data_t *p);
void	xdd_lockstep_before_pass(target_data_t *p);
int32_t	xdd_lockstep_after_pass(target_data_t *p);
int32_t xdd_lockstep_check_triggers(worker_data_t *wdp, lockstep_t *lsp);

// memory.c
void	xdd_lock_memory(unsigned char *bp, uint32_t bsize, char *sp);
void	xdd_unlock_memory(unsigned char *bp, uint32_t bsize, char *sp);

// net_utils.c
int32_t	xint_lookup_addr(const char *name, uint32_t flags, in_addr_t *result);

// parse.c
void					xdd_parse_args(xdd_plan_t* planp, int32_t argc, char *argv[], uint32_t flags);
void					xdd_parse(xdd_plan_t* planp, int32_t argc, char *argv[]);
void					xdd_usage(int32_t fullhelp);
int 					xdd_check_option(char *op);
int32_t					xdd_process_paramfile(xdd_plan_t* planp, char *fnp);
int 					xdd_parse_target_number(xdd_plan_t* planp, int32_t argc, char *argv[], uint32_t flags, int *target_number);
target_data_t 		*xdd_get_target_datap(xdd_plan_t* planp, int32_t target_number, char *op);
xint_restart_t 			*xdd_get_restartp(target_data_t *tdp);
xint_raw_t				*xdd_get_rawp(target_data_t *tdp);
xint_e2e_t 				*xdd_get_e2ep(void);
xint_throttle_t 		*xdd_get_throtp(target_data_t *tdp);
xint_triggers_t 		*xdd_get_trigp(target_data_t *tdp);
xint_extended_stats_t 	*xdd_get_esp(target_data_t *tdp);
int32_t					xdd_linux_cpu_count(void);
int32_t					xdd_cpu_count(void);
int32_t					xdd_atohex(unsigned char *destp, char *sourcep);

// parse_func.c
int32_t	xdd_parse_arg_count_check(int32_t args, int32_t argc, char *option);

// nclk.c
void	nclk_initialize(nclk_t *nclkp);
void	nclk_shutdown(void);
void	nclk_now(nclk_t *nclkp);
int64_t	pclk_now(void);

// xint_preallocate.c
int32_t	xint_target_preallocate_for_os(target_data_t *p);
int32_t	xint_target_preallocate_for_os(target_data_t *p);
int32_t	xint_target_preallocate_for_os(target_data_t *p);
int32_t	xint_target_preallocate(target_data_t *p);

// xint_pretruncate.c
int32_t	xint_target_pretruncate(target_data_t *p);

// processor.c
void	xdd_processor(target_data_t *p);
int		xdd_get_processor(void);

// target_data.c
void	xdd_init_new_target_data(target_data_t *tdp, int32_t n);
void	xdd_calculate_xfer_info(target_data_t *tdp);
worker_data_t 	*xdd_create_worker_data(target_data_t *tdp, int32_t q);
void	xdd_build_target_data_substructure(xdd_plan_t* planp);
void	xdd_build_target_data_substructure_e2e(xdd_plan_t* planp, target_data_t *tdp);

// worker_thread.c
void 	*xdd_worker_thread(void *pin);

// worker_thread_cleanup.c
void	xdd_worker_thread_cleanup(worker_data_t *wdp);

// worker_thread_init.c
int32_t	xdd_worker_thread_init(worker_data_t *wdp);

// worker_thread_io.c
void	xdd_worker_thread_io(worker_data_t *wdp);
int32_t	xdd_worker_thread_wait_for_previous_io(worker_data_t *wdp);
int32_t	xdd_worker_thread_release_next_io(worker_data_t *wdp);
void	xdd_worker_thread_update_local_counters(worker_data_t *wdp);
void	xdd_worker_thread_update_target_counters(worker_data_t *wdp);
void	xdd_worker_thread_check_io_status(worker_data_t *wdp);

// worker_thread_io_for_os.c
void	xdd_io_for_os(worker_data_t *wdp);

// worker_thread_ttd_after_io_op.c
void	xdd_threshold_after_io_op(worker_data_t *wdp);
void	xdd_status_after_io_op(worker_data_t *wdp);
void	xdd_dio_after_io_op(worker_data_t *wdp);
void	xdd_raw_after_io_op(worker_data_t *wdp);
void	xdd_e2e_after_io_op(worker_data_t *wdp);
void	xdd_extended_stats(worker_data_t *wdp);
void	xdd_worker_thread_ttd_after_io_op(worker_data_t *wdp);

// worker_thread_ttd_before_io_op.c
void	xdd_dio_before_io_op(worker_data_t *wdp);
void	xdd_raw_before_io_op(worker_data_t *wdp);
int32_t	xdd_e2e_before_io_op(worker_data_t *wdp);
void	xdd_throttle_before_io_op(worker_data_t *wdp);
int32_t	xdd_worker_thread_ttd_before_io_op(worker_data_t *wdp);

// read_after_write.c
void	xdd_raw_err(char const *fmt, ...);
int32_t	xdd_raw_setup_reader_socket(target_data_t *tdp);
int32_t	xdd_raw_sockets_init(target_data_t *tdp);
int32_t	xdd_raw_reader_init(target_data_t *tdp);
int32_t	xdd_raw_read_wait(worker_data_t *wdp);
int32_t	xdd_raw_setup_writer_socket(target_data_t *tdp);
int32_t	xdd_raw_writer_init(target_data_t *tdp);
int32_t	xdd_raw_writer_send_msg(worker_data_t *wdp);

// restart.c
int	xdd_restart_create_restart_file(xint_restart_t *rp);
int	xdd_restart_write_restart_file(xint_restart_t *rp);
void 	*xdd_restart_monitor(void *junk);

// results_display.c
void	xdd_results_fmt_what(results_t *rp);
void	xdd_results_fmt_pass_number(results_t *rp);
void	xdd_results_fmt_target_number(results_t *rp);
void	xdd_results_fmt_queue_number(results_t *rp);
void	xdd_results_fmt_bytes_transferred(results_t *rp);
void	xdd_results_fmt_bytes_read(results_t *rp);
void	xdd_results_fmt_bytes_written(results_t *rp);
void	xdd_results_fmt_ops(results_t *rp);
void	xdd_results_fmt_read_ops(results_t *rp);
void	xdd_results_fmt_write_ops(results_t *rp);
void	xdd_results_fmt_bandwidth(results_t *rp);
void	xdd_results_fmt_read_bandwidth(results_t *rp);
void	xdd_results_fmt_write_bandwidth(results_t *rp);
void	xdd_results_fmt_iops(results_t *rp);
void	xdd_results_fmt_read_iops(results_t *rp);
void	xdd_results_fmt_write_iops(results_t *rp);
void	xdd_results_fmt_latency(results_t *rp);
void	xdd_results_fmt_elapsed_time_from_1st_op(results_t *rp);
void	xdd_results_fmt_elapsed_time_from_pass_start(results_t *rp);
void	xdd_results_fmt_elapsed_over_head_time(results_t *rp);
void	xdd_results_fmt_elapsed_pattern_fill_time(results_t *rp);
void	xdd_results_fmt_elapsed_buffer_flush_time(results_t *rp);
void	xdd_results_fmt_cpu_time(results_t *rp);
void	xdd_results_fmt_percent_cpu_time(results_t *rp);
void	xdd_results_fmt_user_time(results_t *rp);
void	xdd_results_fmt_system_time(results_t *rp);
void	xdd_results_fmt_percent_user_time(results_t *rp);
void	xdd_results_fmt_percent_system_time(results_t *rp);
void	xdd_results_fmt_op_type(results_t *rp);
void	xdd_results_fmt_transfer_size_bytes(results_t *rp);
void	xdd_results_fmt_transfer_size_blocks(results_t *rp);
void	xdd_results_fmt_transfer_size_kbytes(results_t *rp);
void	xdd_results_fmt_transfer_size_mbytes(results_t *rp);
void	xdd_results_fmt_e2e_io_time(results_t *rp);
void	xdd_results_fmt_e2e_send_receive_time(results_t *rp);
void	xdd_results_fmt_e2e_percent_send_receive_time(results_t *rp);
void	xdd_results_fmt_e2e_lag_time(results_t *rp);
void	xdd_results_fmt_e2e_percent_lag_time(results_t *rp);
void	xdd_results_fmt_e2e_first_read_time(results_t *rp);
void	xdd_results_fmt_e2e_last_write_time(results_t *rp);
void	xdd_results_fmt_delimiter(results_t *rp);
void 	*xdd_results_display(results_t *rp);
void	xdd_results_format_id_add( char *sp, char *format_stringp  );

// results_manager.c
void    *xdd_results_manager(void *data);
int32_t	xdd_results_manager_init(xdd_plan_t *planp);
void    *xdd_results_header_display(results_t *tmprp, xdd_plan_t *planp);
void    *xdd_process_pass_results(xdd_plan_t *planp);
void    *xdd_process_run_results(xdd_plan_t *planp);
void    xdd_combine_results(results_t *to, results_t *from, xdd_plan_t *planp);
void    *xdd_extract_pass_results(results_t *rp, target_data_t *p, xdd_plan_t *planp);

// schedule.c
void	xdd_schedule_options(void);

// sg.c
xdd_sgio_t *xdd_get_sgiop(worker_data_t *wdp);
int32_t	xdd_sg_io(worker_data_t *wdp, char rw);
int32_t	xdd_sg_read_capacity(worker_data_t *wdp);
void	xdd_sg_set_reserved_size(target_data_t *tdp, int fd);
void	xdd_sg_get_version(target_data_t *tdp, int fd);

// signals.c
void	xdd_signal_handler(int signum, siginfo_t *sip, void *ucp);
int32_t	xdd_signal_init(xdd_plan_t *planp);
void	xdd_signal_start_debugger();

// target_cleanup.c
void	xdd_target_thread_cleanup(target_data_t *p);

// target_init.c
int32_t	xint_target_init(target_data_t *p);
int32_t	xint_target_init_barriers(target_data_t *p);
int32_t	xint_target_init_start_worker_threads(target_data_t *p);

// target_open.c
int32_t	xdd_target_open(target_data_t *p);
void	xdd_target_reopen(target_data_t *p);
int32_t	xdd_target_shallow_open(worker_data_t *wdp);
void	xdd_target_name(target_data_t *p);
int32_t	xdd_target_existence_check(target_data_t *p);
int32_t	xdd_target_open_for_os(target_data_t *p);

// target_pass.c
int32_t	xdd_target_pass(xdd_plan_t* planp, target_data_t *tdp);
void	xdd_target_pass_loop(xdd_plan_t* planp, target_data_t *tdp);
void	xdd_target_pass_e2e_monitor(target_data_t *tdp);
void	xdd_target_pass_task_setup(worker_data_t *wdp);
void 	xdd_target_pass_end_of_pass(target_data_t *tdp);
int32_t xdd_target_pass_count_active_worker_threads(target_data_t *tdp);

// target_pass_e2e_specific.c
void	xdd_targetpass_e2e_loop_dst(xdd_plan_t* planp, target_data_t *tdp);
void	xdd_targetpass_e2e_loop_src(xdd_plan_t* planp, target_data_t *tdp);
void	xdd_targetpass_e2e_task_setup_src(worker_data_t *wdp);
void	xdd_targetpass_e2e_eof_src(target_data_t *tdp);
void	xdd_targetpass_e2e_monitor(target_data_t *tdp);

// target_pass_qt_locator.c
worker_data_t	*xdd_get_specific_worker_thread(target_data_t *tdp, int32_t q);
worker_data_t	*xdd_get_any_available_worker_thread(target_data_t *tdp);

// target_thread.c
void 	*xdd_target_thread(void *pin);

// target_ttd_after_pass.c
int32_t	xdd_target_ttd_after_pass(target_data_t *p);

// target_ttd_before_io_op.c
void	xdd_syncio_before_io_op(target_data_t *p);
int32_t	xdd_start_trigger_before_io_op(target_data_t *p);
int32_t	xdd_timelimit_before_io_op(target_data_t *p);
int32_t	xdd_runtime_before_io_op(target_data_t *p);
int32_t	xdd_target_ttd_before_io_op(target_data_t *tdp, worker_data_t *wdp);
int32_t	xdd_target_ttd_after_io_op(target_data_t *tdp, worker_data_t *wdp);

// target_ttd_before_pass.c
void	xdd_timer_calibration_before_pass(void);
void	xdd_start_delay_before_pass(target_data_t *tdp);
void	xdd_raw_before_pass(target_data_t *tdp);
void	xdd_e2e_before_pass(target_data_t *tdp);
void	xdd_init_target_data_before_pass(target_data_t *tdp);
void	xdd_init_worker_data_before_pass(worker_data_t *wdp);
int32_t	xdd_target_ttd_before_pass(target_data_t *tdp);

// timestamp.c
void	xdd_ts_overhead(struct xdd_ts_header *ts_hdrp); 
void	xdd_ts_setup(target_data_t *p);
void	xdd_ts_write(target_data_t *p);
void	xdd_ts_cleanup(struct xdd_ts_header *ts_hdrp);
void	xdd_ts_reports(target_data_t *p);

// utils.c
char 	*xdd_getnexttoken(char *tp);
int 	xdd_tokenize(char *cp);
int	xdd_random_int(void);
double 	xdd_random_float(void);

// verify.c
int32_t	xdd_verify_checksum(worker_data_t *wdp, int64_t current_op);
int32_t	xdd_verify_hex(worker_data_t *wdp, int64_t current_op);
int32_t	xdd_verify_sequence(worker_data_t *wdp, int64_t current_op);
int32_t	xdd_verify_singlechar(worker_data_t *wdp, int64_t current_op);
int32_t	xdd_verify_contents(worker_data_t *wdp, int64_t current_op);
int32_t	xdd_verify_location(worker_data_t *wdp, int64_t current_op);
int32_t	xdd_verify(worker_data_t *wdp, int64_t current_op);

// xdd.c
int32_t	xdd_start_targets(xdd_plan_t *planp);
void	xdd_start_results_manager(xdd_plan_t *planp);
void	xdd_start_heartbeat(xdd_plan_t *planp);
void	xdd_start_restart_monitor(xdd_plan_t *planp);
void	xdd_start_interactive(xdd_plan_t *planp);

// xnet_end_to_end_init.c
int32_t xint_e2e_xni_init(target_data_t *tdp);

// xnet_end_to_end.c
int32_t xint_e2e_dest_connect(target_data_t *tdp);
int32_t xint_e2e_src_connect(target_data_t *tdp);
int32_t xint_e2e_xni_send(worker_data_t *wdp);
int32_t xint_e2e_xni_recv(worker_data_t *wdp);
#endif
/*
 * Local variables:
 *  indent-tabs-mode: t
 *  default-tab-width: 4
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
