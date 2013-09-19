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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#define  THIS_IS_A_SUBROUTINE 
#include "bx_data_structures.h"


// The following is the format of the --target option
//    0     1    2         3             4               5                    6             7                   8
// --target # <in|out|int> #WorkerThreads     <file|net|sg> <options>
// --target # <in|out|int> #WorkerThreads     file          <filename>      <file_size>			<transfer_size>		<dio,null,sync>
// --target # <in|out|int> #WorkerThreads     network       <client|server> <user@hostname:port> <TCP|UDP|UDT>
// --target # <in|out|int> #WorkerThreads     sg            /dev/sg#
//
// The #defines for TARGET_* define the offset from the --target argument of the 
// different types of targets
// For example, if --target is argv[1] in the command line then the target number
// is the next argument (argv[1+1} or argv[2]), the target "designation" as an input,
// output, or internal is argv[1+2] or argv[3], and the target "type" as a file, 
// network, or sg device is argv[4]. The arguments following the target "type" 
// depend on the "type". If the target is of type "file" then the following arguments
// will be the file name followed by various file-related options (like directIO).
// If the target is of type "network" then the following arguments will be
// network-related items. 
// Multiple targets need to be specified in a command line so the #define
// TARGET_* definitions are offsets relative to the occurence of a --target 
// in the command line. 

#define TARGET_NUMBER			1							// Target number relative to zero
#define TARGET_INOUT			TARGET_NUMBER+1				// Designation of this target - input or output or internal
#define TARGET_WORKER_THREADS			TARGET_INOUT+1				// Number of WorkerThreads for this target
#define TARGET_TYPE				TARGET_WORKER_THREADS+1		 	// file or network or sg

#define TARGET_FILE_NAME		TARGET_TYPE+1   			// name of file for type file
#define TARGET_FILE_SIZE		TARGET_FILE_NAME+1   		// size of file in bytes
#define TARGET_FILE_XFER_SIZE	TARGET_FILE_SIZE+1   		// size of each data transfer in bytes
#define TARGET_FILE_OPTIONS		TARGET_FILE_XFER_SIZE+1   	// options for type file
#define TARGET_FILE_LASTARG		TARGET_FILE_OPTIONS   		// Last argument for type file

#define TARGET_NETWORK_CS		TARGET_TYPE+1   			// Client or Server for type network
#define TARGET_NETWORK_HOSTNAME	TARGET_NETWORK_CS+1			// name of remote user@host:port for type network
#define TARGET_NETWORK_PROTO	TARGET_NETWORK_HOSTNAME+1	// Protocol to use for type network 
#define TARGET_NETWORK_LASTARG	TARGET_NETWORK_PROTO   		// Last argument for type network

#define TARGET_SG_NAME			TARGET_TYPE+1   			// Full path name to SG device for type SG (SCSI Generic)
#define TARGET_SG_LASTARG		TARGET_SG_NAME   			// Last argument for type sg


//---------------------------------------------------------------------------//
// get_target_file_options() will process the arguments that follow the 
// --target command-line option that are specific to a "file" target type.
// Upon success this subroutine will return the number of arguments processed.
//
int
get_target_file_options(int target_number, int index, int argc, char *argv[]) {

fprintf(stderr,"get_target_file_options: target_number=%d, index=%d, argc=%d\n",target_number,index,argc);
	// Sanity check the number of remaining arguments
	if (index+TARGET_FILE_LASTARG >= argc-1) {
		fprintf(stderr, "get_target_file_options: Not enough remaining arguments\n");
		return(-1);
	}
	// TYPE FILE
	bx_td[target_number].bx_td_flags |= BX_TD_TYPE_FILE;

	// FILENAME
	bx_td[target_number].bx_td_file_name = argv[index+TARGET_FILE_NAME];
fprintf(stderr,"get_target_file_options: target_number=%d,file name=%s\n",target_number, bx_td[target_number].bx_td_file_name);

	// FILE SIZE
	bx_td[target_number].bx_td_file_size = atoll(argv[index+TARGET_FILE_SIZE]);
fprintf(stderr,"get_target_file_options: target_number=%d,file size=%lld\n",target_number, bx_td[target_number].bx_td_file_size);

	// TRANSFER SIZE
	bx_td[target_number].bx_td_transfer_size = atoi(argv[index+TARGET_FILE_XFER_SIZE]);
	if (Buffer_Size <  bx_td[target_number].bx_td_transfer_size)
		Buffer_Size =  bx_td[target_number].bx_td_transfer_size;
fprintf(stderr,"get_target_file_options: target_number=%d,transfer size=%d\n",target_number, bx_td[target_number].bx_td_transfer_size);
	
	// OPTIONS
	if (0 == strcmp(argv[index+TARGET_FILE_OPTIONS],"bio")) 
		bx_td[target_number].bx_td_flags |= BX_TD_TYPE_FILE_BIO;
	else if (0 == strcmp(argv[index+TARGET_FILE_OPTIONS],"dio")) 
		bx_td[target_number].bx_td_flags |= BX_TD_TYPE_FILE_DIO;
	else if (0 == strcmp(argv[index+TARGET_FILE_OPTIONS],"NULL")) 
		bx_td[target_number].bx_td_flags |= BX_TD_TYPE_FILE_NULL;
	else {
		fprintf(stderr, "get_target_network_options: Invalid file option '%s' - must be 'bio', 'dio', or 'null'\n", argv[index+TARGET_FILE_OPTIONS]);
		return(-1);
	}

fprintf(stderr,"get_target_file_options: return=%d\n",TARGET_FILE_LASTARG+1);
	return(TARGET_FILE_LASTARG+1);
} // End of get_target_file_options()

//---------------------------------------------------------------------------//
// get_target_network_options() will process the arguments that follow the 
// --target command-line option that are specific to a "net" target type.
// Upon success this subroutine will return the number of arguments processed.
//
int
get_target_network_options(int target_number, int index, int argc, char *argv[]) {
	int 	i;
	int		length;
	char	*cp;

	// Sanity check the number of remaining arguments
	if (index+TARGET_NETWORK_LASTARG >= argc-1) {
		fprintf(stderr, "get_target_net_options: Not enough remaining arguments\n");
		return(-1);
	}
	// TYPE NETWORK
	bx_td[target_number].bx_td_flags |= BX_TD_TYPE_NETWORK;
	
	// CLIENT or SERVER
	if (0 == strcmp(argv[index+TARGET_NETWORK_CS],"client")) 
		bx_td[target_number].bx_td_flags |= BX_TD_TYPE_NETWORK_CLIENT;
	else if (0 == strcmp(argv[index+TARGET_NETWORK_CS],"server")) 
		bx_td[target_number].bx_td_flags |= BX_TD_TYPE_NETWORK_SERVER;
	else {
		fprintf(stderr, "get_target_network_options: Invalid network target '%s' - must be 'client' or 'server'\n", argv[index+TARGET_NETWORK_CS]);
		return(-1);
	}
	
	// USER@HOSTNAME:PORT
	// user name
	cp = argv[index+TARGET_NETWORK_HOSTNAME];
	// Record the start of the "username" part of this string
	bx_td[target_number].bx_td_network_user = cp;

	// Get the length of the user@hostname:port string 
	length = strlen(argv[index+TARGET_NETWORK_HOSTNAME]);
	i = 0;
	// Locate the end of the "username" and null-terminate "username"
	while ((*cp != '@') && (i <= length)) {
		cp++;
		i++;
	}
	*cp = '\0';
	cp++;
	// Record the start of the "hostname" part of this string
	bx_td[target_number].bx_td_network_hostname = cp;
	// Locate the end of the "hostname" and null-terminate "hostname"
	while ((*cp != ':') && (i <= length)) {
		cp++;
		i++;
	}
	*cp = '\0';
	cp++;
	// Record the start of the "port" part of this string
	bx_td[target_number].bx_td_network_port = cp;

	// PROTOCOL
	if (0 == strcmp(argv[index+TARGET_NETWORK_PROTO],"tcp")) 
		bx_td[target_number].bx_td_flags |= BX_TD_TYPE_NETWORK_TCP;
	else if (0 == strcmp(argv[index+TARGET_NETWORK_PROTO],"udp")) 
		bx_td[target_number].bx_td_flags |= BX_TD_TYPE_NETWORK_UDP;
	else if (0 == strcmp(argv[index+TARGET_NETWORK_PROTO],"udt"))
		bx_td[target_number].bx_td_flags |= BX_TD_TYPE_NETWORK_UDT;
	else if (0 == strcmp(argv[index+TARGET_NETWORK_PROTO],"rdma"))
		bx_td[target_number].bx_td_flags |= BX_TD_TYPE_NETWORK_RDMA;
	else {
		fprintf(stderr, "get_target_network_options: Invalid network protocol '%s' - must be 'tcp', 'udp', 'udt', or 'rdma'\n", argv[index+TARGET_NETWORK_PROTO]);
		return(-1);
	}
	return(TARGET_NETWORK_LASTARG+1);

} // End of get_target_network_options()

//---------------------------------------------------------------------------//
// get_target_file_options() will process the arguments that follow the 
// --target command-line option that are specific to an "sg" target type.
// Upon success this subroutine will return the number of arguments processed.
//
int
get_target_sg_options(int target_number, int index, int argc, char *argv[]) {

	// Sanity check the number of remaining arguments
	if (index+TARGET_SG_LASTARG >= argc-1) {
		fprintf(stderr, "get_target_sg_options: Not enough remaining arguments\n");
		return(-1);
	}

	// TYPE SG
	bx_td[target_number].bx_td_flags |= BX_TD_TYPE_SG;

	// SG DEVICE NAME
	bx_td[target_number].bx_td_sg_device_name = argv[index+TARGET_SG_NAME];
	
	return(TARGET_SG_LASTARG+1);

} // End of get_target_sg_options()

//---------------------------------------------------------------------------//
// get_target_options() will process the arguments that follow the --target 
// command-line option.
// Upon success this subroutine will return the number of arguments processed.
//
int
get_target_options(int index, int argc, char *argv[]) {
	int		status;
	int		target_number;
	int		worker_threads;
	int		current_arg_index;


	// Target Number
	// Sanity check the number of remaining arguments
fprintf(stderr,"get_target_options: index=%d, argc=%d\n",index,argc);
	current_arg_index = index;
	if (current_arg_index >= argc-1) {
		fprintf(stderr, "get_target_options: Not enough remaining arguments\n");
		return(-1);
	}
	target_number = atoi(argv[index+TARGET_NUMBER]);
	if ((target_number >= MAX_TARGETS) || (target_number < 0)) {
		fprintf(stderr, "get_target_options: Invalid target number '%d' - must be between 0 and %d\n", target_number,MAX_TARGETS);
		return(-1);
	}
	bx_td[target_number].bx_td_my_target_number = target_number;
	bx_td[target_number].bx_td_flags = BX_TD_TARGET_DEFINED;
	Number_Of_Targets++;

	// IN or OUT
	// Sanity check the number of remaining arguments
	current_arg_index++;
	if (current_arg_index == argc-1) {
		fprintf(stderr, "get_target_options: Not enough remaining arguments\n");
		return(-1);
	}

	if (0 == strcmp(argv[index+TARGET_INOUT],"in")) 
		bx_td[target_number].bx_td_flags |= BX_TD_INPUT;
	else if (0 == strcmp(argv[index+TARGET_INOUT],"out")) 
		bx_td[target_number].bx_td_flags |= BX_TD_OUTPUT;
	else if (0 == strcmp(argv[index+TARGET_INOUT],"internal")) 
		bx_td[target_number].bx_td_flags |= BX_TD_INTERNAL;
	else {
		fprintf(stderr, "get_target_options: Invalid target designator '%s' - must be 'in', 'out', or 'internal'\n", argv[index+TARGET_INOUT]);
		return(-1);
	}
	
	// # Worker Threads
	// Sanity check the number of remaining arguments
	current_arg_index++;
	if (current_arg_index >= argc-1) {
		fprintf(stderr, "get_target_options: Not enough remaining arguments\n");
		return(-1);
	}
	worker_threads = atoi(argv[index+TARGET_WORKER_THREADS]);
	if ((worker_threads < 1) || (worker_threads > MAX_WORKER_THREADS)) {
		fprintf(stderr, "get_target_options: Invalid number of worker_threads '%d' - must be greater than 0 and less than %d\n", worker_threads, MAX_WORKER_THREADS);
		return(-1);
	}
	bx_td[target_number].bx_td_number_of_worker_threads = worker_threads;

	// TYPE
	// Sanity check the number of remaining arguments
	current_arg_index++;
	if (current_arg_index >= argc-1) {
		fprintf(stderr, "get_target_options: Not enough remaining arguments\n");
		return(-1);
	}

	if (0 == strcmp(argv[index+TARGET_TYPE],"file"))
		status = get_target_file_options(target_number, index, argc, argv);
	else if (0 == strcmp(argv[index+TARGET_TYPE],"net")) 
		status = get_target_network_options(target_number, index, argc, argv);
	else if (0 == strcmp(argv[index+TARGET_TYPE],"sg")) 
		status = get_target_sg_options(target_number, index, argc, argv);
	else {
		fprintf(stderr, "get_target_options: Invalid target type '%s' - must be 'file', 'net', or 'sg'\n", argv[index+TARGET_TYPE]);
		return(-1);
	}

	// At this point the status could be the number of arguments processed 
	// or -1 if something went wrong
fprintf(stderr,"get_target_options: return=%d\n",status);
	return(status);

} // End of get_target_options()

//---------------------------------------------------------------------------//
// get_sequence() will process the arguments that follow the --sequence
// command-line option.
// The "sequence" is the order in which a buffer should be processed from
// one target to the next. The order is simply the target numbers from 
// left to right.
// Example:
// .... --sequence 0 1 2 
// will have each buffer start at target 0 which will pass it to target 1
// which will subsequently pass it to target 2. 
//
// Once the last target in the sequence is done with a buffer, that buffer
// will be given back to the first target in the sequence. 
//
// Upon success this subroutine will return the number of arguments processed.
//
int
get_sequence(int index, int current_argc, int argc, char *argv[]) {
	int 	i;
	int		new_index;
	int		sequence_index;
	int		target_number;
	int		args_remaining;
	char	*cp;

fprintf(stderr,"get_sequence: index=%d, argc=%d\n",index,argc);
	args_remaining = argc-index-1;
	if (args_remaining == 0)
		return(0);
	new_index = index+1;
	args_remaining = argc-new_index-1;
	if (args_remaining == 0)
		return(0);
	cp = argv[new_index];

	sequence_index = 0;
	while (*cp != '-') {
		if (sequence_index > MAX_SEQUENCE_ENTRIES-1) {
			fprintf(stderr, "get_sequence: Too many targets specified in this sequence - must be less than %d\n", MAX_SEQUENCE_ENTRIES);
			return(-1);
		}
		Sequence[sequence_index] = atoi(argv[new_index]);
		if ((Sequence[sequence_index] < 0) || (Sequence[sequence_index] > MAX_TARGETS)) {
			fprintf(stderr, "get_sequence: Invalid sequence number '%d' - must be between 0 and %d\n", Sequence[sequence_index],MAX_TARGETS);
			return(-1);
		}
fprintf(stderr,"get_sequence: sequence_index %d, Sequence is %d, argc=%d, new_index=%d\n",sequence_index, Sequence[sequence_index],argc,new_index);
		Sequence[sequence_index+1] = Sequence[0]; // Points to the first target in the sequence
		new_index++;
		args_remaining = argc-new_index;
		if (args_remaining == 0)
			break;
		cp = argv[new_index];
		sequence_index++;
	}
	// At this point the "sequence_index" is the number of targets in the sequence
	// We need to point the bx_td of each target to the subsequent target
	for (i = 0; i < sequence_index; i++) {
		target_number = Sequence[i];
		bx_td[target_number].bx_td_next_buffer_queue = Sequence[i+1];
	}
fprintf(stderr,"get_sequence: return=%d\n",sequence_index);
	return(sequence_index+2);

} // End of get_sequence()

int
ui(int argc, char *argv[]) {
	int		i;
	int 	current_argc;
	int		status = 0;

	// Scan the arguments and mark the "options"
	//

	current_argc = argc;
	i = 1;
	while (current_argc > 1) {
fprintf(stderr,"ui: current_argc=%d\n",current_argc);
		if (0 == strcmp(argv[i],"--target")) {
fprintf(stderr,"ui: getting target options - current_argc=%d, i=%d, argv[i]='%s'\n",current_argc,i,argv[i]);
			status = get_target_options(i, argc, argv);
			if (status < 0)
				break;
			current_argc -= status;
			i += status;
		} else if (0 == strcmp(argv[i],"--buffers")) {
fprintf(stderr,"ui: getting buffer options - current_argc=%d, i=%d, argv[i]='%s'\n",current_argc,i,argv[i]);
			Number_Of_Buffers = atoi(argv[i+1]);
			if (Number_Of_Buffers < 1)
				break;
			current_argc -= 2;
			i += 2;
fprintf(stderr,"ui: got buffer options - current_argc=%d, i=%d\n",current_argc,i);
		} else if (0 == strcmp(argv[i],"--debug")) {
fprintf(stderr,"ui: getting debug options - current_argc=%d, i=%d, argv[i]='%s'\n",current_argc,i,argv[i]);
			Debug_Level = atoi(argv[i+1]);
			if (Number_Of_Buffers < 1)
				break;
			current_argc -= 2;
			i += 2;
		} else if (0 == strcmp(argv[i],"--sequence")) {
fprintf(stderr,"ui: getting sequence options - current_argc=%d, i=%d, argv[i]='%s'\n",current_argc,i,argv[i]);
			status = get_sequence(i, current_argc, argc, argv);
			if (status < 0)
				break;
			current_argc -= status;
			i += status;
fprintf(stderr,"ui: got sequence options - current_argc=%d, status=%d, i=%d\n",current_argc,status,i);
		} else { 
			fprintf(stderr,"ui: Invalid argument: '%s'\n",argv[i]);
			usage(argv[0]);
			return(-1);
		}
fprintf(stderr,"ui: end of WHILE - current_argc=%d, status=%d, argc=%d\n",current_argc,status,argc);

	} 

	if (status > 0)
		status = 0;
	return(status);

} // End of ui()

void
usage(char *progname) {

	fprintf(stderr,"Usage: %s --target <options> --buffers # --sequence <# # #...#> --debug\n",progname);
	fprintf(stderr,"The following is the format of the --target option\n");
	fprintf(stderr,"   0     1 2            3             4             5               6                    7                8\n");
	fprintf(stderr,"--target # <in|out|int> #WorkerThreads     <file|net|sg> <options>\n");
	fprintf(stderr,"--target # <in|out|int> #WorkerThreads     file          <filename>      <file_size>          <transfer_size>  <dio,null,sync>\n");
	fprintf(stderr,"--target # <in|out|int> #WorkerThreads     network       <client|server> <user@hostname:port> <TCP|UDP|UDT>\n");
	fprintf(stderr,"--target # <in|out|int> #WorkerThreads     sg            /dev/sg#\n");
}
