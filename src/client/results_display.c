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
/*
 * This file contains the subroutines necessary to compose the output of a display line.
 */
#include "xint.h"

/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_what(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%15s","What           ");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%15s","    UNITS>>    ");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%15s",rp->what);
	}
}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_pass_number(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%7s","   Pass");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%7s"," Number");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%7d",rp->pass_number);
	}
}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_target_number(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%7s"," Target");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%7s"," Number");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%7d",rp->my_target_number);
	}

}
/*----------------------------------------------------------------------------*/
// For a RESULTS_QUEUE_PASS then the queue number is the specific number
// of that Worker Thread (i.e. 0, 1, 2, ...)
// Otherwise, the queue number should be the queue_depth (i.e. the total
// number of Worker Threads for this target).
void 
xdd_results_fmt_queue_number(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%7s","  Queue");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%7s"," Number");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		if (rp->flags & RESULTS_QUEUE_PASS) 
			fprintf(rp->output,"%7d",rp->my_worker_thread_number);
		else fprintf(rp->output,"%7d",rp->queue_depth);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_bytes_transferred(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%16s","    Bytes_Xfered");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%16s","           Bytes");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%16lld",(long long)rp->bytes_xfered);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_bytes_read(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%16s","      Bytes_Read");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%16s","           Bytes");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%16lld",(long long)rp->bytes_read);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_bytes_written(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%16s","   Bytes_Written");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%16s","           Bytes");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%16lld",(long long)rp->bytes_written);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_ops(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%11s","        Ops");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%11s","       #ops");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%11lld",(long long)rp->op_count);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_read_ops(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%11s","   Read_Ops");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%11s","       #ops");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%11lld",(long long)rp->read_op_count);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_write_ops(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%11s","  Write_Ops");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%11s","       #ops");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%11lld",(long long)rp->write_op_count);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_bandwidth(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%10s"," Bandwidth");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%10s","  MBytes/s");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%10.3f",rp->bandwidth);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_read_bandwidth(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%10s","   Read_BW");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%10s","  MBytes/s");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%10.3f",rp->read_bandwidth);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_write_bandwidth(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%10s","  Write_BW");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%10s","  MBytes/s");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%10.3f",rp->write_bandwidth);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_iops(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%10s","      IOPS");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%10s","     Ops/s");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%10.3f",rp->iops);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_read_iops(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%10s","   Rd_IOPS");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%10s","     Ops/s");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%10.3f",rp->read_iops);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_write_iops(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%10s","  Wrt_IOPS");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%10s","     Ops/s");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%10.3f",rp->write_iops);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_latency(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%9s","  Latency");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%9s"," millisec");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%9.3f",rp->latency);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_elapsed_time_from_1st_op(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%9s","   1st_Op");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%9s"," millisec");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%9.3f",rp->elapsed_pass_time_from_first_op);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_elapsed_time_from_pass_start(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%11s","    Elapsed");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%11s","    seconds");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%11.3f",rp->elapsed_pass_time);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_elapsed_over_head_time(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%9s"," Overhead");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%9s"," millisec");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%9.3f",-1.0);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_elapsed_pattern_fill_time(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%9s"," Pat_Fill");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%9s"," millisec");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%9.3f",-1.0);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_elapsed_buffer_flush_time(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%9s","    Flush");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%9s"," millisec");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%9.3f",-1.0);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_cpu_time(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%9s"," CPU Time");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%9s","  seconds");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%9.3f",rp->us_time);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_percent_cpu_time(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%9s"," Pct_CPU");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%9s"," percent");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%9.3f",rp->percent_cpu);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_user_time(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%9s","User Time");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%9s","  seconds");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%9.3f",rp->user_time);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_system_time(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%9s"," Sys Time");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%9s","  seconds");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%9.3f",rp->system_time);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_percent_user_time(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%9s","  Pct_Usr");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%9s","  percent");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%9.3f",rp->percent_user);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_percent_system_time(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%9s","  Pct_Sys");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%9s","  percent");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%9.3f",rp->percent_system);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_op_type(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%8s"," Op_Type");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%8s","    text");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%8s",rp->optype);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_transfer_size_bytes(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%12s","   Xfer_Size");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%12s","       bytes");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%12.0f",rp->xfer_size_bytes);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_transfer_size_blocks(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%12s","   Xfer_Size");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%12s","      blocks");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%12.2f",rp->xfer_size_blocks);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_transfer_size_kbytes(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%12s","   Xfer_Size");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%12s","  1024-bytes");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%12.2f",rp->xfer_size_kbytes);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_transfer_size_mbytes(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%12s","   Xfer_Size");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%12s","1024^2 bytes");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%12.2f",rp->xfer_size_mbytes);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_e2e_io_time(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%12s","     E2E_I/O");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%12s","     seconds");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%12.2f",rp->e2e_io_time_this_pass);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_e2e_send_receive_time(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%12s","     E2E_S/R");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%12s","     seconds");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%12.2f",rp->e2e_sr_time_this_pass);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_e2e_percent_send_receive_time(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%12s"," E2E_Pct_S/R");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%12s","     percent");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%12.2f",rp->e2e_sr_time_percent_this_pass);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_e2e_lag_time(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%8s"," E2E_Lag");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%8s","millisec");
	}  else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%8.2f",rp->e2e_wait_1st_msg);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_e2e_percent_lag_time(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%12s","E2E_Pct_Lag ");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%12s","   percent  ");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%12.2f",-1.0);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_e2e_first_read_time(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%12s","  E2E_1st_Rd");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%12s","    millisec");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%12.2f",-1.0);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_e2e_last_write_time(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%12s"," E2E_Lst_Wrt");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%12s","    millisec");
	} else if (rp->flags & RESULTS_PASS_INFO) {
		fprintf(rp->output,"%12.2f",-1.0);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_node_name(results_t *rp) {
	if (rp->flags & RESULTS_HEADER_TAG) {
		fprintf(rp->output,"%10s"," Node_Name");
	} else if (rp->flags & RESULTS_UNITS_TAG) {
		fprintf(rp->output,"%10s","      text");
	} else if (rp->flags & RESULTS_PASS_INFO) {
                struct utsname name;
                uname(&name);
		fprintf(rp->output,"%10s",name.nodename);
	}

}
/*----------------------------------------------------------------------------*/
void 
xdd_results_fmt_delimiter(results_t *rp) {
	fprintf(rp->output,"%c",rp->delimiter);

}

//-----------------------------------------------------------------------------------
// The Format Identifier structure for the results_fmt table below
struct xdd_results_fmt {
	char    *fmt_name;		/* name of the format identifier */
    void     (*func_ptr)(results_t *rp);	/* pointer to function that implements this format identifier*/
}; 
typedef struct xdd_results_fmt xdd_results_fmt_t;

static	xdd_results_fmt_t	xdd_results_fmt_table[] = { 
	{"+WHAT", 				xdd_results_fmt_what},
	{"+PASS", 				xdd_results_fmt_pass_number},
	{"+TARGET", 			xdd_results_fmt_target_number},
	{"+QUEUE",	 			xdd_results_fmt_queue_number},
	{"+BYTESTRANSFERED", 	xdd_results_fmt_bytes_transferred},
	{"+BYTESXFERED", 		xdd_results_fmt_bytes_transferred},
	{"+BYTESREAD", 			xdd_results_fmt_bytes_read},
	{"+BYTESWRITTEN", 		xdd_results_fmt_bytes_written},
	{"+OPS", 				xdd_results_fmt_ops},
	{"+READOPS", 			xdd_results_fmt_read_ops},
	{"+WRITEOPS", 			xdd_results_fmt_write_ops},
	{"+BANDWIDTH", 			xdd_results_fmt_bandwidth},
	{"+READBANDWIDTH", 		xdd_results_fmt_read_bandwidth},
	{"+WRITEBANDWIDTH", 	xdd_results_fmt_write_bandwidth},
	{"+IOPS", 				xdd_results_fmt_iops},
	{"+READIOPS", 			xdd_results_fmt_read_iops},
	{"+WRITEIOPS", 			xdd_results_fmt_write_iops},
	{"+LATENCY", 			xdd_results_fmt_latency},
	{"+ELAPSEDTIME1STOP", 	xdd_results_fmt_elapsed_time_from_1st_op},
	{"+ELAPSEDTIMEPASS", 	xdd_results_fmt_elapsed_time_from_pass_start},
	{"+OVERHEADTIME", 		xdd_results_fmt_elapsed_over_head_time},
	{"+PATTERNFILLTIME", 	xdd_results_fmt_elapsed_pattern_fill_time},
	{"+BUFFERFLUSHTIME", 	xdd_results_fmt_elapsed_buffer_flush_time},
	{"+CPUTIME", 			xdd_results_fmt_cpu_time},
	{"+PERCENTCPUTIME", 	xdd_results_fmt_percent_cpu_time},
	{"+PERCENTCPU",		 	xdd_results_fmt_percent_cpu_time},
	{"+USERTIME", 			xdd_results_fmt_user_time},
	{"+USER", 				xdd_results_fmt_user_time},
	{"+USRTIME", 			xdd_results_fmt_user_time},
	{"+SYSTEMTIME", 		xdd_results_fmt_system_time},
	{"+SYSTEM",		 		xdd_results_fmt_system_time},
	{"+SYSTIME", 			xdd_results_fmt_system_time},
	{"+PERCENTUSER", 		xdd_results_fmt_percent_user_time},
	{"+PERCENTUSERTIME", 	xdd_results_fmt_percent_user_time},
	{"+PERCENTUSR", 		xdd_results_fmt_percent_user_time},
	{"+PERCENTSYSTEM", 		xdd_results_fmt_percent_system_time},
	{"+PERCENTSYSTEMTIME", 	xdd_results_fmt_percent_system_time},
	{"+PERCENTSYS", 		xdd_results_fmt_percent_system_time},
	{"+OPTYPE", 			xdd_results_fmt_op_type},
	{"+XFERSIZEBYTES", 		xdd_results_fmt_transfer_size_bytes},
	{"+XFERSIZEBLOCKS",		xdd_results_fmt_transfer_size_blocks},
	{"+XFERSIZEKBYTES",		xdd_results_fmt_transfer_size_kbytes},
	{"+XFERSIZEMBYTES",		xdd_results_fmt_transfer_size_mbytes},
	{"+E2EIOTIME", 			xdd_results_fmt_e2e_io_time},
	{"+E2ESRTIME", 			xdd_results_fmt_e2e_send_receive_time},
	{"+E2EPERCENTSRTIME", 	xdd_results_fmt_e2e_percent_send_receive_time},
	{"+E2ELAGTIME", 		xdd_results_fmt_e2e_lag_time},
	{"+E2EPERCENTLAGTIME", 	xdd_results_fmt_e2e_percent_lag_time},
	{"+E2EFIRSTREADTIME", 	xdd_results_fmt_e2e_first_read_time},
	{"+E2ELASTWRITETIME", 	xdd_results_fmt_e2e_last_write_time},
	{"+DELIM", 				xdd_results_fmt_delimiter},
	{"+NODENAME", 				xdd_results_fmt_node_name},
	{ NULL, 				NULL}
	};

/*----------------------------------------------------------------------------*/
// This routine will take an input format line and call the appropriate functions 
// to process each of the format identifiers in the line
// This routine is called by the display_manager() once for each Worker Thread.
// The display_manager() passes a pointer to the Data Struct for the Worker Thread to be
// processed. 
// A variable in the results structure contains flags that indicate different
// behaviors.
// The value of "flags" can be one or more of the following:
// RESULTS_HEADER_TAG   - will cause each routine called to simply display
//                       the appropriate header tag.
// RESULTS_PASS_INFO - will cause each routine called to display
//                       results from the most recent pass.
// RESULTS_RUN_INFO  - will cause each routine called to display
//                       results from the entire run.
// RESULTS_TARGET_AVERAGE  - will cause each routine called to display
//                       results for a specific target averaged over a pass or run
// RESULTS_COMBINED     - will cause each routine called to display
//                       results for all targets over the entire run
// Each Worker Thread has a single results structure pointed to by its Worker Data Struct.
// The results structure contains all the relevant information that will be
// used by the individual routines in this file.
// The results structure also contains a pointer to the "format" string to
// use by the xdd_results_display(). 
// The format string has text and format identifiers that indicate what to
// display and when/where in the line to display it
// Example:
//    "Pass+{PASS#}+{DELIM}+{TARGET#}+{DELIM}+{QUEUE#}+{DELIM}+{BANDWIDTH}
// Hence this routine will call the format subroutines that hanble the various
// +{XXXXX} format variable identifiers included in the string
// In this example, the xdd_results_display() will invoke the following
// routines in this order:
//     xdd_results_fmt_pass_number()
//     xdd_results_delimeter()
//     xdd_results_fmt_target_number()
//     xdd_results_delimeter()
//     xdd_results_fmt_worker_thread_number()
//     xdd_results_delimeter()
//     xdd_results_fmt_bandwidth()
//
// The output like might look something like:
//   1,0,0,37.2
// For pass 1, target 0, Worker Thread 0, and a bandwidth of 37.2 MB/sec.
// Note that the "delimiter" is set to a comma.
//
void *
xdd_results_display(results_t *rp) {
	int		index;
	char	*cp;
	char	*sp;
	int		splen;
	int		remaining;
	int		namelen;
	char	*namep;
	int		found;
	char	output_delimiter;

	
	
	output_delimiter=0;
	sp = rp->format_string;
	splen = strlen(sp);
	if (splen <= 0) {
		return(0);
	}
	cp = sp;
	remaining = splen;
	while (remaining > 0) {
		index = 0;	
		found = 0;
		while (xdd_results_fmt_table[index].fmt_name != NULL) {
			namep = xdd_results_fmt_table[index].fmt_name;
			namelen = strlen(namep);
			if (namelen > remaining) { // A strcmp would put us past the end of this string
				index++; // just look at the next fmt_name because it is not this one
				continue;
			}
			if (strncmp(cp, namep, namelen) == 0) { // This is the right fmt id
				// Output a "delimiter" character before we output the value
				if (output_delimiter) 
					fprintf(rp->output,"%c",rp->delimiter);
				else output_delimiter = 1;
				xdd_results_fmt_table[index].func_ptr(rp); // Call the associated routine
				found = 1;
				break;
			}
			index++;
		} // End of WHILE loop that scans the table

		if (found == 1) { // At this point a match was found 
			cp += namelen; // increment past the start of the last token
			remaining -= namelen; // decrement the "ramining" counter
			continue;
		}
		// no matches on the last scan thru the fmt_table
		fputc(*cp, stderr);  // Display this character
		cp++;
		remaining--;
	} // End of WHILE(remaining)

	// At this point each of the fmt_ids have been processed.
	fprintf(rp->output,"\n");
	return(0);
			
}

/*----------------------------------------------------------------------------*/
// This routine will add a string of format IDs or other text to the end
// of the existing format ID string.
void 
xdd_results_format_id_add( char *sp , char *format_stringp) {

	char	*tmpp;
	int		length_format_string;
	int		length_added_string;
	int		new_length;


	length_format_string = strlen(format_stringp);
	length_added_string = strlen(sp);

	new_length = length_format_string + length_added_string + 2;
	tmpp = malloc(new_length);
	if (tmpp == NULL ) {
		fprintf(xgp->errout, "%s: xdd_results_format_id_add: Warning: Cannot allocate %d bytes of memory for adding '%s' to format string '%s'\n",
			xgp->progname, 
			new_length,
			sp,
			format_stringp);
	}
	sprintf(tmpp, "%s%s ",format_stringp, sp);
	format_stringp = tmpp;
	
	return;
}
