
#include "libxdd.h"
#include <assert.h>
#include <string.h>
#define XDDMAIN
#include "xint.h"

int xdd_plan_init(xdd_plan_pub_t* plan) {
    assert(0 != plan);
    memset(plan, 0, sizeof(*plan));
    return 0;
}

int xdd_plan_destroy(xdd_plan_pub_t* plan) {
    assert(0 != plan);
    memset(plan, 0, sizeof(*plan));
    return 0;
}

int xdd_plan_create_e2e(xdd_plan_pub_t* plan) {
    assert(0 != plan);
    return 0;
}

int xdd_plan_start(const xdd_plan_pub_t* plan) {
    assert(0 != plan);
    return 0;
}

int xdd_plan_wait(const xdd_plan_pub_t* plan) {
    assert(0 != plan);
    return 0;
}
