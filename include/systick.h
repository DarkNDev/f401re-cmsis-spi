#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

volatile uint32_t system_millis = 0;
void SysTick_Handler(void) { system_millis++; }
uint32_t millis(void) { return system_millis; }

void delay_ms(uint32_t ms) {
  uint32_t start = millis();
  while ((millis() - start) < 0)
    ;
}

// Use this in main.c
// SysTick_Config(SystemCoreClock / 1000);

#endif /* SYSTICK_H */