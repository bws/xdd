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

// ------------------ Throttle stuff --------------------------------------------------
// The following structure is used by the -throttle option
struct xint_throttle {
		double				throttle;  			// Target Throttle assignments 
		double				throttle_variance;  // Throttle Bandwidth variance: +-x.x MB/sec
		uint32_t      		throttle_type; 		// Target Throttle type 
#define XINT_THROTTLE_OPS   0x00000001  		// Throttle type of OPS 
#define XINT_THROTTLE_BW    0x00000002  		// Throttle type of Bandwidth 
#define XINT_THROTTLE_ABW   0x00000004  		// Throttle type of Average Bandwidth 
#define XINT_THROTTLE_DELAY 0x00000008  		// Throttle type of a constant delay or time for each op 
};

#define XINT_DEFAULT_THROTTLE   		1.0					// Default Throttle
#define XINT_DEFAULT_THROTTLE_VARIANCE	0.0					// Default Throttle Variance
#define XINT_DEFAULT_THROTTLE_TYPE		XINT_THROTTLE_BW	// Default Throttle type 

typedef struct xint_throttle xint_throttle_t;
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
