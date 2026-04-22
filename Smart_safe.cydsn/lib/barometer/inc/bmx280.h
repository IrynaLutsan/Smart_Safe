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
#ifndef BMP280_H_
#define BMP280_H_
    
#include "lib_common_types.h"
    
/********************************************************************************
 **********                           PUBLIC TYPES                    ***********
*********************************************************************************/
typedef void (*bmx_write_ptr)(uint8_t, uint8_t);
typedef void (*bmx_read_ptr)(uint8_t, uint8_t*, uint16_t);
typedef void (*bmx_mdelay_ptr)(uint32_t millisec);

typedef struct
{
  /** Component mandatory fields **/
  bmx_write_ptr  write_reg;
  bmx_read_ptr   read_reg;
  /** Component optional fields **/
  bmx_mdelay_ptr   mdelay;
} bmx_ctx_t;


/********************************************************************************
 **********                         PUBLIC FUNCTIONS                  ***********
*********************************************************************************/
ret_code_t bmp280_init(bmx_ctx_t ctx);
uint8_t bmp280_get_status(void);	
void bmp280_set_config(uint8_t t_sb, uint8_t filter, uint8_t spi3w_en);
void bmp280_set_ctrl(uint8_t osrs_t, uint8_t osrs_p, uint8_t mode);
void bmp280_measure(void);
int16_t bmp280_get_temperature();
uint32_t bmp280_get_pressure();


#endif /* BMP280_H_ */

/* [] END OF FILE */