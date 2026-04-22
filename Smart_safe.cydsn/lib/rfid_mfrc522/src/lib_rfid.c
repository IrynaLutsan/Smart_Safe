/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
#include "lib_rfid.h"
#include "project.h"
#include "rfid_mrfc522.h"

#define LOG_LEVEL LOG_LEVEL_INFO
#define TAG "RFID"
#include "log_dbg.h"


/********************************************************************************
 **********                        FUNCTION PROTOTYPES                ***********
*********************************************************************************/
static void platform_write_rfid(u_char, u_char);
static u_char platform_read_rfid(u_char);
static void platform_gpio_rst_rfid(uint8_t);
static void platform_delay_rfid(uint32_t);

/********************************************************************************
 **********                         PUBLIC FUNCTIONS                  ***********
*********************************************************************************/

/**
 * @brief   Initialize RFID MFRC522 module
 * @details Initializes the MFRC522 RFID reader with platform-specific functions
 *          for SPI communication, GPIO reset, and delay operations. Verifies
 *          module version and logs initialization status
 * @return  void
 * @note    Must be called before using any other RFID functions. Uses SPI slave select 1
 */
void lib_rfid_init()
{
    rfid_ctx_t ctx;
    ctx.read_reg = platform_read_rfid;
    ctx.write_reg = platform_write_rfid;
    ctx.rst = platform_gpio_rst_rfid;
    ctx.mdelay = platform_delay_rfid;
    
    MFRC522_init(ctx);
    u_char status = Read_MFRC522(VersionReg);
    LOG_D(TAG, "MFRC522_Ver: %x", status);
    
    if (status == 0xFF)
    {
        LOG_E(TAG, "MFRC522 init fail");
    }
    else
    {
        LOG_I(TAG, "RFID INIT OK");
    }
}

/**
 * @brief   Scan for RFID cards and retrieve card UID
 * @param   data        Pointer to buffer to store card UID (5 bytes)
 * @details Performs RFID card detection using MFRC522_Request and MFRC522_Anticoll.
 *          Attempts card detection up to 2 times and returns the 5-byte UID if successful
 * @return  ret_code_t  RET_CODE_OK if card detected and UID retrieved, RET_CODE_ERR otherwise
 * @note    Buffer must be at least 5 bytes. Function blocks during scan operation
 */
ret_code_t lib_rfid_scan(uint8_t* data)
{
    ret_code_t ret_code = RET_CODE_ERR;
    u_char status, str[MAX_LEN+1];
    
    status = MFRC522_Request(PICC_REQIDL, str);
    LOG_D(TAG, "Card request status: %d", status);
    
    if (status == MI_OK) 
    {
        LOG_D(TAG, "Card detected, initiating anticollision...");
        CyDelay(1);
        for(uint8_t i = 0; i < 2; i++)
        {
            status = MFRC522_Anticoll(str);
    	    if(status == MI_OK) 
            {
                data[0] = str[0];
    		    data[1] = str[1];
    		    data[2] = str[2];
    		    data[3] = str[3];
    		    data[4] = str[4];
                LOG_I(TAG, "Card UID read: %02X%02X%02X%02X%02X", 
                      data[0], data[1], data[2], data[3], data[4]);
                ret_code = RET_CODE_OK;
                break;
            }
        }
        if(ret_code != RET_CODE_OK)
        {
            LOG_D(TAG, "Anticollision failed");
        }
    }
    else
    {
        LOG_D(TAG, "No card detected");
    }
    return ret_code;
}

/********************************************************************************
 **********                        PRIVATE FUNCTIONS                  ***********
*********************************************************************************/

/**
 * @brief   Platform-specific SPI write function for RFID module
 * @param   addr        Register address to write to
 * @param   val         Value to write to the register
 * @details Performs SPI write operation to MFRC522 using PSoC SPI component.
 *          Uses critical section to ensure atomic operation and slave select control
 * @return  void
 * @note    Uses SPI slave select 1 for RFID communication
 */
static void platform_write_rfid(u_char addr, u_char val)
{
    uint8 interruptState;
	u_char addr_bits = (((addr<<1) & 0x7E));

    interruptState = CyEnterCriticalSection();
    while(SPIM_SpiIsBusBusy() != 0){};
     SPIM_SpiSetActiveSlaveSelect(SPIM_SPI_SLAVE_SELECT1);
    
    SPIM_SpiUartWriteTxData(addr_bits);
    SPIM_SpiUartWriteTxData(val);
    while(SPIM_SpiIsBusBusy() != 0){};
    
    SPIM_SpiSetActiveSlaveSelect(SPIM_SPI_SLAVE_SELECT0);

    CyExitCriticalSection(interruptState);
    
    while(SPIM_SpiUartGetTxBufferSize()>0){};
}

/**
 * @brief   Platform-specific SPI read function for RFID module
 * @param   addr        Register address to read from
 * @details Performs SPI read operation from MFRC522 using PSoC SPI component.
 *          Uses critical section and proper SPI timing for reliable communication
 * @return  u_char      Value read from the specified register
 * @note    Uses SPI slave select 1 for RFID communication
 */
static u_char platform_read_rfid(u_char addr)
{
    u_char rx_bits;
    u_char addr_bits = (((addr<<1) & 0x7E) | 0x80);
    uint8 interruptState;

    interruptState = CyEnterCriticalSection();
    
    while(SPIM_SpiIsBusBusy() != 0){};
    SPIM_SpiSetActiveSlaveSelect(SPIM_SPI_SLAVE_SELECT1);

    SPIM_SpiUartWriteTxData(addr_bits);

    SPIM_SpiUartClearRxBuffer();
    SPIM_SpiUartWriteTxData(0xff);

    while(SPIM_SpiIsBusBusy() != 0){};

    rx_bits = SPIM_SpiUartReadRxData();


    SPIM_SpiSetActiveSlaveSelect(SPIM_SPI_SLAVE_SELECT0);
    
    CyExitCriticalSection(interruptState);

	return (u_char) rx_bits; // return the rx bits, casting to an 8 bit int and chopping off the upper 24 bits
}

/**
 * @brief   Platform-specific GPIO reset control for RFID module
 * @param   state       Reset pin state (0 = reset active, 1 = normal operation)
 * @details Controls the MFRC522 reset pin using PSoC GPIO component
 * @return  void
 * @note    Used by MFRC522 driver for hardware reset operations
 */
static void platform_gpio_rst_rfid(uint8_t state)
{
    RFID_RST_Write(state);
}

/**
 * @brief   Platform-specific delay function for RFID module
 * @param   delay       Delay time in milliseconds
 * @details Provides millisecond delay using PSoC CyDelay function
 * @return  void
 * @note    Used by MFRC522 driver for timing-critical operations
 */
static void platform_delay_rfid(uint32_t delay)
{
    CyDelay(delay);
}

/* [] END OF FILE */
