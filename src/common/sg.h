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

#if LINUX 
#include <linux/major.h>
#include <sys/sysmacros.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <scsi/scsi.h>
/*
  Getting the correct include files for the sg interface can be an ordeal.
  In a perfect world, one would just write:
    #include <scsi/sg.h>
    #include <scsi/scsi.h>
  This would include the files found in the /usr/include/scsi directory.
  Those files are maintained with the GNU library which may or may not
  agree with the kernel and version of sg driver that is running. Any
  many cases this will not matter. However in some it might, for example
  glibc 2.1's include files match the sg driver found in the lk 2.2
  series. Hence if glibc 2.1 is used with lk 2.4 then the additional
  sg v3 interface will not be visible.
  If this is a problem then defining SG_KERNEL_INCLUDES will access the
  kernel supplied header files (assuming they are in the normal place).
  The GNU library maintainers and various kernel people don't like
  this approach (but it does work).
  The technique selected by defining SG_TRICK_GNU_INCLUDES worked (and
  was used) prior to glibc 2.2 . Prior to that version /usr/include/linux
  was a symbolic link to /usr/src/linux/include/linux .

  There are other approaches if this include "mixup" causes pain. These
  would involve include files being copied or symbolic links being
  introduced.

  Sorry about the inconvenience. Typically neither SG_KERNEL_INCLUDES
  nor SG_TRICK_GNU_INCLUDES is defined.

  dpg 20010415
*/

#if defined(__GNUC__) || defined(HAS_LONG_LONG)
typedef long long llse_loff_t;
#else
typedef long      llse_loff_t;
#endif

extern llse_loff_t llse_llseek(unsigned int fd,
                               llse_loff_t offset,
			       unsigned int origin);
//--------------------- Previously from sg_err.h ------------------//
#ifndef SG_ERR_H
#define SG_ERR_H

/* Feel free to copy and modify this GPL-ed code into your applications. */

/* Version 0.84 (20010115) 
	- all output now sent to stderr rather thatn stdout
	- remove header files included in this file
*/


/* Some of the following error/status codes are exchanged between the
   various layers of the SCSI sub-system in Linux and should never
   reach the user. They are placed here for completeness. What appears
   here is copied from drivers/scsi/scsi.h which is not visible in
   the user space. */

/* The following are 'host_status' codes */
#ifndef DID_OK
#define DID_OK 0x00
#endif
#ifndef DID_NO_CONNECT
#define DID_NO_CONNECT 0x01     /* Unable to connect before timeout */
#define DID_BUS_BUSY 0x02       /* Bus remain busy until timeout */
#define DID_TIME_OUT 0x03       /* Timed out for some other reason */
#define DID_BAD_TARGET 0x04     /* Bad target (id?) */
#define DID_ABORT 0x05          /* Told to abort for some other reason */
#define DID_PARITY 0x06         /* Parity error (on SCSI bus) */
#define DID_ERROR 0x07          /* Internal error */
#define DID_RESET 0x08          /* Reset by somebody */
#define DID_BAD_INTR 0x09       /* Received an unexpected interrupt */
#define DID_PASSTHROUGH 0x0a    /* Force command past mid-level */
#define DID_SOFT_ERROR 0x0b     /* The low-level driver wants a retry */
#endif

/* These defines are to isolate applictaions from kernel define changes */
#define SG_ERR_DID_OK           DID_OK
#define SG_ERR_DID_NO_CONNECT   DID_NO_CONNECT
#define SG_ERR_DID_BUS_BUSY     DID_BUS_BUSY
#define SG_ERR_DID_TIME_OUT     DID_TIME_OUT
#define SG_ERR_DID_BAD_TARGET   DID_BAD_TARGET
#define SG_ERR_DID_ABORT        DID_ABORT
#define SG_ERR_DID_PARITY       DID_PARITY
#define SG_ERR_DID_ERROR        DID_ERROR
#define SG_ERR_DID_RESET        DID_RESET
#define SG_ERR_DID_BAD_INTR     DID_BAD_INTR
#define SG_ERR_DID_PASSTHROUGH  DID_PASSTHROUGH
#define SG_ERR_DID_SOFT_ERROR   DID_SOFT_ERROR

/* The following are 'driver_status' codes */
#ifndef DRIVER_OK
#define DRIVER_OK 0x00
#endif
#ifndef DRIVER_BUSY
#define DRIVER_BUSY 0x01
#define DRIVER_SOFT 0x02
#define DRIVER_MEDIA 0x03
#define DRIVER_ERROR 0x04
#define DRIVER_INVALID 0x05
#define DRIVER_TIMEOUT 0x06
#define DRIVER_HARD 0x07
#define DRIVER_SENSE 0x08       /* Sense_buffer has been set */

/* Following "suggests" are "or-ed" with one of previous 8 entries */
#define SUGGEST_RETRY 0x10
#define SUGGEST_ABORT 0x20
#define SUGGEST_REMAP 0x30
#define SUGGEST_DIE 0x40
#define SUGGEST_SENSE 0x80
#define SUGGEST_IS_OK 0xff
#endif
#ifndef DRIVER_MASK
#define DRIVER_MASK 0x0f
#endif
#ifndef SUGGEST_MASK
#define SUGGEST_MASK 0xf0
#endif

/* These defines are to isolate applictaions from kernel define changes */
#define SG_ERR_DRIVER_OK        DRIVER_OK
#define SG_ERR_DRIVER_BUSY      DRIVER_BUSY
#define SG_ERR_DRIVER_SOFT      DRIVER_SOFT
#define SG_ERR_DRIVER_MEDIA     DRIVER_MEDIA
#define SG_ERR_DRIVER_ERROR     DRIVER_ERROR
#define SG_ERR_DRIVER_INVALID   DRIVER_INVALID
#define SG_ERR_DRIVER_TIMEOUT   DRIVER_TIMEOUT
#define SG_ERR_DRIVER_HARD      DRIVER_HARD
#define SG_ERR_DRIVER_SENSE     DRIVER_SENSE
#define SG_ERR_SUGGEST_RETRY    SUGGEST_RETRY
#define SG_ERR_SUGGEST_ABORT    SUGGEST_ABORT
#define SG_ERR_SUGGEST_REMAP    SUGGEST_REMAP
#define SG_ERR_SUGGEST_DIE      SUGGEST_DIE
#define SG_ERR_SUGGEST_SENSE    SUGGEST_SENSE
#define SG_ERR_SUGGEST_IS_OK    SUGGEST_IS_OK
#define SG_ERR_DRIVER_MASK      DRIVER_MASK
#define SG_ERR_SUGGEST_MASK     SUGGEST_MASK



/* The following "print" functions send ACSII to stdout */
extern void sg_print_command(const unsigned char *command, FILE *outp);
extern void sg_print_sense(	const char *leadin,
							const unsigned char *sense_buffer, 
							int sb_len, 
							FILE *outp);
extern void sg_print_status(int masked_status, FILE *outp);
extern void sg_print_host_status(int host_status, FILE *outp);
extern void sg_print_driver_status(int driver_status, FILE *outp);

/* sg_chk_n_print() returns 1 quietly if there are no errors/warnings
   else it prints to standard output and returns 0. */
extern int sg_chk_n_print(	const char *leadin, 
							int masked_status,
							int host_status, 
							int driver_status,
							const unsigned char *sense_buffer, 
							int sb_len, 
							FILE *outp);

/* The following function declaration is for the sg version 3 driver. 
   Only version 3 sg_err.c defines it. */
struct sg_io_hdr;
extern int sg_chk_n_print3(const char *leadin, struct sg_io_hdr *hp, FILE *outp);


/* The following "category" function returns one of the following */
#define SG_ERR_CAT_CLEAN 0      /* No errors or other information */
#define SG_ERR_CAT_MEDIA_CHANGED 1 /* interpreted from sense buffer */
#define SG_ERR_CAT_RESET 2      /* interpreted from sense buffer */
#define SG_ERR_CAT_TIMEOUT 3
#define SG_ERR_CAT_RECOVERED 4  /* Successful command after recovered err */
#define SG_ERR_CAT_SENSE 98     /* Something else is in the sense buffer */
#define SG_ERR_CAT_OTHER 99     /* Some other error/warning has occurred */

extern int sg_err_category(int masked_status, 
							int host_status,
							int driver_status, 
							const unsigned char *sense_buffer,
							int sb_len);

/* The following function declaration is for the sg version 3 driver. 
   Only version 3 sg_err.c defines it. */
extern int sg_err_category3(struct sg_io_hdr *hp);
#endif
#endif
 
//******************************************************************************
/*  ASCII values for a number of symbolic constants, printing functions, etc.
*
*  Some of the tables have been updated for SCSI 2.
*
*/
/*----------------------------------------------------------------------------*/
// Macros for getting the command size and command group
#define group(opcode) (((opcode) >> 5) & 7)
#define COMMAND_SIZE(opcode) scsi_command_size[((opcode) >> 5) & 7]

#define RESERVED_GROUP  0
#define VENDOR_GROUP    1

static const unsigned char scsi_command_size[8] = { 6, 10, 10, 12,
                                                   12, 12, 10, 10 };
// The following constants are used in the command tables that follow
static const char reserved[] = "RESERVED";
static const char vendor[] = "VENDOR SPECIFIC";
static const char unknown[] = "UNKNOWN";

static const char * group_0_commands[] = {
/* 00-03 */ "Test Unit Ready", "Rezero Unit", unknown, "Request Sense",
/* 04-07 */ "Format Unit", "Read Block Limits", unknown, "Reasssign Blocks",
/* 08-0d */ "Read (6)", unknown, "Write (6)", "Seek (6)", unknown, unknown,
/* 0e-12 */ unknown, "Read Reverse", "Write Filemarks", "Space", "Inquiry",
/* 13-16 */ "Verify", "Recover Buffered Data", "Mode Select", "Reserve",
/* 17-1b */ "Release", "Copy", "Erase", "Mode Sense", "Start/Stop Unit",
/* 1c-1d */ "Receive Diagnostic", "Send Diagnostic",
/* 1e-1f */ "Prevent/Allow Medium Removal", unknown,
};

static const char *group_1_commands[] = {
/* 20-22 */  unknown, unknown, unknown,
/* 23-28 */ unknown, "Define window parameters", "Read Capacity",
            unknown, unknown, "Read (10)",
/* 29-2d */ "Read Generation", "Write (10)", "Seek (10)", "Erase",
            "Read updated block",
/* 2e-31 */ "Write Verify","Verify", "Search High", "Search Equal",
/* 32-34 */ "Search Low", "Set Limits", "Prefetch or Read Position",
/* 35-37 */ "Synchronize Cache","Lock/Unlock Cache", "Read Defect Data",
/* 38-3c */ "Medium Scan", "Compare", "Copy Verify", "Write Buffer",
            "Read Buffer",
/* 3d-3f */ "Update Block", "Read Long",  "Write Long",
};

static const char *group_2_commands[] = {
/* 40-41 */ "Change Definition", "Write Same",
/* 42-48 */ "Read sub-channel", "Read TOC", "Read header",
            "Play audio (10)", unknown, "Play audio msf",
            "Play audio track/index",
/* 49-4f */ "Play track relative (10)", unknown, "Pause/resume",
            "Log Select", "Log Sense", unknown, unknown,
/* 50-55 */ unknown, unknown, unknown, unknown, unknown, "Mode Select (10)",
/* 56-5b */ unknown, unknown, unknown, unknown, "Mode Sense (10)", unknown,
/* 5c-5f */ unknown, unknown, unknown,
};

#ifndef READ_16
#define READ_16               0x88
#endif
#ifndef WRITE_16
#define WRITE_16              0x8a
#endif

#ifdef ndef
// The group_4 command are currently not used
/* The following are 16 byte commands in group 4 */
static const char *group_4_commands[] = {
/* 80  Z              */ "XDWRITE EXTENDED(16) [SBC] or WRITE FILEMARKS(16)"
/* 81  Z              */ "REBUILD(16) [SBC] or READ REVERSE(16)",
/* 82  Z              */ "REGENERATE(16) [SBC]",
/* 83  OOOOO O    OO  */ "EXTENDED COPY",
/* 84  OOOOO O    OO  */ "RECEIVE COPY RESULTS",
/* 85  O    O    O    */ "ATA COMMAND PASS THROUGH(16)",
/* 86  OO OO OOOOOOO  */ "ACCESS CONTROL IN",
/* 87  OO OO OOOOOOO  */ "ACCESS CONTROL OUT",
/* 88  MO  O O   O    */ "READ(16)",
/* 89 */
/* 8A  OO  O O   O    */ "WRITE(16)",
/* 8B  O              */ "ORWRITE",
/* 8C  OO  O OO  O M  */ "READ ATTRIBUTE",
/* 8D  OO  O OO  O O  */ "WRITE ATTRIBUTE",
/* 8E  O   O O   O    */ "WRITE AND VERIFY(16)",
/* 8F  OO  O O   O    */ "VERIFY(16)",
/* 90  O   O O   O    */ "PRE-FETCH(16)",
/* 91  O   O O   O    */ "SYNCHRONIZE CACHE(16) or SPACE(16)",
/* 92  Z   O O        */ "LOCK UNLOCK CACHE(16) [SBC] or LOCATE(16)",
/* 93  O              */ "WRITE SAME(16) or ERASE(16)",
/* 94 -97 [usage proposed by SCSI Socket Services project] */
						 "SCSI Socket Services", "SCSI Socket Services", "SCSI Socket Services", "SCSI Socket Services",
/* 98 - 9D undefined */
						"unknown","unknown","unknown","unknown","unknown","unknown",
/* 9E                 */ "SERVICE ACTION IN(16)",
/* 9F              M  */ "SERVICE ACTION OUT(16)",
};
#endif

/* The following are 12 byte commands in group 5 */
static const char *group_5_commands[] = {
/* a0-a5 */ unknown, unknown, unknown, unknown, unknown,
            "Move medium/play audio(12)",
/* a6-a9 */ "Exchange medium", unknown, "Read(12)", "Play track relative(12)",
/* aa-ae */ "Write(12)", unknown, "Erase(12)", unknown,
            "Write and verify(12)",
/* af-b1 */ "Verify(12)", "Search data high(12)", "Search data equal(12)",
/* b2-b4 */ "Search data low(12)", "Set limits(12)", unknown,
/* b5-b6 */ "Request volume element address", "Send volume tag",
/* b7-b9 */ "Read defect data(12)", "Read element status", unknown,
/* ba-bf */ unknown, unknown, unknown, unknown, unknown, unknown,
};

static const char **commands[] = {
    group_0_commands, group_1_commands, group_2_commands,
    (const char **) RESERVED_GROUP, (const char **) RESERVED_GROUP,
    group_5_commands, (const char **) VENDOR_GROUP,
    (const char **) VENDOR_GROUP
};

// The Statuses table contains the ASCII strings of what each status means
static const char * statuses[] = {
/* 0-4 */ "Good", "Check Condition", "Condition Met", unknown, "Busy",
/* 5-9 */ unknown, unknown, unknown, "Intermediate", unknown,
/* a-c */ "Intermediate-Condition Met", unknown, "Reservation Conflict",
/* d-10 */ unknown, unknown, unknown, unknown,
/* 11-14 */ "Command Terminated", unknown, unknown, "Queue Full",
/* 15-1a */ unknown, unknown, unknown,  unknown, unknown, unknown,
/* 1b-1f */ unknown, unknown, unknown,  unknown, unknown,
};
//******************************************************************************
// The following tables contain the ASCII strings for the most
// common status and sense code combinations. Only a few of
// these apply to disk storage devices but they are all here
// just in case. 
//
#define D 0x001  /* DIRECT ACCESS DEVICE (disk) */
#define T 0x002  /* SEQUENTIAL ACCESS DEVICE (tape) */
#define L 0x004  /* PRINTER DEVICE */
#define P 0x008  /* PROCESSOR DEVICE */
#define W 0x010  /* WRITE ONCE READ MULTIPLE DEVICE */
#define R 0x020  /* READ ONLY (CD-ROM) DEVICE */
#define S 0x040  /* SCANNER DEVICE */
#define O 0x080  /* OPTICAL MEMORY DEVICE */
#define M 0x100  /* MEDIA CHANGER DEVICE */
#define C 0x200  /* COMMUNICATION DEVICE */

struct error_info{
    unsigned char code1, code2;
    unsigned short int devices;
    const char * text;
};

struct error_info2{
    unsigned char code1, code2_min, code2_max;
    unsigned short int devices;
    const char * text;
};

static struct error_info2 additional2[] =
{
  {0x40,0x00,0x7f,D,"Ram failure (%x)"},
  {0x40,0x80,0xff,D|T|L|P|W|R|S|O|M|C,"Diagnostic failure on component (%x)"},
  {0x41,0x00,0xff,D,"Data path failure (%x)"},
  {0x42,0x00,0xff,D,"Power-on or self-test failure (%x)"},
  {0, 0, 0, 0, NULL}
};

static struct error_info additional[] =
{
  {0x00,0x01,T,"Filemark detected"},
  {0x00,0x02,T|S,"End-of-partition/medium detected"},
  {0x00,0x03,T,"Setmark detected"},
  {0x00,0x04,T|S,"Beginning-of-partition/medium detected"},
  {0x00,0x05,T|S,"End-of-data detected"},
  {0x00,0x06,D|T|L|P|W|R|S|O|M|C,"I/O process terminated"},
  {0x00,0x11,R,"Audio play operation in progress"},
  {0x00,0x12,R,"Audio play operation paused"},
  {0x00,0x13,R,"Audio play operation successfully completed"},
  {0x00,0x14,R,"Audio play operation stopped due to error"},
  {0x00,0x15,R,"No current audio status to return"},
  {0x01,0x00,D|W|O,"No index/sector signal"},
  {0x02,0x00,D|W|R|O|M,"No seek complete"},
  {0x03,0x00,D|T|L|W|S|O,"Peripheral device write fault"},
  {0x03,0x01,T,"No write current"},
  {0x03,0x02,T,"Excessive write errors"},
  {0x04,0x00,D|T|L|P|W|R|S|O|M|C,
     "Logical unit not ready, cause not reportable"},
  {0x04,0x01,D|T|L|P|W|R|S|O|M|C,
     "Logical unit is in process of becoming ready"},
  {0x04,0x02,D|T|L|P|W|R|S|O|M|C,
     "Logical unit not ready, initializing command required"},
  {0x04,0x03,D|T|L|P|W|R|S|O|M|C,
     "Logical unit not ready, manual intervention required"},
  {0x04,0x04,D|T|L|O,"Logical unit not ready, format in progress"},
  {0x05,0x00,D|T|L|W|R|S|O|M|C,"Logical unit does not respond to selection"},
  {0x06,0x00,D|W|R|O|M,"No reference position found"},
  {0x07,0x00,D|T|L|W|R|S|O|M,"Multiple peripheral devices selected"},
  {0x08,0x00,D|T|L|W|R|S|O|M|C,"Logical unit communication failure"},
  {0x08,0x01,D|T|L|W|R|S|O|M|C,"Logical unit communication time-out"},
  {0x08,0x02,D|T|L|W|R|S|O|M|C,"Logical unit communication parity error"},
  {0x09,0x00,D|T|W|R|O,"Track following error"},
  {0x09,0x01,W|R|O,"Tracking servo failure"},
  {0x09,0x02,W|R|O,"Focus servo failure"},
  {0x09,0x03,W|R|O,"Spindle servo failure"},
  {0x0A,0x00,D|T|L|P|W|R|S|O|M|C,"Error log overflow"},
  {0x0C,0x00,T|S,"Write error"},
  {0x0C,0x01,D|W|O,"Write error recovered with auto reallocation"},
  {0x0C,0x02,D|W|O,"Write error - auto reallocation failed"},
  {0x10,0x00,D|W|O,"Id crc or ecc error"},
  {0x11,0x00,D|T|W|R|S|O,"Unrecovered read error"},
  {0x11,0x01,D|T|W|S|O,"Read retries exhausted"},
  {0x11,0x02,D|T|W|S|O,"Error too long to correct"},
  {0x11,0x03,D|T|W|S|O,"Multiple read errors"},
  {0x11,0x04,D|W|O,"Unrecovered read error - auto reallocate failed"},
  {0x11,0x05,W|R|O,"L-ec uncorrectable error"},
  {0x11,0x06,W|R|O,"Circ unrecovered error"},
  {0x11,0x07,W|O,"Data resynchronization error"},
  {0x11,0x08,T,"Incomplete block read"},
  {0x11,0x09,T,"No gap found"},
  {0x11,0x0A,D|T|O,"Miscorrected error"},
  {0x11,0x0B,D|W|O,"Unrecovered read error - recommend reassignment"},
  {0x11,0x0C,D|W|O,"Unrecovered read error - recommend rewrite the data"},
  {0x12,0x00,D|W|O,"Address mark not found for id field"},
  {0x13,0x00,D|W|O,"Address mark not found for data field"},
  {0x14,0x00,D|T|L|W|R|S|O,"Recorded entity not found"},
  {0x14,0x01,D|T|W|R|O,"Record not found"},
  {0x14,0x02,T,"Filemark or setmark not found"},
  {0x14,0x03,T,"End-of-data not found"},
  {0x14,0x04,T,"Block sequence error"},
  {0x15,0x00,D|T|L|W|R|S|O|M,"Random positioning error"},
  {0x15,0x01,D|T|L|W|R|S|O|M,"Mechanical positioning error"},
  {0x15,0x02,D|T|W|R|O,"Positioning error detected by read of medium"},
  {0x16,0x00,D|W|O,"Data synchronization mark error"},
  {0x17,0x00,D|T|W|R|S|O,"Recovered data with no error correction applied"},
  {0x17,0x01,D|T|W|R|S|O,"Recovered data with retries"},
  {0x17,0x02,D|T|W|R|O,"Recovered data with positive head offset"},
  {0x17,0x03,D|T|W|R|O,"Recovered data with negative head offset"},
  {0x17,0x04,W|R|O,"Recovered data with retries and/or circ applied"},
  {0x17,0x05,D|W|R|O,"Recovered data using previous sector id"},
  {0x17,0x06,D|W|O,"Recovered data without ecc - data auto-reallocated"},
  {0x17,0x07,D|W|O,"Recovered data without ecc - recommend reassignment"},
  {0x18,0x00,D|T|W|R|O,"Recovered data with error correction applied"},
  {0x18,0x01,D|W|R|O,"Recovered data with error correction and retries applied"},
  {0x18,0x02,D|W|R|O,"Recovered data - data auto-reallocated"},
  {0x18,0x03,R,"Recovered data with circ"},
  {0x18,0x04,R,"Recovered data with lec"},
  {0x18,0x05,D|W|R|O,"Recovered data - recommend reassignment"},
  {0x19,0x00,D|O,"Defect list error"},
  {0x19,0x01,D|O,"Defect list not available"},
  {0x19,0x02,D|O,"Defect list error in primary list"},
  {0x19,0x03,D|O,"Defect list error in grown list"},
  {0x1A,0x00,D|T|L|P|W|R|S|O|M|C,"Parameter list length error"},
  {0x1B,0x00,D|T|L|P|W|R|S|O|M|C,"Synchronous data transfer error"},
  {0x1C,0x00,D|O,"Defect list not found"},
  {0x1C,0x01,D|O,"Primary defect list not found"},
  {0x1C,0x02,D|O,"Grown defect list not found"},
  {0x1D,0x00,D|W|O,"Miscompare during verify operation"},
  {0x1E,0x00,D|W|O,"Recovered id with ecc correction"},
  {0x20,0x00,D|T|L|P|W|R|S|O|M|C,"Invalid command operation code"},
  {0x21,0x00,D|T|W|R|O|M,"Logical block address out of range"},
  {0x21,0x01,M,"Invalid element address"},
  {0x22,0x00,D,"Illegal function (should use 20 00, 24 00, or 26 00)"},
  {0x24,0x00,D|T|L|P|W|R|S|O|M|C,"Invalid field in cdb"},
  {0x25,0x00,D|T|L|P|W|R|S|O|M|C,"Logical unit not supported"},
  {0x26,0x00,D|T|L|P|W|R|S|O|M|C,"Invalid field in parameter list"},
  {0x26,0x01,D|T|L|P|W|R|S|O|M|C,"Parameter not supported"},
  {0x26,0x02,D|T|L|P|W|R|S|O|M|C,"Parameter value invalid"},
  {0x26,0x03,D|T|L|P|W|R|S|O|M|C,"Threshold parameters not supported"},
  {0x27,0x00,D|T|W|O,"Write protected"},
  {0x28,0x00,D|T|L|P|W|R|S|O|M|C,"Not ready to ready transition (medium may have changed)"},
  {0x28,0x01,M,"Import or export element accessed"},
  {0x29,0x00,D|T|L|P|W|R|S|O|M|C,"Power on, reset, or bus device reset occurred"},
  {0x2A,0x00,D|T|L|W|R|S|O|M|C,"Parameters changed"},
  {0x2A,0x01,D|T|L|W|R|S|O|M|C,"Mode parameters changed"},
  {0x2A,0x02,D|T|L|W|R|S|O|M|C,"Log parameters changed"},
  {0x2B,0x00,D|T|L|P|W|R|S|O|C,"Copy cannot execute since host cannot disconnect"},
  {0x2C,0x00,D|T|L|P|W|R|S|O|M|C,"Command sequence error"},
  {0x2C,0x01,S,"Too many windows specified"},
  {0x2C,0x02,S,"Invalid combination of windows specified"},
  {0x2D,0x00,T,"Overwrite error on update in place"},
  {0x2F,0x00,D|T|L|P|W|R|S|O|M|C,"Commands cleared by another initiator"},
  {0x30,0x00,D|T|W|R|O|M,"Incompatible medium installed"},
  {0x30,0x01,D|T|W|R|O,"Cannot read medium - unknown format"},
  {0x30,0x02,D|T|W|R|O,"Cannot read medium - incompatible format"},
  {0x30,0x03,D|T,"Cleaning cartridge installed"},
  {0x31,0x00,D|T|W|O,"Medium format corrupted"},
  {0x31,0x01,D|L|O,"Format command failed"},
  {0x32,0x00,D|W|O,"No defect spare location available"},
  {0x32,0x01,D|W|O,"Defect list update failure"},
  {0x33,0x00,T,"Tape length error"},
  {0x36,0x00,L,"Ribbon, ink, or toner failure"},
  {0x37,0x00,D|T|L|W|R|S|O|M|C,"Rounded parameter"},
  {0x39,0x00,D|T|L|W|R|S|O|M|C,"Saving parameters not supported"},
  {0x3A,0x00,D|T|L|W|R|S|O|M,"Medium not present"},
  {0x3B,0x00,T|L,"Sequential positioning error"},
  {0x3B,0x01,T,"Tape position error at beginning-of-medium"},
  {0x3B,0x02,T,"Tape position error at end-of-medium"},
  {0x3B,0x03,L,"Tape or electronic vertical forms unit not ready"},
  {0x3B,0x04,L,"Slew failure"},
  {0x3B,0x05,L,"Paper jam"},
  {0x3B,0x06,L,"Failed to sense top-of-form"},
  {0x3B,0x07,L,"Failed to sense bottom-of-form"},
  {0x3B,0x08,T,"Reposition error"},
  {0x3B,0x09,S,"Read past end of medium"},
  {0x3B,0x0A,S,"Read past beginning of medium"},
  {0x3B,0x0B,S,"Position past end of medium"},
  {0x3B,0x0C,S,"Position past beginning of medium"},
  {0x3B,0x0D,M,"Medium destination element full"},
  {0x3B,0x0E,M,"Medium source element empty"},
  {0x3D,0x00,D|T|L|P|W|R|S|O|M|C,"Invalid bits in identify message"},
  {0x3E,0x00,D|T|L|P|W|R|S|O|M|C,"Logical unit has not self-configured yet"},
  {0x3F,0x00,D|T|L|P|W|R|S|O|M|C,"Target operating conditions have changed"},
  {0x3F,0x01,D|T|L|P|W|R|S|O|M|C,"Microcode has been changed"},
  {0x3F,0x02,D|T|L|P|W|R|S|O|M|C,"Changed operating definition"},
  {0x3F,0x03,D|T|L|P|W|R|S|O|M|C,"Inquiry data has changed"},
  {0x43,0x00,D|T|L|P|W|R|S|O|M|C,"Message error"},
  {0x44,0x00,D|T|L|P|W|R|S|O|M|C,"Internal target failure"},
  {0x45,0x00,D|T|L|P|W|R|S|O|M|C,"Select or reselect failure"},
  {0x46,0x00,D|T|L|P|W|R|S|O|M|C,"Unsuccessful soft reset"},
  {0x47,0x00,D|T|L|P|W|R|S|O|M|C,"Scsi parity error"},
  {0x48,0x00,D|T|L|P|W|R|S|O|M|C,"Initiator detected error message received"},
  {0x49,0x00,D|T|L|P|W|R|S|O|M|C,"Invalid message error"},
  {0x4A,0x00,D|T|L|P|W|R|S|O|M|C,"Command phase error"},
  {0x4B,0x00,D|T|L|P|W|R|S|O|M|C,"Data phase error"},
  {0x4C,0x00,D|T|L|P|W|R|S|O|M|C,"Logical unit failed self-configuration"},
  {0x4E,0x00,D|T|L|P|W|R|S|O|M|C,"Overlapped commands attempted"},
  {0x50,0x00,T,"Write append error"},
  {0x50,0x01,T,"Write append position error"},
  {0x50,0x02,T,"Position error related to timing"},
  {0x51,0x00,T|O,"Erase failure"},
  {0x52,0x00,T,"Cartridge fault"},
  {0x53,0x00,D|T|L|W|R|S|O|M,"Media load or eject failed"},
  {0x53,0x01,T,"Unload tape failure"},
  {0x53,0x02,D|T|W|R|O|M,"Medium removal prevented"},
  {0x54,0x00,P,"Scsi to host system interface failure"},
  {0x55,0x00,P,"System resource failure"},
  {0x57,0x00,R,"Unable to recover table-of-contents"},
  {0x58,0x00,O,"Generation does not exist"},
  {0x59,0x00,O,"Updated block read"},
  {0x5A,0x00,D|T|L|P|W|R|S|O|M,"Operator request or state change input (unspecified)"},
  {0x5A,0x01,D|T|W|R|O|M,"Operator medium removal request"},
  {0x5A,0x02,D|T|W|O,"Operator selected write protect"},
  {0x5A,0x03,D|T|W|O,"Operator selected write permit"},
  {0x5B,0x00,D|T|L|P|W|R|S|O|M,"Log exception"},
  {0x5B,0x01,D|T|L|P|W|R|S|O|M,"Threshold condition met"},
  {0x5B,0x02,D|T|L|P|W|R|S|O|M,"Log counter at maximum"},
  {0x5B,0x03,D|T|L|P|W|R|S|O|M,"Log list codes exhausted"},
  {0x5C,0x00,D|O,"Rpl status change"},
  {0x5C,0x01,D|O,"Spindles synchronized"},
  {0x5C,0x02,D|O,"Spindles not synchronized"},
  {0x60,0x00,S,"Lamp failure"},
  {0x61,0x00,S,"Video acquisition error"},
  {0x61,0x01,S,"Unable to acquire video"},
  {0x61,0x02,S,"Out of focus"},
  {0x62,0x00,S,"Scan head positioning error"},
  {0x63,0x00,R,"End of user area encountered on this track"},
  {0x64,0x00,R,"Illegal mode for this track"},
  {0, 0, 0, NULL}
};

static const char *snstext[] = {
    "None",                     /* There is no sense information */
    "Recovered Error",          /* The last command completed successfully
                                   but used error correction */
    "Not Ready",                /* The addressed target is not ready */
    "Medium Error",             /* Data error detected on the medium */
    "Hardware Error",           /* Controller or device failure */
    "Illegal Request",
    "Unit Attention",           /* Removable medium was changed, or
                                   the target has been reset */
    "Data Protect",             /* Access to the data is blocked */
    "Blank Check",              /* Reached unexpected written or unwritten
                                   region of the medium */
    "Key=9",                    /* Vendor specific */
    "Copy Aborted",             /* COPY or COMPARE was aborted */
    "Aborted Command",          /* The target aborted the command */
    "Equal",                    /* A SEARCH DATA command found data equal */
    "Volume Overflow",          /* Medium full with still data to be written */
    "Miscompare",               /* Source data and data on the medium
                                   do not agree */
    "Key=15"                    /* Reserved */
};
 
static const char * hostbyte_table[]={
"DID_OK", "DID_NO_CONNECT", "DID_BUS_BUSY", "DID_TIME_OUT", "DID_BAD_TARGET",
"DID_ABORT", "DID_PARITY", "DID_ERROR", "DID_RESET", "DID_BAD_INTR",
"DID_PASSTHROUGH", "DID_SOFT_ERROR", NULL};
 
static const char * driverbyte_table[]={
"DRIVER_OK", "DRIVER_BUSY", "DRIVER_SOFT",  "DRIVER_MEDIA", "DRIVER_ERROR",
"DRIVER_INVALID", "DRIVER_TIMEOUT", "DRIVER_HARD", "DRIVER_SENSE", NULL};

static const char * driversuggest_table[]={"SUGGEST_OK",
"SUGGEST_RETRY", "SUGGEST_ABORT", "SUGGEST_REMAP", "SUGGEST_DIE",
unknown,unknown,unknown, "SUGGEST_SENSE",NULL}; 
 
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
