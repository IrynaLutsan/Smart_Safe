/**
 * @file lib_common_types.h
 * @brief Shared type definitions and return codes used across libraries.
 */

#ifndef LIB_COMMON_TYPES_H
#define LIB_COMMON_TYPES_H

#include "stdint.h"

/**
 * @brief Logging level definitions.
 */
#define LOG_LEVEL_ERR  1 /**< Error-only logging. */
#define LOG_LEVEL_INFO 2 /**< Informational logging. */
#define LOG_LEVEL_DBG  3 /**< Debug logging. */

/**
 * @brief Standard return code values for library APIs.
 */
typedef enum ret_code {
    RET_CODE_OK      = 0, /**< Operation completed successfully. */
    RET_CODE_ERR     = 1, /**< Operation failed. */
    RET_CODE_TIMEOUT = 2, /**< Operation timed out. */
} ret_code_t;

#endif // LIB_COMMON_TYPES_H

/* [] END OF FILE */
