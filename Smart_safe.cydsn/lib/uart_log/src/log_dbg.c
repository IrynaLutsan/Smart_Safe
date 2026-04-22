/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
#include "log_dbg.h"
#include <stdarg.h>
#include "project.h"
#include <stdio.h>


/********************************************************************************
 **********                         PRIVATE VARIABLES                 ***********
*********************************************************************************/
static char buf[LOG_DBG_BUF_SIZE];

/********************************************************************************
 **********                        PRIVATE FUNCTIONS                  ***********
*********************************************************************************/

/**
 * @brief Initialize UART transport used by logging module.
 */
void dbg_log_init(void)
{
    SW_Tx_UART_Start();
}

/**
 * @brief Core logging function that formats and outputs debug messages
 * @param level Log level string (e.g., "ERROR", "INFO", "DEBUG")
 * @param module Module tag/name for message source identification
 * @param fmt Format string (printf-style) for message content
 * @param ... Variable arguments to be formatted according to fmt
 * @details Formats log messages with timestamp information [level][module] prefix,
 *          then outputs via UART serial communication. Message is formatted using
 *          snprintf for safe buffer handling and variable argument expansion via vsnprintf.
 *          Output includes automatic CRLF line termination for proper serial display.
 * @return void
 * @note This is the core implementation for LOG_E, LOG_I, and LOG_D macros.
 *       Maximum message length is limited by LOG_DBG_BUF_SIZE (defined in log_dbg.h).
 *       Messages exceeding buffer size will be truncated to prevent buffer overflow.
 * @see log_dbg.h for macro definitions and LOG_LEVEL configuration
 */
void dbg_log_print(const char *level, const char *module, const char *fmt, ...)
{
    
    int n = 0;
    n += snprintf(buf, sizeof(buf), "[%s][%s] ", level, module);
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf + n, sizeof(buf) - n, fmt, args);
    va_end(args);
    SW_Tx_UART_PutString((const char8 *)buf);
    SW_Tx_UART_PutCRLF();
}

/* [] END OF FILE */
