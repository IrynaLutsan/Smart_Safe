#ifndef LOG_DBG_H
#define LOG_DBG_H

/**
 * @file log_dbg.h
 * @brief Debug logging macros and utilities for system-wide logging
 * @details Provides a flexible logging system with configurable log levels
 *          (ERROR, INFO, DEBUG). Logs are output via UART serial communication
 *          with module tags and severity levels for easy debugging and monitoring.
 * 
 * @section Usage
 * In each source file, define LOG_LEVEL and TAG before including this header:
 * @code
 * #define LOG_LEVEL LOG_LEVEL_INFO
 * #define TAG "MODULE_NAME"
 * #include "log_dbg.h"
 * @endcode
 * 
 * Then use the logging macros:
 * @code
 * LOG_E(TAG, "Error message: %d", error_code);  // Error level
 * LOG_I(TAG, "Info message");                   // Info level
 * LOG_D(TAG, "Debug value: %x", debug_val);     // Debug level
 * @endcode
 * 
 * @section Levels
 * - LOG_LEVEL_ERR (1): Only errors
 * - LOG_LEVEL_INFO (2): Errors and info messages
 * - LOG_LEVEL_DBG (3): All messages (errors, info, debug)
 * 
 * Default LOG_LEVEL is LOG_LEVEL_DBG if not specified.
 * Maximum message length is 256 characters (LOG_DBG_BUF_SIZE).
 * 
 * @see log_dbg.c for implementation details
 */

#include <stdarg.h>
#include <lib_common_types.h>
    
#define LOG_DBG_BUF_SIZE 256

/**
 * @brief Set the current logging level (ERROR, INFO, or DEBUG)
 * @details Controls which log messages are output based on severity.
 *          Set this before including this header in each module to define
 *          the minimum severity level for that module's logging output.
 * @note If not defined before including log_dbg.h, defaults to LOG_LEVEL_DBG
 */

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DBG
#endif

/********************************************************************************
 **********                         PUBLIC FUNCTIONS                  ***********
*********************************************************************************/
/**
 * @brief Initialize UART backend for debug logging.
 * @details Must be called once before using LOG_E/LOG_I/LOG_D macros.
 */
void dbg_log_init(void);

/**
 * @brief Core logging function (internal use)
 * @param level Log level string identifier ("ERR", "INFO", or "DBG")
 * @param module Module tag for source identification
 * @param fmt Format string (printf-style)
 * @param ... Variable arguments for format string
 * @details This function is called by the logging macros and should not be
 *          called directly. Use LOG_E, LOG_I, or LOG_D macros instead.
 * @see LOG_E, LOG_I, LOG_D
 */
void dbg_log_print(const char *level, const char *module, const char *fmt, ...);

/**
 * @brief Log a DEBUG-level message
 * @param module Module tag (typically defined as TAG in each file)
 * @param fmt Format string (printf-style)
 * @param ... Variable arguments for format string
 * @details Outputs message only if LOG_LEVEL >= LOG_LEVEL_DBG
 * @example LOG_D(TAG, "Variable value: 0x%x", var);
 */
#define LOG_D(module, fmt, ...)  do { if (LOG_LEVEL >= LOG_LEVEL_DBG)  dbg_log_print("DBG", module, fmt, ##__VA_ARGS__); } while(0)

/**
 * @brief Log an INFO-level message
 * @param module Module tag (typically defined as TAG in each file)
 * @param fmt Format string (printf-style)
 * @param ... Variable arguments for format string
 * @details Outputs message only if LOG_LEVEL >= LOG_LEVEL_INFO
 * @example LOG_I(TAG, "Initialization complete");
 */
#define LOG_I(module, fmt, ...)  do { if (LOG_LEVEL >= LOG_LEVEL_INFO) dbg_log_print("INFO", module, fmt, ##__VA_ARGS__); } while(0)

/**
 * @brief Log an ERROR-level message
 * @param module Module tag (typically defined as TAG in each file)
 * @param fmt Format string (printf-style)
 * @param ... Variable arguments for format string
 * @details Outputs message only if LOG_LEVEL >= LOG_LEVEL_ERR (always enabled)
 * @example LOG_E(TAG, "Initialization failed with code: %d", error);
 */
#define LOG_E(module, fmt, ...)  do { if (LOG_LEVEL >= LOG_LEVEL_ERR)  dbg_log_print("ERR",  module, fmt, ##__VA_ARGS__); } while(0)

#endif // LOG_DBG_H

/* [] END OF FILE */
