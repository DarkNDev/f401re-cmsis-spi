#include "mcp23x17.h"
#include "spi1_init.h"
#include "tim2_init.h"

volatile uint32_t system_millis = 0;

uint8_t SPI1_Transfer(uint8_t data) {
  // Wait until Transmit buffer is empty (TXE = 1)
  while (!(SPI1->SR & SPI_SR_TXE))
    ;

  // Write data to Data Register to start clock generation
  //*((__IO uint8_t *)&SPI1->DR) = data;
  SPI1->DR = data;

  // Wait until Receive buffer is not empty (RXNE = 1)
  while (!(SPI1->SR & SPI_SR_RXNE))
    ;

  // Return received byte (this also clears the RXNE flag)
  // return *((__IO uint8_t *)&SPI1->DR);
  return SPI1->DR;
}

void MCP23S17_WriteReg(uint8_t reg, uint8_t value) {
  // Pull CS (PA4) LOW to select the chip
  GPIOA->BSRR = GPIO_BSRR_BR4;

  // Send Write Opcode (0x40), Register Address, and Data
  SPI1_Transfer(MCP23S17_SPI_WRITE);
  SPI1_Transfer(reg);
  SPI1_Transfer(value);

  // Wait until SPI is no longer busy before releasing CS
  while (SPI1->SR & SPI_SR_BSY)
    ;

  // Pull CS (PA4) HIGH to end transaction
  GPIOA->BSRR = GPIO_BSRR_BS4;
}

void TIM2_IRQHandler(void) {
  if (TIM2->SR & TIM_SR_UIF) {
    TIM2->SR &= ~TIM_SR_UIF;

    // task
    system_millis++;
  }
}

int main(void) {
  TIM2_Init();
  SPI1_Init();

  MCP23S17_WriteReg(MCP_IODIRA, 0);

  volatile uint8_t state = 0;
  volatile uint32_t last_blink = 0;

  while (1) {
    volatile uint32_t now = millis();

    if ((now - last_blink) >= 1000) {
      last_blink = now;
      state ^= MCP_GP0;
      MCP23S17_WriteReg(MCP_GPIOA, state); // 00000001
    }
  }
}