#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

/**
 * Converts a char* string to int with error checking.
 * Returns 0 on success, -1 on failure.
 * The converted value is stored in the output parameter.
 */
int int_(const char *str, int *output) {
    if (str == NULL || output == NULL) {
        return -1;
    }

    char *endptr;
    long val = strtol(str, &endptr, 10);

    // Check for no conversion
    if (endptr == str) {
        return -1;
    }

    // Check for trailing non-numeric characters
    if (*endptr != '\0') {
        return -1;
    }

    // Check for overflow/underflow
    if (val < INT_MIN || val > INT_MAX) {
        return -1;
    }

    *output = (int)val;
    return 0;
}