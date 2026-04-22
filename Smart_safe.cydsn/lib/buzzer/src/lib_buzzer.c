/********************************************************************************
 **********                           INCLUDE FILES                   ***********
*********************************************************************************/
#include "lib_buzzer.h"
#include "project.h"
#include "PWM1_BUZZER.h"

#define LOG_LEVEL LOG_LEVEL_INFO
#define TAG "BUZZER"
#include "log_dbg.h"


/********************************************************************************
 **********                        PRIVATE DEFINITIONS                ***********
*********************************************************************************/
// Musical note frequencies (in Hz)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_Eb4 311
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_Bb4 466
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784

// Default tone duration in milliseconds
#define DEFAULT_TONE_DURATION_MS 200

// PWM clock input for buzzer, use ~64 kHz source for correct musical notes
// If you leave the clock at 1 MHz, notes will sound very high.
#define BUZZER_PWM_CLOCK_HZ 64000UL


/********************************************************************************
 **********                         PUBLIC FUNCTIONS                  ***********
*********************************************************************************/

/**
 * @brief   Initialize buzzer PWM module
 * @details Initializes and starts the PWM_BUZZER component for tone generation
 * @return  void
 * @note    Must be called before using lib_buzzer_tone function
 */
void lib_buzzer_init(void)
{
    PWM1_BUZZER_Init();
    PWM1_BUZZER_Enable();
    PWM1_BUZZER_Start();
    PWM1_BUZZER_WriteCompare(0);  // Start with buzzer off
    
    LOG_I(TAG, "Buzzer initialized");
}

/**
 * @brief   Play a tone at specified frequency for given duration
 * @param   frequency   Tone frequency in Hz (0 = silence)
 * @param   duration_ms Duration to play the tone in milliseconds
 * @details Generates a square wave tone using PWM at the specified frequency
 *          and plays it for the given duration. Function blocks during playback
 * @return  void
 * @note    Frequency range: 100-800 Hz recommended for 64 kHz / 8-bit PWM output. Duration: 50-5000 ms typical
 */
void lib_buzzer_tone(uint16_t frequency, uint16_t duration_ms)
{
    if(frequency == 0)
    {
        CyDelay(duration_ms);
        return;
    }
    
    uint32_t pwm_period = (BUZZER_PWM_CLOCK_HZ / frequency) - 1;
    
    if(pwm_period > 255) pwm_period = 255;
    if(pwm_period < 1) pwm_period = 1;
    
    uint8_t pwm_compare = pwm_period / 2;
    
    PWM1_BUZZER_WritePeriod((uint8_t)pwm_period);
    PWM1_BUZZER_WriteCompare(pwm_compare);
    
    LOG_D(TAG, "Playing tone at %d Hz for %d ms (Period: %d, Compare: %d)", 
          frequency, duration_ms, pwm_period, pwm_compare);
    
    CyDelay(duration_ms);
    
    PWM1_BUZZER_WriteCompare(0);
}

/**
 * @brief   Play a simple Ukrainian melody
 * @details Plays a short Ukrainian-inspired melody using predefined note frequencies.
 * @return  void
 * @note    Melody expects the PWM clock to be ~64 kHz and 8-bit PWM resolution.
 */
void lib_buzzer_play_melody(void)
{
    lib_buzzer_tone(NOTE_G4, 150);
    CyDelay(50);
    lib_buzzer_tone(NOTE_A4, 150);
    CyDelay(50);
    lib_buzzer_tone(NOTE_G4, 150);
    CyDelay(50);
    lib_buzzer_tone(NOTE_E4, 150);
    CyDelay(100);
    lib_buzzer_tone(NOTE_C5, 200);
    CyDelay(100);
    lib_buzzer_tone(NOTE_G4, 150);
    CyDelay(50);
    lib_buzzer_tone(NOTE_A4, 150);
    CyDelay(50);
    lib_buzzer_tone(NOTE_G4, 150);
    CyDelay(50);
    lib_buzzer_tone(NOTE_E4, 150);
    CyDelay(100);
    lib_buzzer_tone(NOTE_C5, 200);
    CyDelay(200);
}

/**
 * @brief   Play success beep sequence
 * @details Plays a short ascending tone sequence (C5-E5-G5) to indicate successful operation
 * @return  void
 * @note    Blocking function that plays 3 short beeps with 50ms pauses between them
 */
void lib_buzzer_beep_success(void)
{
    lib_buzzer_tone(NOTE_C5, 100);
    CyDelay(50);
    lib_buzzer_tone(NOTE_E5, 100);
    CyDelay(50);
    lib_buzzer_tone(NOTE_G5, 150);
}

/**
 * @brief   Play error beep sequence
 * @details Plays a descending tone sequence (G4-E4-C4) to indicate error condition
 * @return  void
 * @note    Blocking function that plays 3 short beeps in descending order with 50ms pauses
 */
void lib_buzzer_beep_error(void)
{
    lib_buzzer_tone(NOTE_G4, 150);
    CyDelay(50);
    lib_buzzer_tone(NOTE_E4, 150);
    CyDelay(50);
    lib_buzzer_tone(NOTE_C4, 200);
}

/**
 * @brief   Play key press feedback beep
 * @details Plays a short C5 note (50ms) to provide audio feedback for key presses
 * @return  void
 * @note    Blocking function that plays a single short beep for UI feedback
 */
void lib_buzzer_beep_keypress(void)
{
    lib_buzzer_tone(NOTE_C5, 50);
}
