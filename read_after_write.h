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
 */

/**
 * @author Tom Ruwart (tmruwart@ioperformance.com)
 * @author I/O Performance, Inc.
 *
 * @file read_after_write.h
 * Defines the read-after-write data structure.
 */

/** Messaging structure for read-after-write processes */
struct xdd_raw_msg {
	uint32_t magic;  /**< Magic number */
	uint32_t filler; 
	int64_t  sequence; /**< Sequence number */
	pclk_t  sendtime; /**< Time this packet was sent in global pico seconds */
	pclk_t  recvtime; /**< Time this packet was received in global pico seconds */
	int64_t  location; /**< Starting location in bytes for this operation */
	int64_t  length;  /**< Length in bytes this operation */
}; 
typedef struct xdd_raw_msg xdd_raw_msg_t;

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vim: ts=8 sts=8 sw=8 noexpandtab
 */
