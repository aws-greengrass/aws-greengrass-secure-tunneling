#ifndef TEST_IPC_MOCK_H
#define TEST_IPC_MOCK_H

#include <gg/error.h>

GgError gg_test_setup_ipc_for_tests(void);
void gg_test_cleanup_ipc(void);

#endif
