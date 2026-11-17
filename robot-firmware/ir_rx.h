/*
 * ir_rx.h - Dock beacon receiver for NUCLEO-G071RB
 *
 * Two TSOP38238 on TIM3 input capture (both edges, PSC=15 -> 1 us ticks,
 * ARR=0xFFFF free-running). Left/right are as seen looking rearward from the
 * robot toward the dock.
 *
 * Frame format (same for time-multiplexed and OR-able schemes):
 *   8 bits, MSB first, 2 ms per bit
 *   "1" = 1.5 ms burst + 0.5 ms gap, "0" = 0.5 ms burst + 1.5 ms gap
 *   >= 10 ms gap between frames
 *
 * Codes: base 0xA0, one flag bit per dock channel
 *   L = 0xA8, R = 0xA4, N = 0xA1  (OR-merged frames like 0xAC, 0xAD are valid)
 *
 * The decoder keeps a "last seen" time per flag, so both schemes work unchanged:
 *   time-multiplexed: L and R arrive in separate frames within the window
 *   OR-able:          L and R arrive together in one 0xAC frame
 */
#ifndef IR_RX_H
#define IR_RX_H

#include "stm32g0xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#define IR_FLAG_L 0x08u
#define IR_FLAG_R 0x04u
#define IR_FLAG_N 0x01u

typedef enum {
    IR_RX_LEFT = 0,  /* PA7, TIM3_CH2 - left looking rearward  */
    IR_RX_RIGHT,     /* PA6, TIM3_CH1 - right looking rearward */
    IR_RX_COUNT
} ir_rx_id_t;

typedef struct {
    uint8_t  flags;          /* IR_FLAG_* seen within the window */
    uint8_t  last_code;      /* last valid frame */
    uint32_t frames_ok;
    uint32_t frames_bad;     /* malformed frames or out-of-range pulses */
    uint32_t last_frame_ms;  /* HAL tick of last valid frame */
} ir_rx_status_t;

void ir_rx_init(TIM_HandleTypeDef *htim);
void ir_rx_capture_isr(TIM_HandleTypeDef *htim);   /* call from HAL_TIM_IC_CaptureCallback */
void ir_rx_get_status(ir_rx_id_t id, ir_rx_status_t *out);

#endif /* IR_RX_H */