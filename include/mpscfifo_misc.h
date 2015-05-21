/**
 * Miscelaneous header file
 */

/**
 * Return TRUE is condition is false
 */
#define TEST(cond, text)                                               \
  do {                                                                 \
    error |= !(cond);                                                  \
    if (error) {                                                       \
      printf("Error %s:%d FN %s: %s. Condition '" #cond "' failed.\n", \
             __FILE__, __LINE__, __func__, text);                      \
    }                                                                  \
  } while (false);

#define UNUSED(x) (void)(x)

