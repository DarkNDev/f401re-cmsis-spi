#include "mcp23x17.h"
#include "spi1_init.h"
#include "tim2_init.h"

#define BUFFER_SIZE 3

typedef struct {
  uint8_t spi_tx_buffer[BUFFER_SIZE];
  volatile uint8_t spi_index;
} SPI_Buffer_t;

SPI_Buffer_t spi_buffer = {.spi_index = 0};
volatile uint32_t system_millis = 0;

// Starts a non-blocking 3-byte SPI transfer
void SPI1_StartTransfer(uint8_t b0, uint8_t b1, uint8_t b2) {
  // Wait if a previous transfer is still in progress
  while (SPI1->SR & SPI_SR_BSY)
    ;

  spi_buffer.spi_tx_buffer[0] = b0;
  spi_buffer.spi_tx_buffer[1] = b1;
  spi_buffer.spi_tx_buffer[2] = b2;

  // Pull CS (PA4) LOW to select the chip
  GPIOA->BSRR = GPIO_BSRR_BR4;

  // Enable RXNE interrupt and write Byte 0 to start clock generation
  SPI1->CR2 |= SPI_CR2_RXNEIE;
  SPI1->DR = spi_buffer.spi_tx_buffer[0];
}

// Hardware SPI1 Interrupt Service Routine (handles both TX and RX)
void SPI1_IRQHandler(void) {
  if ((SPI1->SR & SPI_SR_RXNE) && (SPI1->CR2 & SPI_CR2_RXNEIE)) {
    // Dummy read: clears RXNE flag and prevents Overrun without storing
    // anything
    (void)SPI1->DR;

    spi_buffer.spi_index++;

    if (spi_buffer.spi_index < BUFFER_SIZE) {
      SPI1->DR = spi_buffer.spi_tx_buffer[spi_buffer.spi_index];
    } else {
      SPI1->CR2 &= ~SPI_CR2_RXNEIE;

      while (SPI1->SR & SPI_SR_BSY)
        ;

      GPIOA->BSRR = GPIO_BSRR_BS4;
      spi_buffer.spi_index = 0;
    }
  }
}

void TIM2_IRQHandler(void) {
  if (TIM2->SR & TIM_SR_UIF) {
    TIM2->SR &= ~TIM_SR_UIF;

    // Task
    system_millis++;
  }
}

void MCP23S17_WriteReg(uint8_t reg, uint8_t value) {
  SPI1_StartTransfer(MCP23S17_SPI_WRITE, reg, value);
}

int main(void) {
  TIM2_Init();
  SPI1_Init();
  EnableSPI1_Interrupt();

  MCP23S17_WriteReg(MCP_IODIRA, 0);

  volatile uint8_t state = 0;
  volatile uint32_t last_blink = 0;

  while (1) {
    volatile uint32_t now = millis();
    // Only trigger a new write if the previous SPI interrupt transfer finished
    if (!(SPI1->SR & SPI_SR_BSY) && (now - last_blink) >= 500) {
      last_blink = now;
      state ^= MCP_GP2;
      MCP23S17_WriteReg(MCP_GPIOA, state);
    }
  }
}