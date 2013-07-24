
#include "libxdd.h"
#include <assert.h>
#include <string.h>
#define XDDMAIN
#include "xdd_types_internal.h"
#include "xint.h"

int xdd_targetattr_init(xdd_targetattr_t *attr) {
    struct xdd_target_attributes *p = calloc(1, sizeof(*p));
    (*attr) = p;
    return 0;
}

int xdd_targetattr_destroy(xdd_targetattr_t *attr) {
    free (*attr);
    return 0;
}

int xdd_targetattr_set_type(xdd_targetattr_t *attr, xdd_target_type_t xtt) {
    (*attr)->target_type = xtt;
    return 0;
}

int xdd_targetattr_set_uri(xdd_targetattr_t *attr, char* uri) {
    strncpy((*attr)->uri, uri, 2047);
    (*attr)->uri[2047] = '\0';
    return 0;
}

int xdd_targetattr_set_dio(xdd_targetattr_t *attr, int dio_flag) {
    (*attr)->u.in.dio_flag = dio_flag;
    return 0;
}

int xdd_targetattr_set_start_offset(xdd_targetattr_t *attr, off_t off) {
    (*attr)->u.in.start_offset = off;
    return 0;
}

int xdd_targetattr_set_length(xdd_targetattr_t *attr, size_t length) {
    (*attr)->length = length;
    return 0;
}

int xdd_planattr_init(xdd_planattr_t* pattr) {
    struct xdd_plan_attributes* xpa = calloc(1, sizeof(*xpa));
    (*pattr) = xpa;
    return 0;
}

int xdd_planattr_destroy(xdd_planattr_t* pattr) {
    free(*pattr);
    return 0;
}

int xdd_planattr_set_block_size(xdd_planattr_t* pattr, size_t block_size) {
    (*pattr)->block_size = block_size;
    return 0;
}

int xdd_planattr_set_request_size(xdd_planattr_t* pattr, size_t request_size) {
    (*pattr)->request_size = request_size;
    return 1;
}

int xdd_planattr_set_retry_flag(xdd_planattr_t* pattr, int retry_flag) {
    (*pattr)->retry_flag = retry_flag;
    return 1;
}

int xdd_plan_init(xdd_planpub_t* plan, xdd_targetattr_t* tattrs, size_t ntattr, xdd_planattr_t pattr) {
    int rc = 0;
    xdd_plan_t *planp;
    xdd_occupant_t barrier_occupant;
    
    // Initializae the global data
    xint_global_data_initialization("libxdd");
    if (0 == xgp) {
        return 1;
    }

    // Initialize the internal plan type
    planp = xint_plan_data_initialization();
    if (0 == planp) {
        return 1;
    }

    // Assemble the targets underneath the plan
    //rc = xdd_initialization(argc, argv, planp);
    if (0 != rc) {
        return 1;
    }

    // Copy the plan data into the opaque public plan
    (*plan) = malloc(sizeof(**plan));
    (*plan)->data = planp;
    (*plan)->occupant = barrier_occupant;
    return rc;
}

int xdd_plan_destroy(xdd_planpub_t* plan) {
    xdd_destroy_all_barriers((*plan)->data);
    free(*plan);
    return 0;
}

int xdd_plan_start(const xdd_planpub_t* plan) {
    int rc = 1;

    // Start all of the threads
    rc = xint_plan_start((*plan)->data, &(*plan)->occupant);
    if (0 == rc) {
        xdd_destroy_all_barriers((*plan)->data);
        return 1;
    }
    return rc;
}

int xdd_plan_wait(const xdd_planpub_t* plan) {
    int rc = 0;
    xdd_plan_t* planp = (*plan)->data;

    // Wait for the results manager, which signals completion
    xdd_barrier(&planp->main_results_final_barrier,
                &(*plan)->occupant, 1);
    return rc;
}

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
