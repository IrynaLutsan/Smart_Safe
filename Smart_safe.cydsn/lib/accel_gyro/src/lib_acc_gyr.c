/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
#include "lib_acc_gyr.h"
#include "project.h"
#include "lsm6ds3tr-c_reg.h"

#define LOG_LEVEL LOG_LEVEL_INFO
#define TAG "ACC_GYR"
#define LSM6DS3TR_C_I2C_ADDR (0x6Au)
#include "log_dbg.h"


/********************************************************************************
 **********                         PRIVATE VARIABLES                 ***********
*********************************************************************************/
static stmdev_ctx_t dev_ctx_lsm;
static lsm6ds3tr_c_reg_t reg;


/********************************************************************************
 **********                        FUNCTION PROTOTYPES                ***********
*********************************************************************************/
static int32_t platform_write_lsm(void*, uint8_t reg, const uint8_t *bufp, uint16_t len);
static int32_t platform_read_lsm(void*, uint8_t reg, uint8_t *bufp, uint16_t len);
static void platform_delay_lsm(uint32_t delay);


/********************************************************************************
 **********                         PUBLIC FUNCTIONS                  ***********
*********************************************************************************/

/**
 * @brief   Initialization of accelerometer/gyroscope LSM6DS3TR-C
 * @details Configures I2C interface, resets sensor, sets up ODR, filters,
 *          and measurement ranges
 * @return  void
 * @note    Must be called once during system initialization
 */
void lib_acc_gyr_init()
{
    uint8_t  rst;
    uint8_t whoamI = 0;
    
    dev_ctx_lsm.write_reg = platform_write_lsm;
    dev_ctx_lsm.read_reg = platform_read_lsm;
    dev_ctx_lsm.mdelay = platform_delay_lsm;
    
    lsm6ds3tr_c_device_id_get(&dev_ctx_lsm, &whoamI);
    LOG_I(TAG, "Device ID: 0x%02X (expected: 0x%02X)", whoamI, LSM6DS3TR_C_ID);
    
    /* Restore default configuration */
    lsm6ds3tr_c_reset_set(&dev_ctx_lsm, PROPERTY_ENABLE);

    do 
    {
        lsm6ds3tr_c_reset_get(&dev_ctx_lsm, &rst);
        if (rst)
        {
            LOG_D(TAG, "Reset in progress");
        }
    } while (rst);

    /* Enable Block Data Update */
    lsm6ds3tr_c_block_data_update_set(&dev_ctx_lsm, PROPERTY_ENABLE);
    LOG_D(TAG, "Block Data Update enabled");
    
    /* Set Output Data Rate */
    lsm6ds3tr_c_xl_data_rate_set(&dev_ctx_lsm, LSM6DS3TR_C_XL_ODR_1k66Hz);
    lsm6ds3tr_c_gy_data_rate_set(&dev_ctx_lsm, LSM6DS3TR_C_GY_ODR_1k66Hz);
    LOG_D(TAG, "ODR: 1.66 kHz");
    
    /* Set full scale */
    lsm6ds3tr_c_xl_full_scale_set(&dev_ctx_lsm, LSM6DS3TR_C_2g);
    lsm6ds3tr_c_gy_full_scale_set(&dev_ctx_lsm, LSM6DS3TR_C_2000dps);
    LOG_D(TAG, "Full scale: ±2g accel, ±2000dps gyro");
    
    /* Configure filtering chain(No aux interface) */
    /* Accelerometer - analog filter */
    lsm6ds3tr_c_xl_filter_analog_set(&dev_ctx_lsm, LSM6DS3TR_C_XL_ANA_BW_400Hz);
    /* Accelerometer - LPF1 + LPF2 path */
    lsm6ds3tr_c_xl_lp2_bandwidth_set(&dev_ctx_lsm, LSM6DS3TR_C_XL_LOW_NOISE_LP_ODR_DIV_100);
    LOG_D(TAG, "Accel filters: 400Hz analog, LPF");
    
    /* Gyroscope - filtering chain */
    lsm6ds3tr_c_gy_band_pass_set(&dev_ctx_lsm, LSM6DS3TR_C_HP_260mHz_LP1_STRONG);
    LOG_D(TAG, "Gyro filter: bandpass 260mHz");
    
    LOG_I(TAG, "LSM6DS3TR-C init complete");
}

/**
 * @brief   Get accelerometer, gyroscope, and temperature data
 * @details Reads current acceleration (X,Y,Z), angular rate (X,Y,Z),
 *          and sensor temperature. Converts RAW values to physical units
 * @return  lib_acc_gyr_data_t structure with data and freshness flags
 * @note    Function automatically checks register status before reading
 */
lib_acc_gyr_data_t lib_acc_gyr_get()
{
    lib_acc_gyr_data_t ret_data;
    int16_t data_raw_acceleration[3];
    int16_t data_raw_angular_rate[3];
    int16_t data_raw_temperature;
    
    lsm6ds3tr_c_status_reg_get(&dev_ctx_lsm, &reg.status_reg);
    
    if (reg.status_reg.xlda) 
    {
        memset(&ret_data, 0x00, sizeof(ret_data));
        lsm6ds3tr_c_acceleration_raw_get(&dev_ctx_lsm, data_raw_acceleration);
        ret_data.acc.x = lsm6ds3tr_c_from_fs2g_to_ug(data_raw_acceleration[0]);
        ret_data.acc.y = lsm6ds3tr_c_from_fs2g_to_ug(data_raw_acceleration[1]);
        ret_data.acc.z = lsm6ds3tr_c_from_fs2g_to_ug(data_raw_acceleration[2]);
        ret_data.is_new.acc = reg.status_reg.xlda;
        LOG_D(TAG, "Accel X=%ld Y=%ld Z=%ld uG", ret_data.acc.x, ret_data.acc.y, ret_data.acc.z);
    }

    if (reg.status_reg.gda) 
    {
        memset(data_raw_angular_rate, 0x00, 3 * sizeof(int16_t));
        lsm6ds3tr_c_angular_rate_raw_get(&dev_ctx_lsm, data_raw_angular_rate);
        ret_data.gyr.x = lsm6ds3tr_c_from_fs2000dps_to_udps(data_raw_angular_rate[0]);
        ret_data.gyr.y = lsm6ds3tr_c_from_fs2000dps_to_udps(data_raw_angular_rate[1]);
        ret_data.gyr.z = lsm6ds3tr_c_from_fs2000dps_to_udps(data_raw_angular_rate[2]);                       
        ret_data.is_new.gyr = reg.status_reg.gda;
        LOG_D(TAG, "Gyro X=%ld Y=%ld Z=%ld udps", ret_data.gyr.x, ret_data.gyr.y, ret_data.gyr.z);
    }

    if (reg.status_reg.tda) 
    {
        memset(&data_raw_temperature, 0x00, sizeof(int16_t));
        lsm6ds3tr_c_temperature_raw_get(&dev_ctx_lsm, &data_raw_temperature);
        ret_data.temperature = lsm6ds3tr_c_from_lsb_to_celsius_x10(data_raw_temperature);    
        ret_data.is_new.tmp = reg.status_reg.tda;
        LOG_D(TAG, "Temperature: %d (x10 degC)", ret_data.temperature);
    }
                
    
    return ret_data;
}

/********************************************************************************
 **********                        PRIVATE FUNCTIONS                  ***********
*********************************************************************************/

/**
 * @brief   Write data to LSM6DS3TR-C sensor registers
 * @param   handle      Pointer to device structure (unused)
 * @param   reg         Starting register address
 * @param   bufp        Pointer to buffer with data to write
 * @param   len         Number of bytes to write
 * @return  0 on success
 * @details Writes len bytes starting from register address reg. Uses I2C
 *          with device address 0x6A. Requires counter for each byte write
 * @note    Blocking function - waits for I2C operation completion
 */
static int32_t platform_write_lsm(void* handle, uint8_t reg, const uint8_t *bufp, uint16_t len)
{
    (void) handle;
    
    for(uint8_t i = 0; i < len; i++)
    {
        uint8_t dat[2] = {(reg + i), bufp[i]};
        (void) I2C_I2CMasterWriteBuf(LSM6DS3TR_C_I2C_ADDR,
                                           dat,
                                           2,
                                           I2C_I2C_MODE_COMPLETE_XFER);
        while (I2C_I2CMasterStatus() & I2C_I2C_MSTAT_XFER_INP) {}
        while ((I2C_I2CMasterStatus() & I2C_I2C_MSTAT_RD_CMPLT) == 0) {}
    }

    return 0;
}

/**
 * @brief   Read data from LSM6DS3TR-C sensor registers
 * @param   handle      Pointer to device structure (unused)
 * @param   reg         Starting register address
 * @param   bufp        Pointer to buffer for storing read data
 * @param   len         Number of bytes to read
 * @return  0 on success
 * @details Reads len bytes from register address reg. Uses I2C with device address 0x6A.
 *          First sends register address, then reads data
 * @note    Blocking function - waits for I2C operations completion (write + read)
 */
static int32_t platform_read_lsm(void* handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
    (void) handle;
    (void) I2C_I2CMasterWriteBuf(LSM6DS3TR_C_I2C_ADDR,
                                 &reg,
                                 1,
                                 I2C_I2C_MODE_COMPLETE_XFER);
    while (I2C_I2CMasterStatus() & I2C_I2C_MSTAT_XFER_INP) {}
    while ((I2C_I2CMasterStatus() & I2C_I2C_MSTAT_WR_CMPLT) == 0) {}

    (void) I2C_I2CMasterReadBuf(LSM6DS3TR_C_I2C_ADDR,
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
static void platform_delay_lsm(uint32_t delay)
{
    CyDelay(delay);
}

/* [] END OF FILE */
