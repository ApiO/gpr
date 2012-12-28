#ifndef GPR_ASSERT_H
#define GPR_ASSERT_H

#include <assert.h>

#define gpr_assert_failure()               assert(0)
#define gpr_assert_failure_msg(message)    assert(0 && message)
#define gpr_assert(condition)              assert(condition)
#define gpr_assert_msg(condition, message) assert(condition && message)
#define gpr_assert_alloc(ptr)              assert(ptr != NULL && \
                                                  "allocation failed")

#endif // GPR_ASSERT_H