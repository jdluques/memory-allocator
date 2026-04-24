#ifndef FRAMEWORK_H
#define FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── Colours ───────────────────────────────────────────────────────────────────

#define COL_RESET   "\033[0m"
#define COL_RED     "\033[31m"
#define COL_GREEN   "\033[32m"
#define COL_YELLOW  "\033[33m"
#define COL_CYAN    "\033[36m"
#define COL_BOLD    "\033[1m"

// ── Suite state ───────────────────────────────────────────────────────────────

static int _tests_run    = 0;
static int _tests_passed = 0;
static int _tests_failed = 0;

// ── Assertions ────────────────────────────────────────────────────────────────

#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("    " COL_RED "✗ FAIL" COL_RESET \
                   " %s:%d  →  %s\n", __FILE__, __LINE__, #cond); \
            _tests_failed++; \
            _tests_run++; \
            return; \
        } \
        _tests_passed++; \
        _tests_run++; \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            printf("    " COL_RED "✗ FAIL" COL_RESET \
                   " %s:%d  →  %s == %s  (%lld != %lld)\n", \
                   __FILE__, __LINE__, #a, #b, \
                   (long long)(a), (long long)(b)); \
            _tests_failed++; \
            _tests_run++; \
            return; \
        } \
        _tests_passed++; \
        _tests_run++; \
    } while (0)

#define ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            printf("    " COL_RED "✗ FAIL" COL_RESET \
                   " %s:%d  →  %s != %s  (both = %lld)\n", \
                   __FILE__, __LINE__, #a, #b, (long long)(a)); \
            _tests_failed++; \
            _tests_run++; \
            return; \
        } \
        _tests_passed++; \
        _tests_run++; \
    } while (0)

#define ASSERT_NULL(p) \
    ASSERT((p) == NULL)

#define ASSERT_NOT_NULL(p) \
    ASSERT((p) != NULL)

// Confirm every byte in a region equals `val`
#define ASSERT_MEM(ptr, val, len) \
    do { \
        const unsigned char *_p = (const unsigned char *)(ptr); \
        for (size_t _i = 0; _i < (len); _i++) { \
            if (_p[_i] != (unsigned char)(val)) { \
                printf("    " COL_RED "✗ FAIL" COL_RESET \
                       " %s:%d  →  ASSERT_MEM byte[%zu] = 0x%02x, want 0x%02x\n", \
                       __FILE__, __LINE__, _i, _p[_i], (unsigned char)(val)); \
                _tests_failed++; \
                _tests_run++; \
                return; \
            } \
        } \
        _tests_passed++; \
        _tests_run++; \
    } while (0)

// ── Test / suite macros ───────────────────────────────────────────────────────

#define TEST(name) \
    static void name(void)

#define RUN(name) \
    do { \
        printf("  " COL_CYAN "→" COL_RESET " %-45s", #name); \
        int _before = _tests_failed; \
        name(); \
        if (_tests_failed == _before) \
            printf(COL_GREEN " PASS" COL_RESET "\n"); \
        else \
            printf("\n"); \
    } while (0)

#define SUITE(name) \
    printf(COL_BOLD "\n[%s]\n" COL_RESET, name)

// ── Final report ──────────────────────────────────────────────────────────────

#define REPORT() \
    do { \
        printf(COL_BOLD "\n────────────────────────────────────────────\n" COL_RESET); \
        printf("  Assertions : %d\n", _tests_run); \
        printf("  " COL_GREEN "Passed" COL_RESET "     : %d\n", _tests_passed); \
        if (_tests_failed > 0) \
            printf("  " COL_RED "Failed" COL_RESET "     : %d\n", _tests_failed); \
        else \
            printf("  Failed     : 0\n"); \
        printf(COL_BOLD "────────────────────────────────────────────\n" COL_RESET); \
        return (_tests_failed > 0) ? 1 : 0; \
    } while (0)

#endif
