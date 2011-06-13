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

#define XDD_PARSE_PHASE1			0x00000001 // Preprocess phase - phase 1 
#define XDD_PARSE_PHASE2			0x00000002 // Parse Phase 2 occurs after we know how many targets there really are
#define XDD_PARSE_TARGET_IN			0x00000004 // Target In
#define XDD_PARSE_TARGET_OUT		0x00000008 // Target Out
#define XDD_PARSE_TARGET_FILE		0x00000010 // Target_inout I/O Type of "FILE"
#define XDD_PARSE_TARGET_NETWORK	0x00000020 // Target_inout I/O Type of "NETWORK"

/* The format of the entries in the xdd function table */
typedef int (*func_ptr)(int32_t argc, char *argv[], uint32_t flags);

#define XDD_EXT_HELP_LINES 5
struct xdd_func {
	char    *func_name;     /* name of the function */
	char    *func_alt;      /* Alternate name of the function */
    int     (*func_ptr)(int32_t argc, char *argv[], uint32_t flags);      /* pointer to the function */
    int     argc;           /* number of arguments */
    char    *help;          /* help string */
    char    *ext_help[XDD_EXT_HELP_LINES];   /* Extented help strings */
	uint32_t flags;			/* Flags for various parsing functions */
}; 
typedef struct xdd_func xdd_func_t;

#define	XDD_FUNC_INVISIBLE	0x00000001	// When this flag is present then this command will not be displayed with "usage"

// Prototypes required by the parse_table() compilation
int xddfunc_blocksize(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_bytes(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_combinedout(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_createnewfiles(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_csvout(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_datapattern(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_debug(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_deletefile(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_deskew(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_devicefile(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_dio(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_dryrun(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_endtoend(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_errout(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_extended_stats(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_flushwrite(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_fullhelp(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_heartbeat(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_help(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_id(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_interactive(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_kbytes(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_lockstep(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_looseordering(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_maxall(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_maxerrors(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_max_errors_to_print(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_maxpri(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_mbytes(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_memalign(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_memory_usage(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_minall(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_multipath(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_nobarrier(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_nomemlock(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_noordering(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_noproclock(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_numreqs(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_operationdelay(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_operation(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_ordering(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_output(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_output_format(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_passdelay(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_passes(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_passoffset(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_percentcpu(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_preallocate(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_processlock(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_processor(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_queuedepth(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_randomize(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_readafterwrite(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_reallyverbose(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_recreatefiles(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_reopen(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_report_threshold(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_reqsize(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_restart(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_retry(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_roundrobin(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_runtime(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_rwratio(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_seek(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_setup(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_sgio(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_sharedmemory(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_singleproc(int32_t argc, char *argv[], uint32_t flags); 
int xddfunc_startdelay(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_startoffset(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_starttime(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_starttrigger(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_stoponerror(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_stoptrigger(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_serialordering(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_syncio(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_syncwrite(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_target(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_target_inout(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_target_inout_file(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_target_inout_network(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_targetdir(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_targetin(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_targetoffset(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_targets(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_targetstartdelay(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_targetout(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_throttle(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_timelimit(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_timerinfo(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_timeserver(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_timestamp(int32_t argc, char *argv[], uint32_t flags); 
int xddfunc_verify(int32_t argc, char *argv[], uint32_t flags); 
int xddfunc_unverbose(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_verbose(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_version(int32_t argc, char *argv[], uint32_t flags);
int xddfunc_invalid_option(int32_t argc, char *argv[], uint32_t flags);
void xddfunc_currently_undefined_option(char *sp);
 
/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
