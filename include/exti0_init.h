#ifndef EXTI0_INIT_H
#define EXTI0_INIT_H

#include "stm32f401xe.h"

static inline void EXTI0_Init(void) {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

  GPIOA->MODER &= ~(0x3U << (0 * 2U));
  GPIOA->MODER |= (0U << (0 * 2U));

  GPIOA->PUPDR &= ~(0x3U << (0 * 2U));
  GPIOA->PUPDR |= (1U << (0 * 2U));

  SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;
  SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PA;

  EXTI->IMR |= EXTI_IMR_MR0;

  EXTI->FTSR |= EXTI_FTSR_TR0;
  EXTI->RTSR &= ~EXTI_RTSR_TR0;

  NVIC_SetPriority(EXTI0_IRQn, 2);
  NVIC_EnableIRQ(EXTI0_IRQn);
}

/*

Define ISR in main.c:

void EXTI0_IRQHandler(void) {
  if (EXTI->PR & EXTI_PR_PR0) {
    EXTI->PR = EXTI_PR_PR0; // Clear EXTI0 pending bit (write 1)

    //task
  }
}

*/

#endif /* EXTI0_INIT_H */