/**
 * @file lib_rfid.h
 * @brief RFID reader interface.
 */

#ifndef LIB_RFID_H
#define LIB_RFID_H

#include "lib_common_types.h"

/**
 * @brief Length of the RFID UID in bytes.
 */
#define RFID_UID_LEN 5
    
/**
 * @brief Initialize the RFID reader hardware.
 */
void lib_rfid_init(void);

/**
 * @brief Scan for an RFID tag and read its UID.
 *
 * @param[out] data Buffer to receive the RFID UID. Must be at least RFID_UID_LEN bytes.
 * @return RET_CODE_OK on success, or an error code on failure.
 */
ret_code_t lib_rfid_scan(uint8_t* data);

#endif // LIB_RFID_H
/* [] END OF FILE */
