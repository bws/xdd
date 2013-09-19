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
