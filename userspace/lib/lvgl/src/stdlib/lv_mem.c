/**
 * @file lv_mem.c
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_mem_private.h"
#include "lv_string.h"
#include "../misc/lv_assert.h"
#include "../misc/lv_log.h"
#include "../core/lv_global.h"

#if LV_USE_OS == LV_OS_PTHREAD
    #include <pthread.h>
#endif

/*********************
 *      DEFINES
 *********************/
/*memset the allocated memories to 0xaa and freed memories to 0xbb (just for testing purposes)*/
#ifndef LV_MEM_ADD_JUNK
    #define LV_MEM_ADD_JUNK  0
#endif

#define zero_mem LV_GLOBAL_DEFAULT()->memory_zero

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  GLOBAL PROTOTYPES
 **********************/
void * lv_malloc_core(size_t size);
void * lv_realloc_core(void * p, size_t new_size);
void lv_free_core(void * p);
void lv_mem_monitor_core(lv_mem_monitor_t * mon_p);
lv_result_t lv_mem_test_core(void);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/
#if LV_USE_LOG && LV_LOG_TRACE_MEM
    #define LV_TRACE_MEM(...) LV_LOG_TRACE(__VA_ARGS__)
#else
    #define LV_TRACE_MEM(...)
#endif

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

#ifdef NOTHAN_MEM_TRACE
/* Debug-only allocation tracer (NothanOS): logs every large request and
 * tracks the single biggest one, to tell a buffer over-run apart from a
 * pool-busting giant allocation when hunting the home-press Data Abort.
 * Compiled in only for `make asan` (-DNOTHAN_MEM_TRACE=<threshold-bytes>). */
extern void gui_logf(const char *, ...);
size_t nothan_mem_biggest = 0;
static void nothan_mem_note(const char * tag, size_t size)
{
    if(size > nothan_mem_biggest) nothan_mem_biggest = size;
    if(size >= (size_t)NOTHAN_MEM_TRACE)
        gui_logf("[MEM] %s %zu bytes (biggest %zu)\n", tag, size, nothan_mem_biggest);
}
#endif

void * lv_malloc(size_t size)
{
    LV_TRACE_MEM("allocating %lu bytes", (unsigned long)size);
    if(size == 0) {
        LV_TRACE_MEM("using zero_mem");
        return &zero_mem;
    }
#ifdef NOTHAN_MEM_TRACE
    nothan_mem_note("malloc", size);
#endif

    void * alloc = lv_malloc_core(size);

    if(alloc == NULL) {
        LV_LOG_INFO("couldn't allocate memory (%lu bytes)", (unsigned long)size);
#if LV_LOG_LEVEL <= LV_LOG_LEVEL_INFO
        lv_mem_monitor_t mon;
        lv_mem_monitor(&mon);
        LV_LOG_INFO("used: %zu (%3d %%), frag: %3d %%, biggest free: %zu",
                    mon.total_size - mon.free_size, mon.used_pct, mon.frag_pct,
                    mon.free_biggest_size);
#endif
        return NULL;
    }

#if LV_MEM_ADD_JUNK
    lv_memset(alloc, 0xaa, size);
#endif

    LV_TRACE_MEM("allocated at %p", alloc);
    return alloc;
}

void * lv_malloc_zeroed(size_t size)
{
    LV_TRACE_MEM("allocating %lu bytes", (unsigned long)size);
    if(size == 0) {
        LV_TRACE_MEM("using zero_mem");
        return &zero_mem;
    }
#ifdef NOTHAN_MEM_TRACE
    nothan_mem_note("zalloc", size);
#endif

    void * alloc = lv_malloc_core(size);
    if(alloc == NULL) {
        LV_LOG_INFO("couldn't allocate memory (%lu bytes)", (unsigned long)size);
#if LV_LOG_LEVEL <= LV_LOG_LEVEL_INFO
        lv_mem_monitor_t mon;
        lv_mem_monitor(&mon);
        LV_LOG_INFO("used: %zu (%3d %%), frag: %3d %%, biggest free: %zu",
                    mon.total_size - mon.free_size, mon.used_pct, mon.frag_pct,
                    mon.free_biggest_size);
#endif
        return NULL;
    }

    lv_memzero(alloc, size);

    LV_TRACE_MEM("allocated at %p", alloc);
    return alloc;
}

void * lv_calloc(size_t num, size_t size)
{
    LV_TRACE_MEM("allocating number of %zu each %zu bytes", num, size);
    return lv_malloc_zeroed(num * size);
}

void * lv_zalloc(size_t size)
{
    return lv_malloc_zeroed(size);
}

void lv_free(void * data)
{
    LV_TRACE_MEM("freeing %p", data);
    if(data == &zero_mem) return;
    if(data == NULL) return;

    lv_free_core(data);
}

void * lv_reallocf(void * data_p, size_t new_size)
{
    void * new = lv_realloc(data_p, new_size);
    if(!new) {
        lv_free(data_p);
    }
    return new;
}

void * lv_realloc(void * data_p, size_t new_size)
{
    LV_TRACE_MEM("reallocating %p with %lu size", data_p, (unsigned long)new_size);
    if(new_size == 0) {
        LV_TRACE_MEM("using zero_mem");
        lv_free(data_p);
        return &zero_mem;
    }

    if(data_p == &zero_mem) return lv_malloc(new_size);
#ifdef NOTHAN_MEM_TRACE
    nothan_mem_note("realloc", new_size);
#endif

    void * new_p = lv_realloc_core(data_p, new_size);

    if(new_p == NULL) {
        LV_LOG_ERROR("couldn't reallocate memory");
        return NULL;
    }

    LV_TRACE_MEM("reallocated at %p", new_p);
    return new_p;
}

lv_result_t lv_mem_test(void)
{
    if(zero_mem != ZERO_MEM_SENTINEL) {
        LV_LOG_WARN("zero_mem is written");
        return LV_RESULT_INVALID;
    }

    return lv_mem_test_core();
}

void lv_mem_monitor(lv_mem_monitor_t * mon_p)
{
    lv_memzero(mon_p, sizeof(lv_mem_monitor_t));
    lv_mem_monitor_core(mon_p);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
