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
 *       Steve Hodson, DoE/ORNL
 *       Steve Poole, DoE/ORNL
 *       Bradly Settlemyer, DoE/ORNL
 *       Russell Cattelan, Digital Elves
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 *
 * Large parts of this file was a huge cut&paste hack from linux/drivers/scsi/constant.c
 *  which was presumably written by:
 *       Copyright (C) 1993, 1994, 1995 Eric Youngdale
 *       Copyright (C) 1999 - 2001 D. Gilbert
 */
/*
 * This file contains the subroutines necessary to support SCSI Generic I/O under LINUX.
 */
#include "xdd.h"
#if LINUX
#include "sg.h"
// #define SG_DEBUG

#define READ_CAP_REPLY_LEN 8
#define DEF_TIMEOUT 40000       /* 40,000 millisecs == 40 seconds */


//******************************************************************************
// SCSI Generic Support Routines
//******************************************************************************
/*----------------------------------------------------------------------------*/
/* xdd_sg_io() - Perform a "read" or "write" operation on the specified target
 * Will return a -1 if the command fails, 0 for EOF, or the number of 
 * bytes transferred if everything works. 
 * This ruotine takes two parameters:
 *   - Pointer to the PTDS of this target
 *   - A character that is either 'r' or 'w' to indicate a 'read' or 'write' 
 *       operation respectively.
 */
int32_t 
xdd_sg_io(ptds_t *p, char rw) {
	unsigned char 	Cmd[16];		// This is defined as a 16-byte CDB 
	sg_io_hdr_t 	io_hdr;
	int 			status;			// This is the status from the SG driver
	int 			io_status;		// This is the status from the device itself
	int64_t			last_sector;	// Last sector in range of sectors to transfer

	// Set up the sg-specific variables in the PTDS
	p->sg_blocksize = 512; // This is because sg uses a sector size block size
	p->sg_from_block = (p->my_current_byte_location / p->sg_blocksize);
	p->sg_blocks = p->actual_iosize / p->sg_blocksize;
	
	// Init the CDB
	if (rw == 'w') 
		 Cmd[0] = WRITE_16;
	else Cmd[0] = READ_16; // Assume Read
	Cmd[1] = 0;
	// Starting sector - bytes 2-9 - 8-bytes
	Cmd[2] = (unsigned char)((p->sg_from_block >> 56) & 0xFF);
	Cmd[3] = (unsigned char)((p->sg_from_block >> 48) & 0xFF);
	Cmd[4] = (unsigned char)((p->sg_from_block >> 40) & 0xFF);
	Cmd[5] = (unsigned char)((p->sg_from_block >> 32) & 0xFF);
	Cmd[6] = (unsigned char)((p->sg_from_block >> 24) & 0xFF);
	Cmd[7] = (unsigned char)((p->sg_from_block >> 16) & 0xFF);
	Cmd[8] = (unsigned char)((p->sg_from_block >> 8) & 0xFF);
	Cmd[9] = (unsigned char)(p->sg_from_block & 0xFF);
	// Transfer Length - bytes 10-13 - 4-bytes
	Cmd[10] = (unsigned char)((p->sg_blocks >> 24) & 0xff);
	Cmd[11] = (unsigned char)((p->sg_blocks >> 16) & 0xff);
	Cmd[12] = (unsigned char)((p->sg_blocks >> 8) & 0xff);
	Cmd[13] = (unsigned char)(p->sg_blocks & 0xff);
	// MMC-4, and group number - NA
	Cmd[14] = 0;
	// Control 
	Cmd[15] = 0;

	// Init the IO Header that is used by the SG driver
	memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
	io_hdr.interface_id = 'S';
	io_hdr.cmd_len = sizeof(Cmd);
	io_hdr.cmdp = Cmd;
	if (rw == 'w') 
		io_hdr.dxfer_direction = SG_DXFER_TO_DEV; // Write op
	else io_hdr.dxfer_direction = SG_DXFER_FROM_DEV; // Read op
	io_hdr.dxfer_len = p->sg_blocksize * p->sg_blocks;
	io_hdr.dxferp = p->rwbuf;
	io_hdr.mx_sb_len = SENSE_BUFF_LEN;
	io_hdr.sbp = p->sg_sense;
	io_hdr.timeout = DEF_TIMEOUT;
	io_hdr.pack_id = 0;
	io_hdr.flags |= SG_FLAG_DIRECT_IO;

	// This "write" command will send the IO Header and CDB to the SG device Driver
	// which will then send the CDB to the actual device
	errno = 0;
	status = write(p->fd, &io_hdr, sizeof(io_hdr));
	while ((status  < 0) && (EINTR == errno)) {
		status = write(p->fd, &io_hdr, sizeof(io_hdr));
	}
	// Check status of sending the IO Header to the SG driver
	if (status < 0) {
		fprintf(xgp->errout, "%s:(T%d.Q%d): Error sending IO Header and CDB to SG Driver for a %s Command on target %s - status %d, op# %lld\n",
			xgp->progname,
			p->my_target_number,
			p->my_qthread_number,
			(rw == 'w')?"Write":"Read",
			p->target,
			status,
			(long long)p->my_current_op);
		fflush(xgp->errout);
		return(status);
	}

	// Read/block on the return status of the actual SCSI command
	errno = 0;
	status = read(p->fd, &io_hdr, sizeof(io_hdr));
	while ((status < 0) && (EINTR == errno)) {
		status = read(p->fd, &io_hdr, sizeof(io_hdr));
	}

	// Check status of sending the IO Header to the SG driver
	io_status = sg_chk_n_print3("xdd: SG Sense", &io_hdr, stderr);
	if ((status < 0) || (io_status == 0)) { // There was an error....
		fprintf(xgp->errout, "%s (T%d.Q%d): SG I/O Error for %s Command on target %s - status %d, op# %lld, from sector# %llu for %d sectors\n",
			xgp->progname,
			p->my_target_number,
			p->my_qthread_number,
			(rw == 'w')?"Write":"Read",
			p->target,
			status,
			(long long)p->my_current_op,
			(unsigned long long)p->sg_from_block, 
			p->sg_blocks);
		fflush(xgp->errout);

		// Check the type of error and print out the sense information for this error if there was an error
		io_status = sg_err_category3(&io_hdr);
		switch (io_status) {
			case SG_ERR_CAT_RECOVERED:
				fprintf(xgp->errout, "%s (T%d.Q%d): Recovered %s Error on target %s - status %d, op# %lld, from sector# %llu for %d sectors\n",
					xgp->progname,
					p->my_target_number,
					p->my_qthread_number,
					(rw == 'w')?"Write":"Read",
					p->target,
					status,
					(long long)p->my_current_op,
					(unsigned long long)p->sg_from_block, 
					p->sg_blocks);
				break;
			default:
				status = xdd_sg_read_capacity(p); 
				if (status == SUCCESS) { // Check for an out-of-bounds condition
					last_sector = p->sg_from_block + p->sg_blocks - 1;
					if ( last_sector > p->sg_num_sectors) { // LBA out of range error most likely
						fprintf(xgp->errout, "%s (T%d.Q%d): Attempting to access a sector that is PAST the END of the device: op# %llu, from sector# %lld, for %d sectors\n",
							xgp->progname,
							p->my_target_number,
							p->my_qthread_number,
							(long long)p->my_current_op,
							(unsigned long long)p->sg_from_block, 
							p->sg_blocks);
					}
				} // Done checking for out-of-range error
				fprintf(xgp->errout, "%s (T%d.Q%d): %s Error on target %s - status %d, op# %lld, from sector# %llu for %d sectors\n",
					xgp->progname,
					p->my_target_number,
					p->my_qthread_number,
					(rw == 'w')?"Write":"Read",
					p->target,
					status,
					(long long)p->my_current_op,
					(unsigned long long)p->sg_from_block, 
					p->sg_blocks);
				return(0);
		
		} // End of SWITCH stmnt

	} // End of looking at SG driver errors

	// No error - return the amount of data that was transferred
	return(p->sg_blocksize*p->sg_blocks);

} // End of xdd_sg_io() 

/*----------------------------------------------------------------------------*/
/* xdd_sg_read_capacity() - Issue a "Read Capacity" SCSI command to the target
 * and store the results in the associated PTDS
 * Will return SUCCESS or FAILED depending on the outcome of the command.
 */
int32_t 
xdd_sg_read_capacity(ptds_t *p) {
	int 			status;
	unsigned char	rcCmd[10];
	unsigned char	rcBuff[READ_CAP_REPLY_LEN];
	sg_io_hdr_t 	io_hdr;

	rcCmd[0] = 0x25;
	rcCmd[1] = 0;
	rcCmd[2] = 0;
	rcCmd[3] = 0;
	rcCmd[4] = 0;
	rcCmd[5] = 0;
	rcCmd[6] = 0;
	rcCmd[7] = 0;
	rcCmd[8] = 0;
	rcCmd[9] = 0;
	
	memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
	io_hdr.interface_id = 'S';
	io_hdr.cmd_len = sizeof(rcCmd);
	io_hdr.mx_sb_len = SENSE_BUFF_LEN;
	io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
	io_hdr.dxfer_len = sizeof(rcBuff);
	io_hdr.dxferp = rcBuff;
	io_hdr.cmdp = rcCmd;
	io_hdr.sbp = p->sg_sense;
	io_hdr.timeout = DEF_TIMEOUT;

	status = ioctl(p->fd, SG_IO, &io_hdr);
	if (status < 0) {
		fprintf(xgp->errout, "(T%d.Q%d) %s: I/O Error - Read Capacity Command on target %s - status %d, op# %lld\n",
			p->my_target_number,
			p->my_qthread_number,
			xgp->progname,
			p->target,status,
			(long long)p->my_current_op);
		fflush(xgp->errout);
		perror("reason");
		return(FAILED);
	}
	status = sg_err_category3(&io_hdr);
	if (SG_ERR_CAT_MEDIA_CHANGED == status)
		return(FAILED); 
	else if (SG_ERR_CAT_CLEAN != status) {
		fprintf(stderr,"read capacity error");
		return(FAILED); 
	}

	// Retrieve the number of sectors and the sector size from the buffer
	p->sg_num_sectors = 1 + ((rcBuff[0] << 24) | (rcBuff[1] << 16) | (rcBuff[2] << 8) | rcBuff[3]);
	p->sg_sector_size = (rcBuff[4] << 24) | (rcBuff[5] << 16) | (rcBuff[6] << 8) | rcBuff[7];

	return(SUCCESS);

} // End of xdd_sg_read_capacity() 

/*----------------------------------------------------------------------------*/
/* xdd_sg_set_reserved_size() - issued after open  - called from initialization.c
 */
void
xdd_sg_set_reserved_size(ptds_t *p, int fd) {
	int		reserved_size;
	int		status;


	reserved_size = (p->block_size*p->reqsize);
	status = ioctl(fd, SG_SET_RESERVED_SIZE, &reserved_size);
	if (status < 0) {
		fprintf(xgp->errout,"%s: xdd_sg_set_reserved_size: SG_SET_RESERVED_SIZE error - request for %d bytes denied",
			xgp->progname, 
			(p->block_size*p->reqsize));
	}
} // End of xdd_sg_set_reserved_size() 

/*----------------------------------------------------------------------------*/
/* xdd_sg_get_version() - issued after open  - get the current version of SG
 */
void
xdd_sg_get_version(ptds_t *p, int fd) {
	int 	version;
	int		status;


	status = ioctl(fd, SG_GET_VERSION_NUM, &version);
	if ((status < 0) || (version < 30000)) {
		fprintf(xgp->errout, "%s: xdd_sg_get_reserved_size: sg driver prior to 3.x.y - specifically %d\n",
			xgp->progname,
			version);
	}
} // End of xdd_sg_get_version()

/*----------------------------------------------------------------------------*/
/* sg_print_opcode() 
 */
static void 
print_opcode(int opcode, FILE *outp) {

    const char **table = commands[ group(opcode) ];
    switch ((unsigned long) table) {
    case RESERVED_GROUP:
        fprintf(outp, "%s(0x%02x) ", reserved, opcode);
        break;
    case VENDOR_GROUP:
        fprintf(outp, "%s(0x%02x) ", vendor, opcode);
        break;
    default:
        if (table[opcode & 0x1f] != unknown)
            fprintf(outp, "%s ",table[opcode & 0x1f]);
        else
            fprintf(outp, "%s(0x%02x) ", unknown, opcode);
        break;
    }
} // End of sg_print_opcode()

/*----------------------------------------------------------------------------*/
/* sg_print_command() 
 */
void 
sg_print_command (const unsigned char * command, FILE *outp) {
    int i,s;
    print_opcode(command[0], outp);
    for ( i = 1, s = COMMAND_SIZE(command[0]); i < s; ++i)
        fprintf(outp, "%02x ", command[i]);
    fprintf(outp, "\n");
} // End of sg_print_command()

/*----------------------------------------------------------------------------*/
/* sg_print_status() 
 */
void 
sg_print_status (int masked_status, FILE *outp) {
    /* status = (status >> 1) & 0xf; */ /* already done */
    fprintf(outp, "%s ",statuses[masked_status]);
} // End of sg_print_status()

/*----------------------------------------------------------------------------*/
/* sg_print_sense() - Print sense information
 */
void 
sg_print_sense(const char * leadin, 
					const unsigned char * sense_buffer,
                    int sb_len, 
					FILE *outp)
{
    int i, s;
    int sense_class, valid, code;
    const char * error = NULL;

    sense_class = (sense_buffer[0] >> 4) & 0x07;
    code = sense_buffer[0] & 0xf;
    valid = sense_buffer[0] & 0x80;

    if (sense_class == 7) {     /* extended sense data */
        s = sense_buffer[7] + 8;
        if(s > sb_len)
           s = sb_len;

        if (!valid)
            fprintf(outp, "[valid=0] ");
        fprintf(outp, "Info fld=0x%x, ", (int)((sense_buffer[3] << 24) |
                (sense_buffer[4] << 16) | (sense_buffer[5] << 8) |
                sense_buffer[6]));

        if (sense_buffer[2] & 0x80)
           fprintf(outp, "FMK ");     /* current command has read a filemark */
        if (sense_buffer[2] & 0x40)
           fprintf(outp, "EOM ");     /* end-of-medium condition exists */
        if (sense_buffer[2] & 0x20)
           fprintf(outp, "ILI ");     /* incorrect block length requested */

        switch (code) {
        case 0x0:
            error = "Current";  /* error concerns current command */
            break;
        case 0x1:
            error = "Deferred"; /* error concerns some earlier command */
                /* e.g., an earlier write to disk cache succeeded, but
                   now the disk discovers that it cannot write the data */
            break;
        default:
            error = "Invalid";
        }

        fprintf(outp, "%s ", error);

        if (leadin)
            fprintf(outp, "%s: ", leadin);
        fprintf(outp, "sense key: %s\n", snstext[sense_buffer[2] & 0x0f]);

        /* Check to see if additional sense information is available */
        if(sense_buffer[7] + 7 < 13 ||
           (sense_buffer[12] == 0  && sense_buffer[13] ==  0)) goto done;

        for(i=0; additional[i].text; i++)
            if(additional[i].code1 == sense_buffer[12] &&
               additional[i].code2 == sense_buffer[13])
                fprintf(outp, "Additional sense indicates: %s\n",
                        additional[i].text);

        for(i=0; additional2[i].text; i++)
            if(additional2[i].code1 == sense_buffer[12] &&
               additional2[i].code2_min >= sense_buffer[13]  &&
               additional2[i].code2_max <= sense_buffer[13]) {
                fprintf(outp, "Additional sense indicates: ");
                fprintf(outp, additional2[i].text, sense_buffer[13]);
                fprintf(outp, "\n");
            };
    } else {    /* non-extended sense data */

         /*
          * Standard says:
          *    sense_buffer[0] & 0200 : address valid
          *    sense_buffer[0] & 0177 : vendor-specific error code
          *    sense_buffer[1] & 0340 : vendor-specific
          *    sense_buffer[1..3] : 21-bit logical block address
          */

        if (leadin)
            fprintf(outp, "%s: ", leadin);
        if (sense_buffer[0] < 15)
            fprintf(outp, 
	    	    "old sense: key %s\n", snstext[sense_buffer[0] & 0x0f]);
        else
            fprintf(outp, "sns = %2x %2x\n", sense_buffer[0], sense_buffer[2]);

        fprintf(outp, "Non-extended sense class %d code 0x%0x ", 
		sense_class, code);
        s = 4;
    }

 done:
    fprintf(outp, "Raw sense data (in hex):\n  ");
    for (i = 0; i < s; ++i) {
        if ((i > 0) && (0 == (i % 24)))
            fprintf(outp, "\n  ");
        fprintf(outp, "%02x ", sense_buffer[i]);
    }
    fprintf(outp, "\n");
    return;
} // End of sg_print_sense()

/*----------------------------------------------------------------------------*/
/* sg_print_host_status()
 */
void 
sg_print_host_status(int host_status, FILE *outp)
{   static int maxcode=0;
    int i;

    if(! maxcode) {
        for(i = 0; hostbyte_table[i]; i++) ;
        maxcode = i-1;
    }
    fprintf(outp, "Host_status=0x%02x", host_status);
    if(host_status > maxcode) {
        fprintf(outp, "is invalid ");
        return;
    }
    fprintf(outp, "(%s) ",hostbyte_table[host_status]);
} // End of sg_print_host_status() 

/*----------------------------------------------------------------------------*/
/* sg_print_driver_status()
 */
void 
sg_print_driver_status(int driver_status, FILE *outp)
{
    static int driver_max =0 , suggest_max=0;
    int i;
    int dr = driver_status & SG_ERR_DRIVER_MASK;
    int su = (driver_status & SG_ERR_SUGGEST_MASK) >> 4;

    if(! driver_max) {
        for(i = 0; driverbyte_table[i]; i++) ;
        driver_max = i;
        for(i = 0; driversuggest_table[i]; i++) ;
        suggest_max = i;
    }
    fprintf(outp, "Driver_status=0x%02x",driver_status);
    fprintf(outp, " (%s,%s) ",
            dr < driver_max  ? driverbyte_table[dr]:"invalid",
            su < suggest_max ? driversuggest_table[su]:"invalid");
} // End of sg_print_driver_status() 

/*----------------------------------------------------------------------------*/
/* sg_chk_n_print3() 
 */
int 
sg_chk_n_print3(const char *leadin, 
				struct sg_io_hdr *hp, 
				FILE *outp) 
{
    return sg_chk_n_print(leadin, hp->masked_status, hp->host_status,
                          hp->driver_status, hp->sbp, hp->sb_len_wr, outp);
} // End of sg_chk_n_print3() 

/*----------------------------------------------------------------------------*/
/* sg_chk_n_print() 
 */
int 
sg_chk_n_print(const char * leadin, 
				int masked_status,
				int host_status, 
				int driver_status,
				const unsigned char * sense_buffer, 
				int sb_len,
				FILE *outp) 
{

    int done_leadin = 0;
    int done_sense = 0;

    if ((0 == masked_status) && (0 == host_status) &&
        (0 == driver_status))
        return 1;       /* No problems */
    if (0 != masked_status) {
        if (leadin)
            fprintf(outp, "%s: ", leadin);
        done_leadin = 1;
        sg_print_status(masked_status, outp);
        fprintf(outp, "\n");
        if (sense_buffer && ((masked_status == CHECK_CONDITION) ||
                             (masked_status == COMMAND_TERMINATED))) {
            sg_print_sense(0, sense_buffer, sb_len, outp);
            done_sense = 1;
        }
    }
    if (0 != host_status) {
        if (leadin && (! done_leadin))
            fprintf(outp, "%s: ", leadin);
        if (done_leadin)
            fprintf(outp, "plus...: ");
        else
            done_leadin = 1;
        sg_print_host_status(host_status, outp);
        fprintf(outp, "\n");
    }
    if (0 != driver_status) {
        if (leadin && (! done_leadin))
            fprintf(outp, "%s: ", leadin);
        if (done_leadin)
            fprintf(outp, "plus...: ");
        else
            done_leadin = 1;
        sg_print_driver_status(driver_status, outp);
        fprintf(outp, "\n");
        if (sense_buffer && (! done_sense) &&
            (SG_ERR_DRIVER_SENSE & driver_status))
            sg_print_sense(0, sense_buffer, sb_len, outp);
    }
    return 0;
} // End of sg_chk_n_print() 

/*----------------------------------------------------------------------------*/
/* sg_err_category3() 
 */
int 
sg_err_category3(struct sg_io_hdr * hp) {
	return sg_err_category(hp->masked_status, 
							hp->host_status,
							hp->driver_status, 
							hp->sbp, 
							hp->sb_len_wr);
} // End of sg_err_category3() 

/*----------------------------------------------------------------------------*/
/* sg_err_category() 
 */
int 
sg_err_category(int masked_status,
					int host_status,
					int driver_status, 
					const unsigned char * sense_buffer,
					int sb_len)
{

    if ((0 == masked_status) && (0 == host_status) &&
        (0 == driver_status))
        return SG_ERR_CAT_CLEAN;
    if ((CHECK_CONDITION == masked_status) ||
        (COMMAND_TERMINATED == masked_status) ||
        (SG_ERR_DRIVER_SENSE & driver_status)) {
        if (sense_buffer && (sb_len > 2)) {
            if(RECOVERED_ERROR == sense_buffer[2])
                return SG_ERR_CAT_RECOVERED;
            else if ((UNIT_ATTENTION == (0x0f & sense_buffer[2])) &&
                     (sb_len > 12)) {
                if (0x28 == sense_buffer[12])
                    return SG_ERR_CAT_MEDIA_CHANGED;
                if (0x29 == sense_buffer[12])
                    return SG_ERR_CAT_RESET;
            }
        }
        return SG_ERR_CAT_SENSE;
    }
    if (0 != host_status) {
        if ((SG_ERR_DID_NO_CONNECT == host_status) ||
            (SG_ERR_DID_BUS_BUSY == host_status) ||
            (SG_ERR_DID_TIME_OUT == host_status))
            return SG_ERR_CAT_TIMEOUT;
    }
    if (0 != driver_status) {
        if (SG_ERR_DRIVER_TIMEOUT == driver_status)
            return SG_ERR_CAT_TIMEOUT;
    }
    return SG_ERR_CAT_OTHER;
} // End of sg_err_category() 
#endif
 
