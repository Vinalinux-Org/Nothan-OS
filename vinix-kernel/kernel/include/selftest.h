/* ============================================================
 * selftest.h — boot-time test harness.
 * ============================================================ */

#ifndef SELFTEST_H
#define SELFTEST_H

/* Every test returns 0 on pass, <0 on fail. */
typedef int (*selftest_fn)(void);

struct selftest {
    const char *name;
    selftest_fn run;
};

/* Run registered tests, log pass/fail, panic on first failure. */
void selftest_run_all(void);

#endif
