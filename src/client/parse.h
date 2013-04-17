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

#include "xint_plan.h"

#define XDD_PARSE_PHASE1			0x00000001 // Preprocess phase - phase 1 
#define XDD_PARSE_PHASE2			0x00000002 // Parse Phase 2 occurs after we know how many targets there really are
#define XDD_PARSE_TARGET_IN			0x00000004 // Target In
#define XDD_PARSE_TARGET_OUT		0x00000008 // Target Out
#define XDD_PARSE_TARGET_FILE		0x00000010 // Target_inout I/O Type of "FILE"
#define XDD_PARSE_TARGET_NETWORK	0x00000020 // Target_inout I/O Type of "NETWORK"

/* The format of the entries in the xdd function table */
typedef int (*func_ptr)(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);

#define XDD_EXT_HELP_LINES 5
struct xdd_func {
	char    *func_name;     /* name of the function */
	char    *func_alt;      /* Alternate name of the function */
    int     (*func_ptr)(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);      /* pointer to the function */
    int     argc;           /* number of arguments */
    char    *help;          /* help string */
    char    *ext_help[XDD_EXT_HELP_LINES];   /* Extented help strings */
	uint32_t flags;			/* Flags for various parsing functions */
}; 
typedef struct xdd_func xdd_func_t;

#define	XDD_FUNC_INVISIBLE	0x00000001	// When this flag is present then this command will not be displayed with "usage"

// Prototypes required by the parse_table() compilation
int xddfunc_blocksize(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_bytes(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_combinedout(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_createnewfiles(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_csvout(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_datapattern(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_debug(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_deletefile(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_devicefile(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_dio(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_dryrun(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_endtoend(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_errout(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_extended_stats(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_flushwrite(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_fullhelp(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_heartbeat(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_help(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_id(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_interactive(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_kbytes(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_lockstep(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_looseordering(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_maxall(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_maxerrors(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_max_errors_to_print(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_maxpri(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_mbytes(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_memalign(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_memory_usage(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_minall(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_multipath(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_nobarrier(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_nomemlock(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_noordering(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_noproclock(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_numreqs(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_operationdelay(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_operation(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_ordering(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_output(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_output_format(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_passdelay(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_passes(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_passoffset(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_percentcpu(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_preallocate(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_processlock(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_processor(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_queuedepth(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_randomize(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_readafterwrite(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_reallyverbose(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_recreatefiles(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_reopen(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_report_threshold(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_reqsize(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_restart(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_retry(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_roundrobin(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_runtime(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_rwratio(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_seek(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_setup(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_sgio(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_sharedmemory(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_singleproc(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags); 
int xddfunc_startdelay(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_startoffset(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_starttime(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_starttrigger(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_stoponerror(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_stoptrigger(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_serialordering(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_syncio(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_syncwrite(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_target(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_target_inout(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_target_inout_file(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_target_inout_network(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_targetdir(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_targetin(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_targetoffset(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_targets(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_targetstartdelay(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_targetout(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_throttle(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_timelimit(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_timerinfo(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_timeserver(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_timestamp(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags); 
int xddfunc_verify(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags); 
int xddfunc_unverbose(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_verbose(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
int xddfunc_version(xdd_plan_t *planp, int32_t argc, char *argv[], uint32_t flags);
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
