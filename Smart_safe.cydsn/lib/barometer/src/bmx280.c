/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
#include "project.h"
#include "log_dbg.h"
#include "bmx280.h"


/********************************************************************************
 **********                        PRIVATE DEFINITIONS                ***********
*********************************************************************************/
#define BMP280_ADDR	0x77	// could be 0x77 or 0x76
#define BMP280_I2CINIT	

#define BMP280_ID_REG		    0xD0
#define BMP280_ID_VAL		    0x58

#define BMP280_CAL_REG_FIRST	0x88
#define BMP280_CAL_REG_LAST	    0xA1
#define BMP280_CAL_DATA_SIZE	(BMP280_CAL_REG_LAST+1 - BMP280_CAL_REG_FIRST)

#define BMP280_STATUS_REG	    0xF3
#define BMP280_CONTROL_REG	    0xF4
#define BMP280_CONFIG_REG	    0xF5

#define BMP280_PRES_REG		    0xF7
#define BMP280_TEMP_REG		    0xFA
#define BMP280_RAWDATA_BYTES	6	    // 3 bytes pressure data and 3 bytes temperature

#define bmp280_20bit_reg(b1, b2, b3)	( \
	((int32_t)(b1) << 12) \
	| ((int32_t)(b2) << 4) \
	| ((int32_t)(b3) >> 4) \
)
    

/********************************************************************************
 **********                         PRIVATE VARIABLES                 ***********
*********************************************************************************/
static int16_t bmp_temperature;
static uint32_t bmp_pressure;
static bmx_ctx_t dev_ctx;

static union _bmp280_cal_union {
	uint8_t bytes[BMP280_CAL_DATA_SIZE];
	struct {
		uint16_t t1;
		int16_t  t2;
		int16_t  t3;
		uint16_t p1;
		int16_t  p2;
		int16_t  p3;
		int16_t  p4;
		int16_t  p5;
		int16_t  p6;
		int16_t  p7;
		int16_t  p8;
		int16_t  p9;
	}dig;
} bmp280_cal;



/********************************************************************************
 **********                        FUNCTION PROTOTYPES                ***********
*********************************************************************************/

/**
 * @brief   Read BMP280 calibration coefficients from sensor
 * @details Reads factory calibration data from registers 0x88-0xA1 (24 bytes)
 *          and stores them in bmp280_cal structure for temperature/pressure compensation
 * @note    Called automatically during bmp280_init(). Required for accurate measurements
 */
static void bmp280_getcalibration(void);



/********************************************************************************
 **********                         PUBLIC FUNCTIONS                  ***********
*********************************************************************************/

/**
 * @brief   Initialize BMP280/BME280 pressure/temperature sensor
 * @param   ctx         Context structure with read/write function pointers
 * @return  RET_CODE_OK on success, RET_CODE_ERR on failure
 * @details Initializes the sensor by checking device ID, loading calibration data,
 *          and configuring measurement parameters (0.5ms delay, 16x filter, normal mode)
 * @note    Must be called before using other sensor functions
 */
ret_code_t bmp280_init(bmx_ctx_t ctx)
{
    dev_ctx = ctx;
	uint8_t buffer[1];

	// look up the ID register
	buffer[0] = 0;
	dev_ctx.read_reg(BMP280_ID_REG, buffer, 1);
	if (buffer[0] != BMP280_ID_VAL)
		return RET_CODE_ERR;

	bmp280_getcalibration();
	bmp280_set_config(0, 4, 0); // 0.5 ms delay, 16x filter, no 3-wire SPI
	bmp280_set_ctrl(2, 5, 3); // T oversample x2, P over x2, normal mode

	return RET_CODE_OK;
}

/**
 * @brief   Get BMP280 status register value
 * @return  Status register byte containing measurement and update flags
 * @details Reads the status register (0xF3) which contains information about
 *          measurement status and data update status
 * @note    Bit 3: measuring (1 = measuring), Bit 0: im_update (1 = copying data)
 */
uint8_t bmp280_get_status(void)
{
	uint8_t data[1];
	dev_ctx.read_reg(BMP280_STATUS_REG, data, 1);
	return data[0];
}

/**
 * @brief   Set BMP280 control register parameters
 * @param   osrs_t      Temperature oversampling (0-7)
 * @param   osrs_p      Pressure oversampling (0-7)
 * @param   mode        Power mode (0=sleep, 1=forced, 3=normal)
 * @details Configures temperature and pressure oversampling rates and power mode.
 *          Higher oversampling improves accuracy but increases measurement time
 * @note    Values: 0=skip, 1=x1, 2=x2, 3=x4, 4=x8, 5=x16
 */
void bmp280_set_ctrl(uint8_t osrs_t, uint8_t osrs_p, uint8_t mode)
{
	dev_ctx.write_reg(BMP280_CONTROL_REG,
		((osrs_t & 0x7) << 5)
		| ((osrs_p & 0x7) << 2)
		| (mode & 0x3)
	);
}

/**
 * @brief   Set BMP280 configuration register parameters
 * @param   t_sb        Standby time between measurements (0-7)
 * @param   filter      IIR filter coefficient (0-7)
 * @param   spi3w_en    3-wire SPI enable (0=4-wire, 1=3-wire)
 * @details Configures standby time, IIR filter, and SPI interface mode.
 *          Higher filter values reduce noise but increase response time
 * @note    t_sb: 0=0.5ms, 1=62.5ms, 2=125ms, 3=250ms, 4=500ms, 5=1000ms, 6=2000ms, 7=4000ms
 */
void bmp280_set_config(uint8_t t_sb, uint8_t filter, uint8_t spi3w_en)
{
	dev_ctx.write_reg(BMP280_CONFIG_REG,
		((t_sb & 0x7) << 5)
		| ((filter & 0x7) << 2)
		| (spi3w_en & 1)
	);
}
	
/**
 * @brief   Perform temperature and pressure measurement
 * @details Reads raw ADC data from sensor and computes calibrated temperature
 *          and pressure values using factory calibration coefficients.
 *          Results are stored in internal variables for later retrieval
 * @note    This function performs the actual measurement. Call this before
 *          reading temperature/pressure values
 */
void bmp280_measure(void)
{
	uint8_t data[BMP280_RAWDATA_BYTES];
	int32_t temp_raw, pres_raw, temp_tmp, var1, var2, t_fine;
    
	// read the raw ADC data from the I2C registers
	dev_ctx.read_reg(BMP280_PRES_REG, data, BMP280_RAWDATA_BYTES);
	pres_raw = bmp280_20bit_reg(data[0], data[1], data[2]);
	temp_raw = bmp280_20bit_reg(data[3], data[4], data[5]);


	// compute the temperature
	var1 = ((((temp_raw >> 3) - ((int32_t)bmp280_cal.dig.t1 << 1)))
		    * ((int32_t)bmp280_cal.dig.t2)) >> 11;
    
	var2 = (((((temp_raw >> 4) - ((int32_t)bmp280_cal.dig.t1))
		    * ((temp_raw >> 4) - ((int32_t)bmp280_cal.dig.t1))) >> 12)
		    * ((int32_t)bmp280_cal.dig.t3)) >> 14;
    
	t_fine = var1 + var2;
	temp_tmp = (t_fine * 5 + 128) >> 8;
    bmp_temperature = temp_tmp / 10;

	// compute the pressure
	var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
	var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)bmp280_cal.dig.p6);
	var2 = var2 + ((var1 * ((int32_t)bmp280_cal.dig.p5)) << 1);
	var2 = (var2 >> 2) + (((int32_t)bmp280_cal.dig.p4) << 16);
	
    var1 = (((bmp280_cal.dig.p3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3)
		    + ((((int32_t)bmp280_cal.dig.p2) * var1) >> 1)) >> 18;
    
	var1 = ((((32768 + var1)) * ((int32_t)bmp280_cal.dig.p1)) >> 15);

	if (var1 == 0) 
    {
		bmp_pressure = 0;
	} 
    else 
    {
		bmp_pressure = (((uint32_t)(((int32_t)1048576)-pres_raw)- (var2 >> 12))) * 3125;
		if (bmp_pressure < 0x80000000) 
        {
			bmp_pressure = (bmp_pressure << 1) / ((uint32_t)var1);
		} 
        else 
        {
			bmp_pressure = (bmp_pressure / (uint32_t)var1) * 2;
		}
		var1 = (((int32_t)bmp280_cal.dig.p9) * ((int32_t)(((bmp_pressure>>3) * (bmp_pressure >> 3)) >> 13))) >> 12;
		var2 = (((int32_t)(bmp_pressure >> 2)) * ((int32_t)bmp280_cal.dig.p8)) >> 13;
		bmp_pressure = (uint32_t)((int32_t)bmp_pressure + ((var1 + var2 + bmp280_cal.dig.p7) >> 4));
	}
}

/**
 * @brief   Get last measured temperature
 * @return  Temperature in 0.01°C units (divide by 100 for °C)
 * @details Returns the temperature value computed during last bmp280_measure() call
 * @note    Call bmp280_measure() first to get fresh data
 */
int16_t bmp280_get_temperature()
{
    return bmp_temperature;
}

/**
 * @brief   Get last measured pressure
 * @return  Pressure in Pa (Pascals)
 * @details Returns the pressure value computed during last bmp280_measure() call
 * @note    Call bmp280_measure() first to get fresh data
 */
uint32_t bmp280_get_pressure()
{
    return bmp_pressure;
}


/********************************************************************************
 **********                        PRIVATE FUNCTIONS                  ***********
*********************************************************************************/

/**
 * @brief   Read BMP280 calibration coefficients from sensor
 * @details Reads factory calibration data from registers 0x88-0xA1 (24 bytes)
 *          and stores them in bmp280_cal structure for temperature/pressure compensation
 * @note    Called automatically during bmp280_init(). Required for accurate measurements
 */
static void bmp280_getcalibration(void)
{
	memset(bmp280_cal.bytes, 0, sizeof(bmp280_cal));

	dev_ctx.read_reg(
		BMP280_CAL_REG_FIRST,
		bmp280_cal.bytes,
		BMP280_CAL_DATA_SIZE
	);
}

