/*
MCP23S17 Switch and LED with SPI interrupt and Ring Buffer
(Complete Non-blocking)
*/
#include "exti0_init.h"
#include "mcp23x17.h"
#include "spi1_init.h"
#include <stdbool.h>

#define BUFFER_SIZE 64
#define BUFFER_SIZE_MASK (64 - 1)

#define SPI1_DR8 (*(volatile uint8_t *)&SPI1->DR)

typedef struct {
  uint8_t tx[3];
  volatile uint8_t rx[3];
  volatile uint8_t tx_index;
  volatile uint8_t rx_index;
} SPI_t;

typedef struct {
  SPI_t spi[BUFFER_SIZE];
  volatile uint32_t head;
  volatile uint32_t tail;
} SPI_Buffer_t;

SPI_Buffer_t spi_buffer = {0};

volatile bool busy = false;
int pending = -1;

static inline void SPI1_KickStart(uint8_t data) {
  busy = true;
  GPIOA->BSRR = GPIO_BSRR_BR4;
  SPI1->CR2 |= SPI_CR2_RXNEIE;
  SPI1_DR8 = data;
}

int SPI1_StartTransfer(uint8_t b0, uint8_t b1, uint8_t b2) {
  uint32_t primask = __get_PRIMASK();
  __disable_irq();

  uint32_t head = spi_buffer.head;
  uint32_t next_head = (head + 1) & BUFFER_SIZE_MASK;

  if (next_head == spi_buffer.tail) {
    __set_PRIMASK(primask);
    return -1;
  }

  SPI_t *spi_tx = &spi_buffer.spi[head];
  spi_tx->tx[0] = b0;
  spi_tx->tx[1] = b1;
  spi_tx->tx[2] = b2;
  spi_tx->tx_index = 1;
  spi_tx->rx_index = 0;

  spi_buffer.head = next_head;

  if (!busy) {
    SPI1_KickStart(b0);
  }

  __set_PRIMASK(primask);
  return (int)head;
}


void SPI1_IRQHandler(void) {
  if ((SPI1->SR & SPI_SR_RXNE) && (SPI1->CR2 & SPI_CR2_RXNEIE)) {

    SPI_t *spi_rx = &spi_buffer.spi[spi_buffer.tail];
    spi_rx->rx[spi_rx->rx_index] = SPI1_DR8;
    spi_rx->rx_index++;

    if (spi_rx->rx_index < 3) {
      SPI1_DR8 = spi_rx->tx[spi_rx->tx_index++];
    } else {
      // frame complete
      while (SPI1->SR & SPI_SR_BSY)
        ;
      GPIOA->BSRR = GPIO_BSRR_BS4;

      spi_buffer.tail = (spi_buffer.tail + 1) & BUFFER_SIZE_MASK;

      if (spi_buffer.tail != spi_buffer.head) {
        // more queued: chain straight into the next frame
        SPI1_KickStart(spi_buffer.spi[spi_buffer.tail].tx[0]);
      } else {
        SPI1->CR2 &= ~SPI_CR2_RXNEIE;
        busy = false;
      }
    }
  }
}

static inline bool MCP23S17_WriteEnqueue(uint8_t reg, uint8_t value) {
  return SPI1_StartTransfer(MCP23S17_SPI_WRITE, reg, value) >= 0;
}

static inline int MCP23S17_ReadEnqueue(uint8_t reg) {
  return SPI1_StartTransfer(MCP23S17_SPI_READ, reg, 0x00);
}

/* static inline bool MCP23S17_ReadDone(int slot) {
  return spi_buffer.spi[slot].rx_index >= 3;
}

static inline uint8_t MCP23S17_ReadResult(int slot) {
  return spi_buffer.spi[slot].rx[2];
} */

void EXTI0_IRQHandler(void) {
  if (EXTI->PR & EXTI_PR_PR0) {
    EXTI->PR = EXTI_PR_PR0; // Clear EXTI0 pending bit (write 1)

    // task
    (void)MCP23S17_ReadEnqueue(MCP_INTCAPB);
    GPIOA->ODR ^= GPIO_ODR_OD1;
  }
}

int main(void) {
  SPI1_Init();
  EnableSPI1_Interrupt();

  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  
  GPIOA->MODER &= ~(0x3U << (1 * 2U));
  GPIOA->MODER |=  (1U << (1 * 2U));

  MCP23S17_WriteEnqueue(MCP_IODIRB, MCP_GP3);
  MCP23S17_WriteEnqueue(MCP_GPPUB, MCP_GP3);
  MCP23S17_WriteEnqueue(MCP_GPINTENB, MCP_GP3);
  MCP23S17_WriteEnqueue(MCP_INTCONB, MCP_GP3);
  MCP23S17_WriteEnqueue(MCP_DEFVALB, MCP_GP3);

  EXTI0_Init();

  while (1) {
    __WFI();
  }
}