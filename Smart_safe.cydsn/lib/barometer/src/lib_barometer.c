/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
#include "lib_barometer.h"
#include "project.h"
#include "bmx280.h"

#define LOG_LEVEL LOG_LEVEL_ERR
#define TAG "BMP"
#define BMP280_I2C_ADDR (0x76u)
#include "log_dbg.h"


/********************************************************************************
 **********                         PRIVATE VARIABLES                 ***********
*********************************************************************************/
static bmx_ctx_t dev_ctx;


/********************************************************************************
 **********                        FUNCTION PROTOTYPES                ***********
*********************************************************************************/
static void platform_write_bmx(uint8_t, uint8_t);
static void platform_read_bmx(uint8_t, uint8_t*, uint16_t);
static void platform_delay_bmx(uint32_t);


/********************************************************************************
 **********                         PUBLIC FUNCTIONS                  ***********
*********************************************************************************/

/**
 * @brief   Initialization of BMP280 barometer sensor
 * @details Configures I2C interface, checks sensor ID, loads calibration data,
 *          and sets up measurement parameters for pressure and temperature
 * @return  void
 * @note    Must be called once during system initialization. Calls bmp280_init()
 */
void lib_barometer_init()
{
    dev_ctx.write_reg = platform_write_bmx;
    dev_ctx.read_reg = platform_read_bmx;
    dev_ctx.mdelay = platform_delay_bmx;
    if(bmp280_init(dev_ctx))
    {
        LOG_E(TAG, "INIT BMP");
        while(1){}
    }
}

/**
 * @brief   Get barometer pressure and temperature data
 * @details Performs measurement and reads current pressure and temperature values.
 *          Converts RAW values to physical units (Pa, 0.01°C)
 * @return  lib_barometer_data_t structure with pressure and temperature data
 * @note    Function performs measurement automatically before reading values
 */
lib_barometer_data_t lib_barometer_get()
{
    lib_barometer_data_t ret_data;
    bmp280_measure();
    ret_data.pressure = bmp280_get_pressure();
    ret_data.temperature = bmp280_get_temperature();;
    
    return ret_data;
}


/********************************************************************************
 **********                        PRIVATE FUNCTIONS                  ***********
*********************************************************************************/

/**
 * @brief   Write data to BMP280 sensor registers
 * @param   reg         Register address
 * @param   value       Value to write
 * @details Writes single byte to BMP280 register using I2C address 0x76
 * @note    Blocking function - waits for I2C operation completion
 */
static void platform_write_bmx(uint8_t reg, uint8_t value)
{
    uint8_t dat[2] = {reg, value};
    (void) I2C_I2CMasterWriteBuf(BMP280_I2C_ADDR,
                                   dat,
                                   2,
                                   I2C_I2C_MODE_COMPLETE_XFER);
    while (I2C_I2CMasterStatus() & I2C_I2C_MSTAT_XFER_INP) {}
    while ((I2C_I2CMasterStatus() & I2C_I2C_MSTAT_RD_CMPLT) == 0) {}
}

/**
 * @brief   Read data from BMP280 sensor registers
 * @param   reg         Starting register address
 * @param   bufp        Buffer to store read data
 * @param   len         Number of bytes to read
 * @details Reads multiple bytes from BMP280 registers using I2C address 0x76.
 *          First sends register address, then reads data
 * @note    Blocking function - waits for I2C operations completion (write + read)
 */
static void platform_read_bmx(uint8_t reg, uint8_t* bufp, uint16_t len)
{
    (void) I2C_I2CMasterWriteBuf(BMP280_I2C_ADDR,
                                 &reg,
                                 1,
                                 I2C_I2C_MODE_COMPLETE_XFER);
    while (I2C_I2CMasterStatus() & I2C_I2C_MSTAT_XFER_INP) {}
    while ((I2C_I2CMasterStatus() & I2C_I2C_MSTAT_WR_CMPLT) == 0) {}

    (void) I2C_I2CMasterReadBuf(BMP280_I2C_ADDR,
                                bufp,
                                len,
                                I2C_I2C_MODE_COMPLETE_XFER);
    while (I2C_I2CMasterStatus() & I2C_I2C_MSTAT_XFER_INP) {}
    while ((I2C_I2CMasterStatus() & I2C_I2C_MSTAT_RD_CMPLT) == 0) {}
}

/**
 * @brief   Delay function required for BMP280 sensor operation
 * @param   delay       Delay time in milliseconds
 * @return  Nothing (void)
 * @details Wrapper around CyDelay() function for compliance with BMP280 driver interface.
 *          Used by driver for pauses between initialization and measurement operations
 * @note    Does not block RTOS (if used)
 */
static void platform_delay_bmx(uint32_t delay)
{
    CyDelay(delay);
}

/* [] END OF FILE */
