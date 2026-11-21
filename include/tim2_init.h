#ifndef TIM2_INIT_H
#define TIM2_INIT_H

#include "stm32f401xe.h"
#include <stdint.h>

extern volatile uint32_t system_millis;

static inline void TIM2_Init(void) {
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

  // TIM2->PSC = 16000 - 1; // Counter ticks at 1 kHz (1 ms period)
  // TIM2->ARR = 1000 - 1;  // Overflows every 1000 ms (1 Hz)

  TIM2->PSC = 16 - 1;   // 16 MHz / 16 = 1 MHz counter (1 µs ticks)
  TIM2->ARR = 1000 - 1; // overflow every 1000 µs = 1 ms

  TIM2->SR &= ~TIM_SR_UIF;
  TIM2->DIER |= TIM_DIER_UIE;

  NVIC_EnableIRQ(TIM2_IRQn);
  NVIC_SetPriority(TIM2_IRQn, 2);

  TIM2->CR1 |= TIM_CR1_CEN;
}

static inline uint32_t millis(void) { return system_millis; }

static inline void delay_ms(uint32_t ms) {
  uint32_t start = millis();
  while ((millis() - start) < ms)
    ;
}

/*

Define ISR and system_millis in main.c:

volatile uint32_t system_millis = 0;

void TIM2_IRQHandler(void) {
  if (TIM2->SR & TIM_SR_UIF) {
    TIM2->SR &= ~TIM_SR_UIF;

    // task
    system_millis++;
  }
}

*/

#endif /* TIM2_INIT_H */