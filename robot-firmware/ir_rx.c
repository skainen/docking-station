/*
 * ir_rx.c - Dock beacon receiver for NUCLEO-G071RB
 */
#include "ir_rx.h"

/* Timing, in 1 us timer ticks unless noted */
#define BIT_THRESHOLD_US  1000u  /* burst > 1 ms -> 1 */
#define PULSE_MIN_US       200u  /* shorter -> glitch */
#define PULSE_MAX_US      2500u  /* longer  -> not our format */
#define FRAME_GAP_US      5000u  /* gap > 5 ms starts a new frame */
#define FRAME_GAP_MS        50u  /* catches gaps that wrap the 16-bit counter */
#define SEEN_WINDOW_MS     150u  /* flag counts as present if seen this recently */

#define NBITS_WAIT_GAP    0xFFu  /* lost sync: ignore pulses until a frame gap */

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    uint32_t      channel;

    uint16_t t_fall;
    uint16_t t_rise;
    uint32_t ms_rise;
    bool     in_burst;
    uint8_t  shift;
    uint8_t  nbits;

    uint32_t seen_ms[3];         /* L, R, N */
    uint8_t  last_code;
    uint32_t frames_ok;
    uint32_t frames_bad;
    uint32_t last_frame_ms;
} rx_t;

static rx_t rx[IR_RX_COUNT] = {
    [IR_RX_LEFT]  = { .port = GPIOA, .pin = GPIO_PIN_7, .channel = TIM_CHANNEL_2 },
    [IR_RX_RIGHT] = { .port = GPIOA, .pin = GPIO_PIN_6, .channel = TIM_CHANNEL_1 },
};

static const uint8_t flag_bit[3] = { IR_FLAG_L, IR_FLAG_R, IR_FLAG_N };

void ir_rx_init(TIM_HandleTypeDef *htim)
{
    for (int i = 0; i < IR_RX_COUNT; i++) {
        rx[i].nbits = NBITS_WAIT_GAP;
    }
    HAL_TIM_IC_Start_IT(htim, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(htim, TIM_CHANNEL_2);
}

static void frame_done(rx_t *r, uint32_t now_ms)
{
    uint8_t c = r->shift;

    /* Base nibble 0xA, bit 1 unused (must be 0), at least one flag set */
    if ((c & 0xF2u) == 0xA0u && (c & (IR_FLAG_L | IR_FLAG_R | IR_FLAG_N))) {
        for (int i = 0; i < 3; i++) {
            if (c & flag_bit[i]) {
                r->seen_ms[i] = now_ms;
            }
        }
        r->last_code = c;
        r->last_frame_ms = now_ms;
        r->frames_ok++;
    } else {
        r->frames_bad++;
    }
    r->nbits = NBITS_WAIT_GAP;
}

void ir_rx_capture_isr(TIM_HandleTypeDef *htim)
{
    rx_t *r;

    if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
        r = &rx[IR_RX_LEFT];
    } else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
        r = &rx[IR_RX_RIGHT];
    } else {
        return;
    }

    uint16_t t      = (uint16_t)HAL_TIM_ReadCapturedValue(htim, r->channel);
    uint32_t now_ms = HAL_GetTick();

    /* TSOP output is active-low: pin low now means the edge was falling (burst start).
     * Bursts and gaps are >= ~300 us, so the pin can't change again before this read. */
    bool low = (HAL_GPIO_ReadPin(r->port, r->pin) == GPIO_PIN_RESET);

    if (low) {
        uint16_t gap = (uint16_t)(t - r->t_rise);
        if ((now_ms - r->ms_rise) > FRAME_GAP_MS || gap > FRAME_GAP_US) {
            r->nbits = 0;
            r->shift = 0;
        }
        r->t_fall   = t;
        r->in_burst = true;
        return;
    }

    if (!r->in_burst) {
        return;
    }
    r->in_burst = false;

    uint16_t width = (uint16_t)(t - r->t_fall);
    r->t_rise  = t;
    r->ms_rise = now_ms;

    if (r->nbits == NBITS_WAIT_GAP) {
        return;
    }

    if (width < PULSE_MIN_US || width > PULSE_MAX_US) {
        r->frames_bad++;
        r->nbits = NBITS_WAIT_GAP;
        return;
    }

    r->shift = (uint8_t)((r->shift << 1) | (width > BIT_THRESHOLD_US ? 1u : 0u));
    if (++r->nbits == 8u) {
        frame_done(r, now_ms);
    }
}

void ir_rx_get_status(ir_rx_id_t id, ir_rx_status_t *out)
{
    uint32_t now = HAL_GetTick();

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    rx_t c = rx[id];
    __set_PRIMASK(primask);

    uint8_t f = 0;
    for (int i = 0; i < 3; i++) {
        if (c.seen_ms[i] != 0u && (now - c.seen_ms[i]) < SEEN_WINDOW_MS) {
            f |= flag_bit[i];
        }
    }

    out->flags         = f;
    out->last_code     = c.last_code;
    out->frames_ok     = c.frames_ok;
    out->frames_bad    = c.frames_bad;
    out->last_frame_ms = c.last_frame_ms;
}