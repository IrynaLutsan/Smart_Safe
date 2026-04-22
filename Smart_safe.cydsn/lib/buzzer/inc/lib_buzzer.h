/**
 * @file lib_buzzer.h
 * @brief Buzzer control interface — fully non-blocking.
 */

#ifndef LIB_BUZZER_H
#define LIB_BUZZER_H

#include "lib_common_types.h"

/**
 * @brief Initialize the buzzer hardware.
 */
void lib_buzzer_init(void);

/**
 * @brief Start a continuous tone immediately (non-blocking). Used for alarm.
 *
 * @param frequency Frequency in Hz (0 = silence).
 */
void lib_buzzer_start(uint16_t frequency);

/**
 * @brief Stop the buzzer and cancel any active sequence immediately.
 */
void lib_buzzer_stop(void);

/**
 * @brief Advance the note-sequence state machine. Call every main-loop tick.
 *
 * @param now_ms Current system time in milliseconds (from sys_tick_ms()).
 */
void lib_buzzer_tick(uint32_t now_ms);

/**
 * @brief Queue a single tone (non-blocking). Returns before tone plays.
 *
 * @param frequency   Frequency in Hz.
 * @param duration_ms Tone duration in ms.
 */
void lib_buzzer_tone(uint16_t frequency, uint16_t duration_ms);

/** @brief Queue the predefined melody (non-blocking). */
void lib_buzzer_play_melody(void);

/** @brief Queue success beep sequence (non-blocking). */
void lib_buzzer_beep_success(void);

/** @brief Queue error beep sequence (non-blocking). */
void lib_buzzer_beep_error(void);

/** @brief Queue key-press feedback beep (non-blocking). */
void lib_buzzer_beep_keypress(void);

/**
 * @brief Return 1 if a sequence or continuous tone is currently active, 0 otherwise.
 */
uint8_t lib_buzzer_is_busy(void);

#endif // LIB_BUZZER_H

/* [] END OF FILE */
