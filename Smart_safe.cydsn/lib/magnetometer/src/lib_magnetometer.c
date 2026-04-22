/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
#include "lib_magnetometer.h"
#include "project.h"
#include "lis3mdl_reg.h"

#define LOG_LEVEL LOG_LEVEL_INFO
#define TAG "MAG"
#define LIS3MDL_I2C_ADDR (0x1Cu)
#include "log_dbg.h"


/********************************************************************************
 **********                         PRIVATE VARIABLES                 ***********
*********************************************************************************/
static stmdev_ctx_t dev_ctx;


/********************************************************************************
 **********                        FUNCTION PROTOTYPES                ***********
*********************************************************************************/
static int32_t platform_write_lis(void*, uint8_t reg, const uint8_t *bufp, uint16_t len);
static int32_t platform_read_lis(void*, uint8_t reg, uint8_t *bufp, uint16_t len);
static void platform_delay_lis(uint32_t delay);


/********************************************************************************
 **********                         PUBLIC FUNCTIONS                  ***********
*********************************************************************************/

/**
 * @brief   Initialization of magnetometer LIS3MDL
 * @details Configures I2C interface, resets sensor, sets up ODR, full scale,
 *          enables temperature measurement, and sets continuous mode
 * @return  void
 * @note    Must be called once during system initialization
 */
void lib_magnetometer_init()
{
    uint8_t  rst;
    uint8_t whoamI = 0;
    
    dev_ctx.write_reg = platform_write_lis;
    dev_ctx.read_reg = platform_read_lis;
    dev_ctx.mdelay = platform_delay_lis;

    /* Initialize platform specific hardware */
 
    /* Check device ID */
    lis3mdl_device_id_get(&dev_ctx, &whoamI);
    LOG_I(TAG, "Device ID: 0x%02X (expected: 0x%02X)", whoamI, LIS3MDL_ID);
    
    if (whoamI != LIS3MDL_ID)
    {
        LOG_E(TAG, "Device ID mismatch! Got 0x%02X, expected 0x%02X", whoamI, LIS3MDL_ID);
        while (1); /*manage here device not found */
    }
    
    LOG_D(TAG, "Device ID verified successfully");
    /* Restore default configuration */
    lis3mdl_reset_set(&dev_ctx, PROPERTY_ENABLE);
    LOG_D(TAG, "Reset initiated");
  
    do 
    {
        lis3mdl_reset_get(&dev_ctx, &rst);
    } while (rst);
    
    LOG_D(TAG, "Reset complete");
  
    /* Enable Block Data Update */
    lis3mdl_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
    LOG_D(TAG, "Block Data Update enabled");
    
    /* Set Output Data Rate */
    lis3mdl_data_rate_set(&dev_ctx, LIS3MDL_HP_40Hz);
    LOG_D(TAG, "ODR set to 40 Hz");
    
    /* Set full scale */
    lis3mdl_full_scale_set(&dev_ctx, LIS3MDL_16_GAUSS);
    LOG_D(TAG, "Full scale: ±16 Gauss");
    
    /* Enable temperature sensor */
    lis3mdl_temperature_meas_set(&dev_ctx, PROPERTY_ENABLE);
    LOG_D(TAG, "Temperature measurement enabled");
    
    /* Set device in continuous mode */
    lis3mdl_operating_mode_set(&dev_ctx, LIS3MDL_CONTINUOUS_MODE);
    LOG_D(TAG, "Operating mode: continuous");
    
    LOG_I(TAG, "LIS3MDL initialization complete");
}

/**
 * @brief   Get magnetometer and temperature data
 * @details Reads current magnetic field (X,Y,Z) and sensor temperature.
 *          Converts RAW values to physical units (mGauss, Celsius)
 * @return  lib_magnetometer_data_t structure with data and freshness flag
 * @note    Function automatically checks data ready status before reading
 */
lib_magnetometer_data_t lib_magnetometer_get()
{
    lib_magnetometer_data_t ret_data;
    int16_t data_raw_magnetic[3];
    int16_t data_raw_temperature;
    uint8_t reg1;
    
    lis3mdl_mag_data_ready_get(&dev_ctx, &reg1);

    if (reg1) 
    {
        /* Read magnetic field data */
        memset(data_raw_magnetic, 0x00, 3 * sizeof(int16_t));
        lis3mdl_magnetic_raw_get(&dev_ctx, data_raw_magnetic);
        ret_data.mag.x = lis3mdl_from_fs16_to_mgauss(
                            data_raw_magnetic[0]);
        ret_data.mag.y = lis3mdl_from_fs16_to_mgauss(
                            data_raw_magnetic[1]);
        ret_data.mag.z = lis3mdl_from_fs16_to_mgauss(
                            data_raw_magnetic[2]);
        
        LOG_D(TAG, "Mag X=%ld Y=%ld Z=%ld mG", 
              ret_data.mag.x, ret_data.mag.y, ret_data.mag.z);

        /* Read temperature data */
        memset(&data_raw_temperature, 0x00, sizeof(int16_t));
        lis3mdl_temperature_raw_get(&dev_ctx, &data_raw_temperature);
        ret_data.temperature = lis3mdl_from_lsb_to_celsius_x10(data_raw_temperature);
        
        LOG_D(TAG, "Temperature: %d (x0.1°C)", ret_data.temperature);
        ret_data.is_new = 1;
    }  
    
    return ret_data;
}

/********************************************************************************
 **********                        PRIVATE FUNCTIONS                  ***********
*********************************************************************************/

/**
 * @brief   Write data to LIS3MDL sensor registers
 * @param   handle      Pointer to device structure (unused)
 * @param   reg         Starting register address
 * @param   bufp        Pointer to buffer with data to write
 * @param   len         Number of bytes to write
 * @return  0 on success
 * @details Writes len bytes starting from register address reg. Uses I2C
 *          with device address 0x1C. Requires counter for each byte write
 * @note    Blocking function - waits for I2C operation completion
 */
static int32_t platform_write_lis(void* handle, uint8_t reg, const uint8_t *bufp, uint16_t len)
{
    (void) handle;
    
    for(uint8_t i = 0; i < len; i++)
    {
        uint8_t dat[2] = {(reg + i), bufp[i]};
        (void) I2C_I2CMasterWriteBuf(LIS3MDL_I2C_ADDR,
                                           dat,
                                           2,
                                           I2C_I2C_MODE_COMPLETE_XFER);
        while (I2C_I2CMasterStatus() & I2C_I2C_MSTAT_XFER_INP) {}
        while ((I2C_I2CMasterStatus() & I2C_I2C_MSTAT_RD_CMPLT) == 0) {}
    }

return 0;
}

/**
 * @brief   Read data from LIS3MDL sensor registers
 * @param   handle      Pointer to device structure (unused)
 * @param   reg         Starting register address
 * @param   bufp        Pointer to buffer for storing read data
 * @param   len         Number of bytes to read
 * @return  0 on success
 * @details Reads len bytes from register address reg. Uses I2C with device address 0x1C.
 *          First sends register address, then reads data
 * @note    Blocking function - waits for I2C operations completion (write + read)
 */
static int32_t platform_read_lis(void* handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
    (void) handle;
    (void) I2C_I2CMasterWriteBuf(LIS3MDL_I2C_ADDR,
                                 &reg,
                                 1,
                                 I2C_I2C_MODE_COMPLETE_XFER);
    while (I2C_I2CMasterStatus() & I2C_I2C_MSTAT_XFER_INP) {}
    while ((I2C_I2CMasterStatus() & I2C_I2C_MSTAT_WR_CMPLT) == 0) {}

    (void) I2C_I2CMasterReadBuf(LIS3MDL_I2C_ADDR,
                                bufp,
                                len,
                                I2C_I2C_MODE_COMPLETE_XFER);
    while (I2C_I2CMasterStatus() & I2C_I2C_MSTAT_XFER_INP) {}
    while ((I2C_I2CMasterStatus() & I2C_I2C_MSTAT_RD_CMPLT) == 0) {}


return 0;
}

/**
 * @brief   Delay function required for sensor operation
 * @param   delay       Delay time in milliseconds
 * @return  Nothing (void)
 * @details Wrapper around CyDelay() function for compliance with
 *          STMicroelectronics interface. Used by driver for pauses between
 *          initialization and read operations
 * @note    Does not block RTOS (if used)
 */
static void platform_delay_lis(uint32_t delay)
{
    CyDelay(delay);
}
/* [] END OF FILE */
