/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
#include "storage.h"
#include "project.h"    /* CySysFlashWriteRow, CY_FLASH_* macros */
#include <string.h>     /* memcmp, memcpy, memset */

#define LOG_LEVEL LOG_LEVEL_INFO
#include "log_dbg.h"
#define TAG "STOR"


/********************************************************************************
 **********                        PRIVATE DEFINITIONS                ***********
*********************************************************************************/

/* PSoC 4 flash row = 128 bytes. Use the last row for persistent storage.
 * Adjust STORAGE_FLASH_ROW if the linker places code near the end of flash. */
#define STORAGE_ROW_SIZE   128u
#define STORAGE_FLASH_ROW  (CY_FLASH_NUMBER_ROWS - 1u)
#define STORAGE_ROW_ADDR   ((const uint8_t *)(CY_FLASH_BASE + \
                             (uint32_t)STORAGE_FLASH_ROW * STORAGE_ROW_SIZE))

/* NVM byte offsets */
#define NVM_PIN_LEN        0u    /* [0]      : PIN length (0xFF = blank) */
#define NVM_PIN_DATA       1u    /* [1..8]   : PIN digits (raw 0-9)      */
#define NVM_RFID_CNT       9u    /* [9]      : number of stored UIDs      */
#define NVM_RFID_DATA      10u   /* [10..49] : packed 5-byte UID blocks   */

#define RFID_UID_LEN       5u

static const uint8_t DEFAULT_PIN[]     = {1u, 2u, 3u, 4u};
static const uint8_t DEFAULT_PIN_LEN   = 4u;


/********************************************************************************
 **********                        PRIVATE FUNCTIONS                  ***********
*********************************************************************************/

/* Read the full row into a local buffer, apply modifier, write back. */
static void nvm_write(uint8_t offset, const uint8_t* data, uint8_t len)
{
    uint8_t buf[STORAGE_ROW_SIZE];
    memcpy(buf, STORAGE_ROW_ADDR, STORAGE_ROW_SIZE);
    memcpy(buf + offset, data, len);
    cystatus status = CySysFlashWriteRow((uint32)STORAGE_FLASH_ROW, buf);
    if (status != CYRET_SUCCESS)
    {
        LOG_E(TAG, "Flash write failed: %d", (int)status);
    }
}

static void nvm_write_byte(uint8_t offset, uint8_t value)
{
    nvm_write(offset, &value, 1u);
}


/********************************************************************************
 **********                         PUBLIC FUNCTIONS                  ***********
*********************************************************************************/

void storage_init(void)
{
    if (STORAGE_ROW_ADDR[NVM_PIN_LEN] == 0xFFu)
    {
        /* First boot: blank flash — write defaults. */
        uint8_t buf[STORAGE_ROW_SIZE];
        memset(buf, 0u, STORAGE_ROW_SIZE);
        buf[NVM_PIN_LEN]  = DEFAULT_PIN_LEN;
        memcpy(buf + NVM_PIN_DATA, DEFAULT_PIN, DEFAULT_PIN_LEN);
        buf[NVM_RFID_CNT] = 0u;

        cystatus status = CySysFlashWriteRow((uint32)STORAGE_FLASH_ROW, buf);
        if (status == CYRET_SUCCESS)
        {
            LOG_I(TAG, "Storage initialised with default PIN 1234");
        }
        else
        {
            LOG_E(TAG, "Storage init flash write failed: %d", (int)status);
        }
    }
    else
    {
        LOG_I(TAG, "Storage ok. PIN len=%u, RFID count=%u",
              (unsigned)STORAGE_ROW_ADDR[NVM_PIN_LEN],
              (unsigned)STORAGE_ROW_ADDR[NVM_RFID_CNT]);
    }
}

uint8_t storage_verify_pin(const uint8_t* buf, uint8_t len)
{
    uint8_t stored_len = STORAGE_ROW_ADDR[NVM_PIN_LEN];
    if (len != stored_len)
    {
        return 0u;
    }
    return (memcmp(buf, STORAGE_ROW_ADDR + NVM_PIN_DATA, len) == 0) ? 1u : 0u;
}

void storage_set_pin(const uint8_t* buf, uint8_t len)
{
    if (len == 0u || len > STORAGE_PIN_MAX_LEN)
    {
        return;
    }
    uint8_t row_buf[STORAGE_ROW_SIZE];
    memcpy(row_buf, STORAGE_ROW_ADDR, STORAGE_ROW_SIZE);
    row_buf[NVM_PIN_LEN] = len;
    memcpy(row_buf + NVM_PIN_DATA, buf, len);
    cystatus status = CySysFlashWriteRow((uint32)STORAGE_FLASH_ROW, row_buf);
    if (status == CYRET_SUCCESS)
    {
        LOG_I(TAG, "PIN updated (len=%u)", (unsigned)len);
    }
    else
    {
        LOG_E(TAG, "PIN write failed: %d", (int)status);
    }
}

uint8_t storage_verify_rfid(const uint8_t* uid)
{
    uint8_t count = STORAGE_ROW_ADDR[NVM_RFID_CNT];
    uint8_t i;
    for (i = 0u; i < count && i < STORAGE_RFID_MAX_CNT; i++)
    {
        const uint8_t* stored = STORAGE_ROW_ADDR + NVM_RFID_DATA + (i * RFID_UID_LEN);
        if (memcmp(uid, stored, RFID_UID_LEN) == 0)
        {
            return 1u;
        }
    }
    return 0u;
}

void storage_add_rfid(const uint8_t* uid)
{
    uint8_t count = STORAGE_ROW_ADDR[NVM_RFID_CNT];
    if (count >= STORAGE_RFID_MAX_CNT)
    {
        LOG_I(TAG, "RFID list full, ignoring add");
        return;
    }

    uint8_t row_buf[STORAGE_ROW_SIZE];
    memcpy(row_buf, STORAGE_ROW_ADDR, STORAGE_ROW_SIZE);
    memcpy(row_buf + NVM_RFID_DATA + (count * RFID_UID_LEN), uid, RFID_UID_LEN);
    row_buf[NVM_RFID_CNT] = count + 1u;

    cystatus status = CySysFlashWriteRow((uint32)STORAGE_FLASH_ROW, row_buf);
    if (status == CYRET_SUCCESS)
    {
        LOG_I(TAG, "RFID added (slot %u)", (unsigned)count);
    }
    else
    {
        LOG_E(TAG, "RFID add flash write failed: %d", (int)status);
    }
}

/* [] END OF FILE */
