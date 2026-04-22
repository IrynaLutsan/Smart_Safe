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

#define NOTE_C4  262u
#define NOTE_D4  294u
#define NOTE_E4  330u
#define NOTE_F4  349u
#define NOTE_G4  392u
#define NOTE_A4  440u
#define NOTE_B4  494u
#define NOTE_C5  523u
#define NOTE_D5  587u
#define NOTE_E5  659u
#define NOTE_F5  698u
#define NOTE_G5  784u

#define BUZZER_PWM_CLOCK_HZ 64000UL

typedef struct
{
    uint16_t freq;      /**< 0 = silence. */
    uint16_t tone_ms;
    uint16_t pause_ms;
} BuzzerNote;

typedef enum { BZ_IDLE, BZ_TONE, BZ_PAUSE } BuzzerPhase;

/* ---- built-in sequences ---- */

static const BuzzerNote MELODY[] =
{
    {NOTE_G4, 150u, 50u}, {NOTE_A4, 150u, 50u}, {NOTE_G4, 150u, 50u},
    {NOTE_E4, 150u, 100u}, {NOTE_C5, 200u, 100u},
    {NOTE_G4, 150u, 50u}, {NOTE_A4, 150u, 50u}, {NOTE_G4, 150u, 50u},
    {NOTE_E4, 150u, 100u}, {NOTE_C5, 200u, 200u}
};

static const BuzzerNote BEEP_SUCCESS[] =
{
    {NOTE_C5, 100u, 50u}, {NOTE_E5, 100u, 50u}, {NOTE_G5, 150u, 0u}
};

static const BuzzerNote BEEP_ERROR[] =
{
    {NOTE_G4, 150u, 50u}, {NOTE_E4, 150u, 50u}, {NOTE_C4, 200u, 0u}
};

static const BuzzerNote BEEP_KEYPRESS[] =
{
    {NOTE_C5, 50u, 0u}
};

/* ---- state ---- */

static BuzzerPhase       g_phase    = BZ_IDLE;
static const BuzzerNote* g_seq      = NULL;
static uint8_t           g_seq_len  = 0u;
static uint8_t           g_seq_idx  = 0u;
static uint32_t          g_deadline = 0u;
static BuzzerNote        g_tone_buf;   /* scratch buffer for lib_buzzer_tone() */


/********************************************************************************
 **********                        PRIVATE FUNCTIONS                  ***********
*********************************************************************************/

static void bz_write_freq(uint16_t freq)
{
    if (freq == 0u)
    {
        PWM1_BUZZER_WriteCompare(0u);
        return;
    }
    uint32_t period = (BUZZER_PWM_CLOCK_HZ / (uint32_t)freq) - 1u;
    if (period > 255u) period = 255u;
    if (period < 1u)   period = 1u;
    PWM1_BUZZER_WritePeriod((uint8_t)period);
    PWM1_BUZZER_WriteCompare((uint8_t)(period / 2u));
}

static void bz_start_note(uint32_t now_ms)
{
    bz_write_freq(g_seq[g_seq_idx].freq);
    g_deadline = now_ms + (uint32_t)g_seq[g_seq_idx].tone_ms;
}

static void bz_queue(const BuzzerNote* seq, uint8_t len)
{
    g_seq     = seq;
    g_seq_len = len;
    g_seq_idx = 0u;
    g_phase   = BZ_IDLE;  /* tick picks it up on the next call */
}


/********************************************************************************
 **********                         PUBLIC FUNCTIONS                  ***********
*********************************************************************************/

void lib_buzzer_init(void)
{
    PWM1_BUZZER_Init();
    PWM1_BUZZER_Enable();
    PWM1_BUZZER_Start();
    PWM1_BUZZER_WriteCompare(0u);
    LOG_I(TAG, "Buzzer initialized");
}

void lib_buzzer_start(uint16_t frequency)
{
    g_phase   = BZ_IDLE;
    g_seq     = NULL;
    g_seq_len = 0u;
    bz_write_freq(frequency);
    LOG_D(TAG, "Continuous tone %d Hz", frequency);
}

void lib_buzzer_stop(void)
{
    g_phase   = BZ_IDLE;
    g_seq     = NULL;
    g_seq_len = 0u;
    PWM1_BUZZER_WriteCompare(0u);
}

void lib_buzzer_tick(uint32_t now_ms)
{
    switch (g_phase)
    {
        case BZ_IDLE:
            if (g_seq != NULL && g_seq_len > 0u)
            {
                bz_start_note(now_ms);
                g_phase = BZ_TONE;
            }
            break;

        case BZ_TONE:
            if (now_ms >= g_deadline)
            {
                PWM1_BUZZER_WriteCompare(0u);  /* silence between notes */

                if (g_seq[g_seq_idx].pause_ms == 0u)
                {
                    /* No inter-note pause — advance immediately */
                    g_seq_idx++;
                    if (g_seq_idx >= g_seq_len)
                    {
                        g_phase = BZ_IDLE;
                        g_seq   = NULL;
                    }
                    else
                    {
                        bz_start_note(now_ms);
                        /* stay in BZ_TONE */
                    }
                }
                else
                {
                    g_deadline = now_ms + (uint32_t)g_seq[g_seq_idx].pause_ms;
                    g_phase    = BZ_PAUSE;
                }
            }
            break;

        case BZ_PAUSE:
            if (now_ms >= g_deadline)
            {
                g_seq_idx++;
                if (g_seq_idx >= g_seq_len)
                {
                    g_phase = BZ_IDLE;
                    g_seq   = NULL;
                }
                else
                {
                    bz_start_note(now_ms);
                    g_phase = BZ_TONE;
                }
            }
            break;

        default:
            break;
    }
}

uint8_t lib_buzzer_is_busy(void)
{
    return (g_phase != BZ_IDLE || (g_seq != NULL && g_seq_len > 0u)) ? 1u : 0u;
}

void lib_buzzer_tone(uint16_t frequency, uint16_t duration_ms)
{
    g_tone_buf.freq     = frequency;
    g_tone_buf.tone_ms  = duration_ms;
    g_tone_buf.pause_ms = 0u;
    bz_queue(&g_tone_buf, 1u);
}

void lib_buzzer_play_melody(void)
{
    bz_queue(MELODY, (uint8_t)(sizeof(MELODY) / sizeof(MELODY[0])));
}

void lib_buzzer_beep_success(void)
{
    bz_queue(BEEP_SUCCESS, (uint8_t)(sizeof(BEEP_SUCCESS) / sizeof(BEEP_SUCCESS[0])));
}

void lib_buzzer_beep_error(void)
{
    bz_queue(BEEP_ERROR, (uint8_t)(sizeof(BEEP_ERROR) / sizeof(BEEP_ERROR[0])));
}

void lib_buzzer_beep_keypress(void)
{
    bz_queue(BEEP_KEYPRESS, (uint8_t)(sizeof(BEEP_KEYPRESS) / sizeof(BEEP_KEYPRESS[0])));
}

/* [] END OF FILE */
