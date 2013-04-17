
#ifndef LIBXDD_H
#define LIBXDD_H

#include "xdd_plan.h"

int xdd_plan_init(xdd_plan_pub_t* plan);

int xdd_plan_destroy(xdd_plan_pub_t* plan);

int xdd_plan_create_e2e(xdd_plan_pub_t* plan);

int xdd_plan_start(const xdd_plan_pub_t* plan);

int xdd_plan_wait(const xdd_plan_pub_t* plan);

#endif
