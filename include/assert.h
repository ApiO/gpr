#ifndef GPCORE_ASSERT_H
#define GPCORE_ASSERT_H

#include <assert.h>

#define GP_ASSERT     (condition)          assert(condition)
#define GP_ASSERT_MSG (condition, message) assert(condition && message)

#endif // GPCORE_ASSERT_H