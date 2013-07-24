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
#ifndef XDD_TYPES_H
#define XDD_TYPES_H

enum xdd_target_type {
	XDD_NULL_TARGET_TYPE = 0,
	XDD_IN_TARGET_TYPE,
	XDD_OUT_TARGET_TYPE,
	XDD_META_TARGET_TYPE
};
/*! \brief XDD target types. */
typedef enum xdd_target_type xdd_target_type_t;

struct xdd_target_attributes;
/*! \brief XDD target attributes. */
typedef struct xdd_target_attributes* xdd_targetattr_t;

struct xdd_target;
/*! \brief Opaque type representing an XDD target. */
typedef struct xdd_target* xdd_target_t;

struct xdd_plan_attributes;
/*! \brief Opaque type representing an XDD paln. */
typedef struct xdd_plan_attributes* xdd_planattr_t; 

struct xdd_plan_pub;
/*! \brief Opaque type representing an XDD paln. */
typedef struct xdd_plan_pub* xdd_planpub_t; 

#endif
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
