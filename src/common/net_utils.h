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
static inline uint64_t ntohll(uint64_t value)
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
static inline uint64_t htonll(uint64_t value)
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
