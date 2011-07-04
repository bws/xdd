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
/*
 * This file implements a resolver cache so that lookups for the same
 * name return the same address within a given XDD process. Space for
 * the cache is is statically allocated and can contain at most
 * MAX_ADDR_CACHE_ENTRIES entries.
 */
#include "xdd.h"

#define MAX_ADDR_CACHE_ENTRIES (E2E_ADDRESS_TABLE_ENTRIES*16)  // The amount of space statically allocated in this process for the address cache

// Static variables that implement a process-wide address cache
static pthread_mutex_t addr_cache_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex to protect access to addr_cache_entries and addr_cache_entry
static int addr_cache_entries = 0;  // The number of entries present in the cache
static struct {
	char name[HOSTNAMELENGTH];  // Hostname
	struct xdd_network_address addr;  // Address
} addr_cache_entry[MAX_ADDR_CACHE_ENTRIES];  // The address cache implementation as an array of (name, address) pairs

/*----------------------------------------------------------------------*/
/* xdd_lookup_addr() - resolve the address for a host, possibly using the cache
 * 
 * This subroutine will translate a hostname to an address using the
 * system resolver library. If the name has been previously resolved
 * then the subroutine can optionally return a cached result. The
 * purpose of the cache is primarily to defeat DNS round-robin
 * schemes. It has the added advantage of minimizing the number of DNS
 * query packets transmitted over the network. Use of the cache can be
 * disabled on a per-request basis using the XDD_LOOKUP_IGNORE_CACHE
 * flag.
 *
 * The `name' argument can contain any of the node names recognized by
 * the getaddrinfo(3) function.
 *
 * Function arguments:
 *     - name, the hostname of the machine to lookup
 *     - addrtype, the type of address to return (XDD_ADDRESS_INET4 or XDD_ADDRESS_INET6)
 *     - flags, a bitmask of options controlling the lookup
 *     - result, the location where the address will be stored upon successful return
 *
 * The size of the cache is fixed at compile-time. Entries in the
 * cache never expire and are never replaced. If the cache is too
 * small to contain a new entry then a warning message is displayed
 * and the new entry is not cached. The cache may contain two entries
 * with the same name provided the entries have different address
 * types.
 * 
 * Return values: 0 is good, -1 is bad
 */
int32_t
xdd_lookup_addr(const char *name, int addrtype, uint32_t flags,
				struct xdd_network_address *result)
{
	struct xdd_network_address addr;
	int hit = 0, found = 0;
	struct addrinfo hints;
	struct addrinfo *addrinfo = NULL;
	int status;
	int i;

	assert(addrtype == XDD_ADDRESS_INET4 || addrtype == XDD_ADDRESS_INET6);

	memset(&addr, 0, sizeof(addr));

	// Lock out the entire function to prevent race conditions
	pthread_mutex_lock(&addr_cache_mutex);

	// Search the cache first if this feature has not been disabled
	if (!(flags & XDD_LOOKUP_IGNORE_CACHE)) {
		for (i = 0; i < addr_cache_entries && !found; i++) {
			if (addr_cache_entry[i].addr.type == addrtype &&
				!strcmp(addr_cache_entry[i].name, name)) {
				addr = addr_cache_entry[i].addr;
				hit = found = 1;
			}
		}
	}

	// Query the resolver library if the address was not found
	if (!found) {
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = addrtype;  // Rely on the XDD_ADDRESS_* constants mapping to AF_*
		status = getaddrinfo(name, NULL, &hints, &addrinfo);
		if (status) {
			fprintf(xgp->errout, "%s: xdd_lookup_addr: status %d: %s\n",
					xgp->progname, status, gai_strerror(status));
		} else {
			assert(addrinfo->ai_family == addrtype);
			assert(addrinfo->ai_addrlen <= sizeof(addr.u));
			addr.type = addrtype;
			memcpy(&addr.u, addrinfo->ai_addr, 
				   MIN(addrinfo->ai_addrlen, sizeof(addr.u)));  // XXX: is this correct?
			freeaddrinfo(addrinfo);
			addrinfo = NULL;
			found = 1;
		}
	}

	// If the address was found insert an entry into the cache if this feature has not been disabled
	if (!hit && found && !(flags & XDD_LOOKUP_IGNORE_CACHE)) {
		if (addr_cache_entries < MAX_ADDR_CACHE_ENTRIES) {
			char *n = addr_cache_entry[addr_cache_entries].name;
			size_t nsz = sizeof(addr_cache_entry[addr_cache_entries].name);

			strncpy(n, name, nsz);
			addr_cache_entry[addr_cache_entries].addr = addr;
			if (n[nsz-1] != '\0') {
				fprintf(xgp->errout, 
						"%s: xdd_lookup_addr: name '%s' too large to fit into address cache\n",
						xgp->progname, name);
			} else {
				addr_cache_entries += 1;
			}
		}
	}

	pthread_mutex_unlock(&addr_cache_mutex);

	if (found) {
		*result = addr;
	}

	return(found ? 0 : -1);
}

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
