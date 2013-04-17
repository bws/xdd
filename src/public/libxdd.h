
#ifndef LIBXDD_H
#define LIBXDD_H

struct xdd_plan;
typedef struct xdd_plan xdd_plan_t;

int xdd_plan_init(xdd_plan_t* plan);

int xdd_plan_destroy(xdd_plan_t* plan);

int xdd_plan_create_e2e(xdd_plan_t* plan);

int xdd_plan_start(const xdd_plan_t* plan);

int xdd_plan_wait(const xdd_plan_t* plan);

#endif
