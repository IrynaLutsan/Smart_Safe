/**
 * @file lib_buzzer.h
 * @brief Buzzer control interface.
 */

#ifndef LIB_BUZZER_H
#define LIB_BUZZER_H

#include "lib_common_types.h"

/**
 * @brief Initialize the buzzer hardware.
 */
void lib_buzzer_init(void);

/**
 * @brief Play a tone on the buzzer.
 *
 * @param frequency Frequency in Hertz.
 * @param duration_ms Duration in milliseconds.
 */
void lib_buzzer_tone(uint16_t frequency, uint16_t duration_ms);

/**
 * @brief Play a predefined melody.
 */
void lib_buzzer_play_melody(void);

/**
 * @brief Play a short success beep pattern.
 */
void lib_buzzer_beep_success(void);

/**
 * @brief Play an error beep sequence.
 */
void lib_buzzer_beep_error(void);

/**
 * @brief Play a key press feedback beep.
 */
void lib_buzzer_beep_keypress(void);

#endif // LIB_BUZZER_H

/* [] END OF FILE */
