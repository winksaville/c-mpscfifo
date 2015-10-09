/**
 * Miscelaneous header file
 */

#ifndef COM_SAVILLE_MPSCFIFO_MISC_H
#define COM_SAVILLE_MPSCFIFO_MISC_H

#include <stdbool.h>
#include <stdio.h>

/**
 * Set "bool error" to "true" if cond is "false"
 */
#define TEST(cond, text)                                                       \
  do {                                                                         \
    bool result = !(cond);                                                     \
    error |= result;                                                           \
    if (result) {                                                              \
      printf("Error %s:%d FN %s: %s. Condition '" #cond "' failed.\n",         \
             __FILE__, __LINE__, __func__, text);                              \
    }                                                                          \
  } while (false);

#define UNUSED(x) (void)(x)

#endif
