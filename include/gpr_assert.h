#ifndef GPR_ASSERT_H
#define GPR_ASSERT_H

#include <assert.h>

#define GPR_ASSERT     (condition)          assert(condition)
#define GPR_ASSERT_MSG (condition, message) assert(condition && message)

#endif // GPR_ASSERT_H