/*
 * my_uart.h
 *
 *  Created on: Mar 12, 2026
 *      Author: AMIN_STORS
 */

#ifndef INC_MY_UART_H_
#define INC_MY_UART_H_

typedef struct {
	USART_TypeDef *usart;
	uint8_t tx_buffer[8];
	uint8_t tx_index;
	volatile uint8_t tx_busy;

	uint8_t *rx_byte;
	uint8_t rx_current_byte;
	uint8_t rx_index;
	volatile uint8_t rx_busy;

	uint8_t reset_busy;
	uint8_t presence;

	uint32_t periph_clock;
	uint32_t over_sampling;
} onewire_irq_handler_t;

onewire_irq_handler_t ow_irq_handler;

void build_uart_buffer(uint8_t byte, uint8_t *buffer);
uint8_t uart_buffer_to_byte(uint8_t *buffer);

uint8_t onewire_reset(USART_TypeDef *USARTx);

void onewire_write_bit(USART_TypeDef *USARTx, uint8_t bit);
void onewire_write_byte(USART_TypeDef *USARTx, uint8_t byte);

uint8_t onewire_read_byte(USART_TypeDef *USARTx);
uint8_t onewire_read_bit(USART_TypeDef *USARTx);

void onewire_read_byte_IT(uint8_t *result);
void onewire_reset_IT(USART_TypeDef *USARTx);
void onewire_reset_IT_1(USART_TypeDef *USARTx);

void onewire_uart_init(USART_TypeDef *USARTx);
void onewire_uart_deinit(USART_TypeDef *USARTx);


void delay_us(uint32_t microseconds);
void delay_ms(uint32_t ms);

#endif /* INC_MY_UART_H_ */
