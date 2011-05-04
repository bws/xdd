/* Copyright (C) 1992-2010 I/O Performance, Inc. and the
 * United States Departments of Energy (DoE) and Defense (DoD)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named 'Copying'; if not, write to
 * the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139.
 */
/* Principal Author:
 *      Tom Ruwart (tmruwart@ioperformance.com)
 * Contributing Authors:
 *       Steve Hodson, DoE/ORNL, (hodsonsw@ornl.gov)
 *       Steve Poole, DoE/ORNL, (spoole@ornl.gov)
 *       Brad Settlemyer, DoE/ORNL (settlemyerbw@ornl.gov)
 *       Russell Cattelan, Digital Elves (russell@thebarn.com)
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */

/* XDD function prototypes */
// access_pattern.c
void	xdd_init_seek_list(ptds_t *p);
void	xdd_save_seek_list(ptds_t *p);
int32_t	xdd_load_seek_list(ptds_t *p);

// barrier.c
int32_t	xdd_init_barrier_chain(void);
void	xdd_init_barrier_occupant(xdd_occupant_t *bop, char *name, uint32_t type, ptds_t *p);
void	xdd_destroy_all_barriers(void);
#ifdef SYSV_SEMAPHORES
int32_t	xdd_init_barrier(struct xdd_barrier *bp, int32_t threads, char *barrier_name);
void	xdd_destroy_barrier(struct xdd_barrier *bp);
int32_t	xdd_barrier(struct xdd_barrier *bp);
#else
int32_t	xdd_init_barrier(struct xdd_barrier *bp, int32_t threads, char *barrier_name);
void	xdd_destroy_barrier(struct xdd_barrier *bp);
int32_t	xdd_barrier(struct xdd_barrier *bp, xdd_occupant_t *occupantp, char owner);
#endif

// datapatterns.c
void	xdd_datapattern_buffer_init(ptds_t *p);
void	xdd_datapattern_fill(ptds_t *qp);

// debug.c
void	xdd_show_ptds(ptds_t *p);
void	xdd_show_global_data(void);

// end_to_end.c
int32_t	xdd_e2e_src_send(ptds_t *qp);
int32_t	xdd_e2e_dest_recv(ptds_t *qp);
int32_t xdd_e2e_eof_source_side(ptds_t *qp);
int32_t xdd_e2e_eof_destination_side(ptds_t *qp);

// end_to_end_init.c
int32_t	xdd_e2e_src_init(ptds_t *qp);
int32_t	xdd_e2e_setup_src_socket(ptds_t *qp);
int32_t	xdd_e2e_dest_init(ptds_t *qp);
int32_t	xdd_e2e_setup_dest_socket(ptds_t *qp);
void	xdd_e2e_set_socket_opts(ptds_t *qp, char *sktname, int skt);
void	xdd_e2e_prt_socket_opts(char *sktname, int skt);
void	xdd_e2e_gethost_error(char *str);
void	xdd_e2e_err(ptds_t *qp, char const *whence, char const *fmt, ...);
int32_t	xdd_sockets_init(void);

// global_clock.c
in_addr_t xdd_init_global_clock_network(char *hostname);
void	xdd_init_global_clock(pclk_t *pclkp);

// global_data.c
void	xdd_init_globals(char *progname);

// global_time.c
void	globtim_err(char const *fmt, ...);
void	clk_initialize(in_addr_t addr, in_port_t port, int32_t bounce, pclk_t *pclkp);
void	clk_delta(in_addr_t addr, in_port_t port, int32_t bounce, pclk_t *pclkp);

// heartbeat.c
void *xdd_heartbeat(void *junk);
void	xdd_heartbeat_legend(ptds_t *p);
void	xdd_heartbeat_values(ptds_t *p, int64_t bytes, int64_t ops, double elapsed);

// info_display.c
void	xdd_display_kmgt(FILE *out, long long int n, int block_size);
void	xdd_system_info(FILE *out);
void	xdd_options_info(FILE *out);
void	xdd_target_info(FILE *out, ptds_t *p);
void	xdd_memory_usage_info(FILE *out);
void	xdd_config_info(void);

// initialization.c
int32_t	xdd_initialization(int32_t argc,char *argv[]);

// interactive.c
void 	*xdd_interactive(void *debugger);
int 	xdd_interactive_parse_command(int tokens, char *cmd);
void	xdd_interactive_usage(int32_t fullhelp);

// interactive_func.c
int 	xdd_interactive_exit(int32_t tokens, char *cmdline, uint32_t flags);
int 	xdd_interactive_goto(int32_t tokens, char *cmdline, uint32_t flags);
int 	xdd_interactive_help(int32_t tokens, char *cmdline, uint32_t flags);
int 	xdd_interactive_run(int32_t tokens, char *cmdline, uint32_t flags);
int 	xdd_interactive_show(int32_t tokens, char *cmdline, uint32_t flags);
int 	xdd_interactive_step(int32_t tokens, char *cmdline, uint32_t flags);
int 	xdd_interactive_stop(int32_t tokens, char *cmdline, uint32_t flags);
int		xdd_interactive_ts_report(int32_t tokens, char *cmdline, uint32_t flags);
void	xdd_interactive_show_rwbuf(int32_t tokens, char *cmdline, uint32_t flags);
void	xdd_interactive_show_global_data(int32_t tokens, char *cmdline, uint32_t flags);
void	xdd_interactive_show_ptds(int32_t tokens, char *cmdline, uint32_t flags);
void	xdd_interactive_show_qtsem(int32_t tokens, char *cmdline, uint32_t flags);
void	xdd_interactive_show_qtstate(int32_t tokens, char *cmdline, uint32_t flags);
void	xdd_interactive_display_state_info(ptds_t *qp);
void	xdd_interactive_show_qtptds(int32_t tokens, char *cmdline, uint32_t flags);
void	xdd_interactive_show_tot(int32_t tokens, char *cmdline, uint32_t flags);
void	xdd_interactive_show_print_tot(int32_t tokens, char *cmdline, uint32_t flags);
void	xdd_interactive_show_tot_display_fields(ptds_t *p, FILE *fp);
void	xdd_interactive_show_trace(int32_t tokens, char *cmdline, uint32_t flags);
void	xdd_interactive_show_barrier(int32_t tokens, char *cmdline, uint32_t flags);

// io_buffers.c
unsigned char *xdd_init_io_buffers(ptds_t *p);

// lockstep.c
int32_t	xdd_lockstep_init(ptds_t *p);
int32_t	xdd_lockstep_before_io_op(ptds_t *p);
void	xdd_lockstep_before_pass(ptds_t *p);
int32_t	xdd_lockstep_after_pass(ptds_t *p);

// memory.c
void	xdd_lock_memory(unsigned char *bp, uint32_t bsize, char *sp);
void	xdd_unlock_memory(unsigned char *bp, uint32_t bsize, char *sp);

// parse.c
void		xdd_parse_args(int32_t argc, char *argv[], uint32_t flags);
void		xdd_parse(int32_t argc, char *argv[]);
void		xdd_usage(int32_t fullhelp);
int 		xdd_check_option(char *op);
int32_t		xdd_process_paramfile(char *fnp);
int 		xdd_parse_target_number(int32_t argc, char *argv[], uint32_t flags, int *target_number);
ptds_t 		*xdd_get_ptdsp(int32_t target_number, char *op);
restart_t 	*xdd_get_restartp(ptds_t *p);
int32_t		xdd_linux_cpu_count(void);
int32_t		xdd_cpu_count(void);
int32_t		xdd_atohex(unsigned char *destp, char *sourcep);

// parse_func.c
int32_t	xdd_parse_arg_count_check(int32_t args, int32_t argc, char *option);

// pclk.c
void	pclk_initialize(pclk_t *pclkp);
void	pclk_shutdown(void);
void	pclk_now(pclk_t *pclkp);

// preallocate.c
int32_t	xdd_target_preallocate_for_os(ptds_t *p);
int32_t	xdd_target_preallocate_for_os(ptds_t *p);
int32_t	xdd_target_preallocate_for_os(ptds_t *p);
int32_t	xdd_target_preallocate(ptds_t *p);

// processor.c
void	xdd_processor(ptds_t *p);
int		xdd_get_processor(void);

// ptds.c
void	xdd_init_new_ptds(ptds_t *p, int32_t n);
void	xdd_calculate_xfer_info(ptds_t *tp);
ptds_t 	*xdd_create_qthread_ptds(ptds_t *tp, int32_t q);
void	xdd_build_ptds_substructure(void);

// qthread.c
void 	*xdd_qthread(void *pin);

// qthread_cleanup.c
void	xdd_qthread_cleanup(ptds_t *qp);

// qthread_init.c
int32_t	xdd_qthread_init(ptds_t *qp);

// qthread_io.c
void	xdd_qthread_io(ptds_t *qp);
int32_t	xdd_qthread_wait_for_previous_io(ptds_t *qp);
int32_t	xdd_qthread_release_next_io(ptds_t *qp);
void	xdd_qthread_update_local_counters(ptds_t *qp);
void	xdd_qthread_update_target_counters(ptds_t *qp);
void	xdd_qthread_check_io_status(ptds_t *qp);

// qthread_io_for_os.c
void	xdd_io_for_os(ptds_t *qp);
void	xdd_io_for_os(ptds_t *qp);
void	xdd_io_for_os(ptds_t *p);

// qthread_ttd_after_io_op.c
void	xdd_threshold_after_io_op(ptds_t *qp);
void	xdd_status_after_io_op(ptds_t *qp);
void	xdd_dio_after_io_op(ptds_t *qp);
void	xdd_raw_after_io_op(ptds_t *qp);
void	xdd_e2e_after_io_op(ptds_t *qp);
void	xdd_extended_stats(ptds_t *qp);
void	xdd_qthread_ttd_after_io_op(ptds_t *qp);

// qthread_ttd_before_io_op.c
void	xdd_dio_before_io_op(ptds_t *qp);
void	xdd_raw_before_io_op(ptds_t *qp);
int32_t	xdd_e2e_before_io_op(ptds_t *qp);
void	xdd_throttle_before_io_op(ptds_t *qp);
int32_t	xdd_qthread_ttd_before_io_op(ptds_t *qp);

// read_after_write.c
void	xdd_raw_err(char const *fmt, ...);
int32_t	xdd_raw_setup_reader_socket(ptds_t *p);
int32_t	xdd_raw_sockets_init(void);
int32_t	xdd_raw_reader_init(ptds_t *p);
int32_t	xdd_raw_read_wait(ptds_t *p);
int32_t	xdd_raw_setup_writer_socket(ptds_t *p);
int32_t	xdd_raw_writer_init(ptds_t *p);
int32_t	xdd_raw_writer_send_msg(ptds_t *p);

// restart.c
int		xdd_restart_create_restart_file(restart_t *rp);
int 	xdd_restart_write_restart_file(restart_t *rp);
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
void	xdd_results_format_id_add( char *sp );

// results_manager.c
void 	*xdd_results_manager(void *n);
int32_t	xdd_results_manager_init(void);
void 	*xdd_results_header_display(results_t *tmprp);
void 	*xdd_process_pass_results(void);
void 	*xdd_process_run_results(void);
void	xdd_combine_results(results_t *to, results_t *from);
void 	*xdd_extract_pass_results(results_t *rp, ptds_t *p);
void 	*xdd_results_dump(results_t *rp, char *dumptype);

// schedule.c
void	xdd_schedule_options(void);

// sg.c
int32_t	xdd_sg_io(ptds_t *p, char rw);
int32_t	xdd_sg_read_capacity(ptds_t *p);
void	xdd_sg_set_reserved_size(ptds_t *p, int fd);
void	xdd_sg_get_version(ptds_t *p, int fd);

// signals.c
void	xdd_signal_handler(int signum, siginfo_t *sip, void *ucp);
int32_t	xdd_signal_init(void);
void	xdd_signal_start_debugger();

// target_cleanup.c
void	xdd_target_thread_cleanup(ptds_t *p);

// target_init.c
int32_t	xdd_target_init(ptds_t *p);
int32_t	xdd_target_init_barriers(ptds_t *p);
int32_t	xdd_target_init_start_qthreads(ptds_t *p);

// target_open.c
int32_t	xdd_target_open(ptds_t *p);
void	xdd_target_reopen(ptds_t *p);
int32_t	xdd_target_shallow_open(ptds_t *p);
void	xdd_target_name(ptds_t *p);
int32_t	xdd_target_existence_check(ptds_t *p);
int32_t	xdd_target_open_for_os(ptds_t *p);

// target_pass.c
int32_t	xdd_targetpass(ptds_t *p);
void	xdd_targetpass_loop(ptds_t *p);
void	xdd_targetpass_e2e_monitor(ptds_t *p);
void	xdd_targetpass_task_setup(ptds_t *qp);
void 	xdd_targetpass_end_of_pass(ptds_t *p);
int32_t xdd_targetpass_count_active_qthreads(ptds_t *p);

// target_pass_e2e_specific.c
void	xdd_targetpass_e2e_loop_dst(ptds_t *p);
void	xdd_targetpass_e2e_loop_src(ptds_t *p);
void	xdd_targetpass_e2e_task_setup_src(ptds_t *qp);
void	xdd_targetpass_e2e_eof_src(ptds_t *p);
void	xdd_targetpass_e2e_monitor(ptds_t *p);

// target_pass_qt_locator.c
ptds_t	*xdd_get_specific_qthread(ptds_t *p, int32_t q);
ptds_t	*xdd_get_any_available_qthread(ptds_t *p);

// target_thread.c
void 	*xdd_target_thread(void *pin);

// target_ttd_after_pass.c
int32_t	xdd_target_ttd_after_pass(ptds_t *p);

// target_ttd_before_io_op.c
void	xdd_syncio_before_io_op(ptds_t *p);
int32_t	xdd_start_trigger_before_io_op(ptds_t *p);
int32_t	xdd_timelimit_before_io_op(ptds_t *p);
int32_t	xdd_runtime_before_io_op(ptds_t *p);
int32_t	xdd_target_ttd_before_io_op(ptds_t *p);

// target_ttd_before_pass.c
void	xdd_timer_calibration_before_pass(void);
void	xdd_start_delay_before_pass(ptds_t *p);
void	xdd_raw_before_pass(ptds_t *p);
void	xdd_e2e_before_pass(ptds_t *p);
void	xdd_init_ptds_before_pass(ptds_t *p);
int32_t	xdd_target_ttd_before_pass(ptds_t *p);

// timestamp.c
void	xdd_ts_overhead(struct tthdr *ttp); 
void	xdd_ts_setup(ptds_t *p);
void	xdd_ts_write(ptds_t *p);
void	xdd_ts_cleanup(struct tthdr *ttp);
void	xdd_ts_reports(ptds_t *p);

// utils.c
char 	*xdd_getnexttoken(char *tp);
int 	xdd_tokenize(char *cp);
int 	xdd_random_int(void);
double 	xdd_random_float(void);

// verify.c
int32_t	xdd_verify_checksum(ptds_t *p, int64_t current_op);
int32_t	xdd_verify_hex(ptds_t *p, int64_t current_op);
int32_t	xdd_verify_sequence(ptds_t *p, int64_t current_op);
int32_t	xdd_verify_singlechar(ptds_t *p, int64_t current_op);
int32_t	xdd_verify_contents(ptds_t *p, int64_t current_op);
int32_t	xdd_verify_location(ptds_t *p, int64_t current_op);
int32_t	xdd_verify(ptds_t *p, int64_t current_op);

// xdd.c
int32_t	xdd_start_targets();
void	xdd_start_results_manager();
void	xdd_start_heartbeat();
void	xdd_start_restart_monitor();
void	xdd_start_interactive();
