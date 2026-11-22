/*
MCP23S17 Switch and LED with SPI interrupt
(Partial Non-blocking)
*/
#include "mcp23x17.h"
#include "spi1_init.h"

#define BUFFER_SIZE 3

typedef struct {
  uint8_t tx[BUFFER_SIZE];
  uint8_t rx[BUFFER_SIZE];
  volatile uint8_t tx_idx;
  volatile uint8_t rx_idx;
  volatile uint8_t busy;
} SPI_t;

SPI_t spi = {0};

volatile uint8_t data = 0;

// Starts a non-blocking 3-byte SPI transfer
void SPI1_StartTransfer(uint8_t b0, uint8_t b1, uint8_t b2) {
  while (spi.busy)
    ;

  spi.busy = 1;
  spi.tx[0] = b0;
  spi.tx[1] = b1;
  spi.tx[2] = b2;
  spi.rx_idx = 0;

  GPIOA->BSRR = GPIO_BSRR_BR4; // CS low
  SPI1->CR2 |= SPI_CR2_RXNEIE;
  SPI1->DR = spi.tx[0]; // kick off byte 0
}

void SPI1_IRQHandler(void) {
  if ((SPI1->SR & SPI_SR_RXNE) && (SPI1->CR2 & SPI_CR2_RXNEIE)) {
    uint8_t i = spi.rx_idx;
    spi.rx[i++] = (uint8_t)SPI1->DR;
    spi.rx_idx = i;

    if (i < BUFFER_SIZE) {
      SPI1->DR = spi.tx[i]; // next byte only after this one arrived
    } else {
      SPI1->CR2 &= ~SPI_CR2_RXNEIE;
      while (SPI1->SR & SPI_SR_BSY)
        ;
      GPIOA->BSRR = GPIO_BSRR_BS4; // CS high
      spi.busy = 0;
    }
  }
}

void MCP23S17_WriteReg(uint8_t reg, uint8_t value) {
  SPI1_StartTransfer(MCP23S17_SPI_WRITE, reg, value);
}

uint8_t MCP23S17_ReadReg(uint8_t reg) {
  SPI1_StartTransfer(MCP23S17_SPI_READ, reg, 0x00);

  while (spi.busy)
    ;

  return spi.rx[2];
}

int main(void) {
  SPI1_Init();
  EnableSPI1_Interrupt();

  MCP23S17_WriteReg(MCP_IODIRB, 0b00000100);
  while (spi.busy)
    ;
  MCP23S17_WriteReg(MCP_GPPUB, 0b00000100);
  while (spi.busy)
    ;
  MCP23S17_WriteReg(MCP_IODIRA, 0b00000000);
  while (spi.busy)
    ;

  while (1) {
    data = MCP23S17_ReadReg(MCP_GPIOB);
    while (spi.busy)
      ;

    if (data & 0b00000100) {
      MCP23S17_WriteReg(MCP_GPIOA, 0b00000000);
    } else {
      MCP23S17_WriteReg(MCP_GPIOA, 0b00000100);
    }
    while (spi.busy)
      ;
  }
}