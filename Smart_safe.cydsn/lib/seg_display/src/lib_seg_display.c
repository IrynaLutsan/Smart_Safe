 /********************************************************************************
 **********                           INCLUDE FILES                   ***********
 *********************************************************************************/
#include "lib_seg_display.h"

#include <project.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define LOG_LEVEL LOG_LEVEL_INFO
#define TAG "SEG"
#include "log_dbg.h"


/********************************************************************************
 **********                        PRIVATE DEFINITIONS               ***********
*********************************************************************************/
#define RCLK_GPIO_Port 3
#define RCLK_Pin 1

#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0


/********************************************************************************
 **********                         PRIVATE VARIABLES                ***********
*********************************************************************************/
static uint8_t g_digits[LIB_SEG_DISPLAY_DIGITS_COUNT] = {0};

static const uint8_t LED_NUM[] = {
	0xC0, //0
	0xF9, //1
	0xA4, //2
	0xB0, //3
	0x99, //4
	0x92, //5
	0x82, //6
	0xF8, //7
	0x80, //8
	0x90, //9
	0xBF, //-
	0xFF  // blank (SEG_DIGIT_BLANK) — all segments off
};


/********************************************************************************
 **********                        FUNCTION PROTOTYPES               ***********
*********************************************************************************/
static void LATCH_CTRL(uint8_t state);
static void FourDigit74HC595_sendOneDigit(uint8_t pos, uint8_t digit, uint8_t dot);
static void _74hc595_7seg_init(void);
static void _74hc595_7seg_update(const uint8_t *digits);


/********************************************************************************
 **********                           ISR FUNCTIONS                   ***********
*********************************************************************************/
/**
 * @brief Timer interrupt handler for display multiplexing.
 */
CY_ISR(isr_7segment_interrupt_handler)
{
	_74hc595_7seg_update(g_digits);
    Timer_7segment_ClearInterrupt(Timer_7segment_INTR_MASK_TC);
}

/********************************************************************************
 **********                         PUBLIC FUNCTIONS                 ***********
*********************************************************************************/

/**
 * @brief Initialize the 7-segment display library.
 */
void lib_seg_display_init(void)
{
	LOG_I(TAG, "Initialize 7-segment display");
	_74hc595_7seg_init();
	LOG_I(TAG, "7-segment display init complete");
}

/**
 * @brief Copy new digits to the internal multiplexing buffer.
 * @param digits Pointer to 8-byte display buffer.
 */
void lib_seg_display_update(const uint8_t *digits)
{
	memcpy(g_digits, digits, LIB_SEG_DISPLAY_DIGITS_COUNT);
	LOG_D(TAG, "Display buffer updated");
}

/**
 * @brief Update one element in the internal display buffer.
 * @param index Digit index in the refresh buffer.
 * @param value Digit code to store.
 */
void lib_seg_display_update_digit(uint8_t index, uint8_t value)
{
	if (index >= LIB_SEG_DISPLAY_DIGITS_COUNT)
	{
		LOG_E(TAG, "Invalid digit index: %u", index);
		return;
	}

	g_digits[index] = value;
	LOG_D(TAG, "Digit[%u] updated to %u", index, value);
}

void lib_seg_display_clear(void)
{
	memset(g_digits, SEG_DIGIT_BLANK, LIB_SEG_DISPLAY_DIGITS_COUNT);
}

/**
 * @brief Display a number represented as a string.
 *
 * Supported characters:
 * - digits 0..9
 * - minus sign '-'
 * - decimal point '.'
 * - space ' '
 *
 * Examples: "-10.56", "0.56", "-2 -2.3", "1.2.3.4.", "-23-".
 *
 * @param num Null-terminated string to display.
 */
void FourDigit74HC595_sendNumber(char* num)
{
	LOG_D(TAG, "Display string: %s", num);
	uint8_t len = strlen(num);
	uint8_t dot = 0;

	while (len) {
		if (num[len - 1] == '.') {
			dot++;
		}
		len--;
	}

	if (strlen(num) - dot > 4) {
		FourDigit74HC595_sendOneDigit(0x01, 10, 0);
		FourDigit74HC595_sendOneDigit(0x02, 10, 0);
		FourDigit74HC595_sendOneDigit(0x03, 10, 0);
		FourDigit74HC595_sendOneDigit(0x04, 10, 0);

		len = 0;
	} else {
		len = strlen(num);
	}

	uint8_t position = 1;
	dot = 0;

	while (len) {
		if (num[len - 1] == '-') {
			FourDigit74HC595_sendOneDigit(position, 10, dot);
			dot = 0;
			position++;
		} else if (num[len - 1] == '.') {
			dot = 1;
		} else if (num[len - 1] == ' ') {
			position++;
		} else {
			FourDigit74HC595_sendOneDigit(position, num[len - 1] - 48, dot);
			dot = 0;
			position++;
		}

		len--;

		if (position > 4) {
			break;
		}
	}
}

/**
 * @brief Display a signed integer value.
 *
 * Supported range: -999..9999.
 *
 * @param num Integer value to display.
 */
void FourDigit74HC595_sendNumberInt(int16_t num)
{
	LOG_D(TAG, "Display integer: %d", num);
	uint8_t position = 1;

	if (num == 0) {
		FourDigit74HC595_sendOneDigit(1, 0, 0);
	} else if (((num < 0) && (num / 1000)) || ((num > 0) && (num / 10000))) {
		FourDigit74HC595_sendOneDigit(0x01, 10, 0);
		FourDigit74HC595_sendOneDigit(0x02, 10, 0);
		FourDigit74HC595_sendOneDigit(0x03, 10, 0);
		FourDigit74HC595_sendOneDigit(0x04, 10, 0);
		FourDigit74HC595_sendOneDigit(0x04, 10, 0);
		FourDigit74HC595_sendOneDigit(0x04, 10, 0);
		FourDigit74HC595_sendOneDigit(0x04, 10, 0);
		FourDigit74HC595_sendOneDigit(0x04, 10, 0);
	} else {
		uint8_t minus = 0;
		if (num < 0) {
			minus = 1;
			num = num * -1;
		}

		while (num) {
			FourDigit74HC595_sendOneDigit(position, num % 10, 0);
			num = num / 10;
			position++;

			if (position > 4) {
				break;
			}
		}

		if (minus) {
			FourDigit74HC595_sendOneDigit(position, 10, 0);
		}
	}
}


/********************************************************************************
 **********                        PRIVATE FUNCTIONS                 ***********
*********************************************************************************/
/**
 * @brief Configure latch GPIO state.
 * @param state Desired pin state.
 */
static inline void LATCH_CTRL( uint8_t state)
{
	SEG_74HC595_LATCH_Write(state);
}

/**
 * @brief Display a single digit on a selected position.
 *
 * @param pos Digit position (1..4).
 * @param digit Digit value (0..9), use 10 for minus sign.
 * @param dot Decimal point flag: 1 to enable, 0 to disable.
 */
static void FourDigit74HC595_sendOneDigit(uint8_t pos, uint8_t digit, uint8_t dot)
{
	(void) dot;
    static uint8 mTxBuffer[2] = {0x01, 0x02};

    mTxBuffer[1] = LED_NUM[digit];
    mTxBuffer[0] = 0xFF & ~(1 << pos);

    SPIM_SpiSetActiveSlaveSelect(SPIM_SPI_SLAVE_SELECT0);
    LATCH_CTRL(GPIO_PIN_RESET);
    SPIM_SpiUartPutArray(mTxBuffer, 2);

    while (SPIM_SpiIsBusBusy() != 0) {}

	LATCH_CTRL(GPIO_PIN_SET);
}

/**
 * @brief Initialize low-level 7-segment driver.
 */
static void _74hc595_7seg_init(void)
{
	Timer_7segment_Start();
    isr_7segment_StartEx(isr_7segment_interrupt_handler);
}

/**
 * @brief Update one multiplexed digit per call.
 * @param digits Pointer to digit buffer.
 */
static void _74hc595_7seg_update(const uint8_t *digits)
{
    static int digit_idx = 0;

    digit_idx++;
    if (digit_idx > 7)
    {
        digit_idx = 0;
    }

    FourDigit74HC595_sendOneDigit((uint8_t)digit_idx, digits[digit_idx], 0);
}

/* [] END OF FILE */