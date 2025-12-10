#ifndef GG_TEST_UNITY_HELPERS_H
#define GG_TEST_UNITY_HELPERS_H

#include <sys/types.h>
#include <unity.h>
#include <stddef.h>

typedef void (*GgTestFunction)(void);

typedef struct GgTestListNode {
    GgTestFunction func;
    const char *func_file_name;
    const char *func_name;
    int func_line_num;
    struct GgTestListNode *next;
} GgTestListNode;

extern GgTestListNode *gg_test_list_head;

void gg_test_register(GgTestListNode *entry);

#define GG_TEST_DEFINE(testname) \
    static void test_gg_##testname(void); \
    static GgTestListNode gg_test_entry_##testname \
        = { .func = test_gg_##testname, \
            .func_file_name = __FILE__, \
            .func_name = "test_gg_" #testname, \
            .func_line_num = __LINE__, \
            .next = NULL }; \
    __attribute__((constructor, no_profile_instrument_function)) static void \
    gg_test_register_##testname(void) { \
        gg_test_register(&gg_test_entry_##testname); \
    } \
    static void test_gg_##testname(void)

int gg_test_run_suite(void);

#define GG_TEST_FOR_EACH(name) \
    for (GgTestListNode * (name) = gg_test_list_head; (name) != NULL; \
         (name) = (name)->next)

#define GG_TEST_ASSERT_OK(expr) TEST_ASSERT_EQUAL(GG_ERR_OK, (expr))
#define GG_TEST_ASSERT_BAD(expr) TEST_ASSERT_NOT_EQUAL(GG_ERR_OK, (expr))

#define GG_TEST_ASSERT_BUF_EQUAL(expected, actual) \
    do { \
        TEST_ASSERT_EQUAL_size_t_MESSAGE( \
            (expected).len, (actual).len, "Size mismatch" \
        ); \
        TEST_ASSERT_EQUAL_CHAR_ARRAY( \
            (char *) (expected).data, (char *) (actual).data, (expected).len \
        ); \
    } while (0)

#define GG_TEST_ASSERT_BUF_EQUAL_STR(expected, actual) \
    do { \
        TEST_ASSERT_EQUAL_size_t_MESSAGE( \
            (expected).len, (actual).len, "Size mismatch" \
        ); \
        TEST_ASSERT_EQUAL_STRING_LEN( \
            (expected).data, (actual).data, (expected).len \
        ); \
    } while (0)

#ifdef ENABLE_COVERAGE
extern void __gcov_dump(void);
#undef TEST_PASS
#define TEST_PASS() \
    do { \
        __gcov_dump(); \
        TEST_ABORT(); \
    } while (0)
#endif

#endif
