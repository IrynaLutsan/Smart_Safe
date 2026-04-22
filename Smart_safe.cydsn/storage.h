/**
 * @file storage.h
 * @brief Non-volatile storage for PIN and RFID UIDs.
 *
 * NVM layout (one 128-byte flash row):
 *   [0]      PIN length  (0xFF = uninitialised → default PIN applied at init)
 *   [1..8]   PIN digits  (raw values 0-9, up to 8 digits)
 *   [9]      RFID count  (number of registered 5-byte UIDs, max 8)
 *   [10..49] RFID UIDs   (packed 5-byte blocks, up to 8 entries)
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>

#define STORAGE_PIN_MAX_LEN   8u
#define STORAGE_RFID_MAX_CNT  8u

/**
 * @brief Initialise storage. Writes default PIN {1,2,3,4} if flash is blank.
 */
void storage_init(void);

/**
 * @brief Return 1 if buf[0..len-1] matches the stored PIN, 0 otherwise.
 */
uint8_t storage_verify_pin(const uint8_t* buf, uint8_t len);

/**
 * @brief Overwrite the stored PIN with buf[0..len-1].
 */
void storage_set_pin(const uint8_t* buf, uint8_t len);

/**
 * @brief Return 1 if uid (5 bytes) matches any stored RFID UID, 0 otherwise.
 */
uint8_t storage_verify_rfid(const uint8_t* uid);

/**
 * @brief Append uid (5 bytes) to the stored RFID list (silently ignored if full).
 */
void storage_add_rfid(const uint8_t* uid);

#endif /* STORAGE_H */

/* [] END OF FILE */
