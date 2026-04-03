/*
 * niyah_main.c — NIYAH v3.0 smoke-test + benchmark driver
 *
 * This is the only entry point. No unused variables.
 * Returns 0 if all smoke assertions pass, 1 otherwise.
 */
#include "niyah_core.h"
#include <stdio.h>

int main(void) {
    int failed = niyah_smoke();
    if (failed == 0) {
        printf("SMOKE PASS — 0 failed\n");
        return 0;
    }
    printf("SMOKE FAIL — %d failed\n", failed);
    return 1;
}
