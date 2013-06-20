/* Copyright (C) 1992-2011 I/O Performance, Inc. and the
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
#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <stdint.h>
#include <arpa/inet.h>

#define HOSTNAMELENGTH 1024

// Flags for xdd_lookup_addr()
#define XDD_LOOKUP_IGNORE_CACHE  (1 << 0) // Neither read from nor write to the cache when performing a lookup

/**
 * Convert from network byte-order (big endian) to host order
 */
inline uint64_t ntohll(uint64_t value)
{
    int endian_test = 1;

    // Determine if host order is little endian
    if (endian_test == *((char*)(&endian_test))) {
	// Swap the bytes
	uint32_t low = ntohl((uint32_t)(value & 0xFFFFFFFFLL));
	uint32_t high = ntohl((uint32_t)(value >> 32));
	value = ((uint64_t)(low) << 32) | (uint64_t)(high);
    }
    return value;
}

/**
 * Convert from host byte-order to network byte-order (big endian)
 */
inline uint64_t htonll(uint64_t value)
{
    // Re-use the htonll implementation to swap the bytes
    return htonll(value);
}

#endif /* NET_UTILS_H */
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
