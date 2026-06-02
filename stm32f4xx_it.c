#include "main.h"
#include "my_uart.h"
#include "DS2431.h"

uint8_t rx_data_test[8];
uint8_t test = 0;

void timer_start(TIM_TypeDef *TIMx) {
    LL_TIM_SetCounter(TIMx, 0);
    LL_TIM_EnableIT_UPDATE(TIMx);
    LL_TIM_EnableCounter(TIMx);
}

void timer_stop(TIM_TypeDef *TIMx) {
    LL_TIM_DisableCounter(TIMx);
    LL_TIM_DisableIT_UPDATE(TIMx);
}

void DS2431_write_data_polling(DS2431_TypeDef *DS2431) {
    if(!onewire_reset(DS2431->usart)) {
        return;
    }

    onewire_write_byte(DS2431->usart, 0xCC);
    onewire_write_byte(DS2431->usart, 0x0F);
    onewire_write_byte(DS2431->usart, DS2431->cmd.address);
    onewire_write_byte(DS2431->usart, 0x00);

	for(int i = 0; i < 8; i++) {
		onewire_write_byte(DS2431->usart, DS2431->cmd.write_data_buffer[i]);
	}

	DS2431->crc1 = onewire_read_byte(DS2431->usart);
	DS2431->crc2 = onewire_read_byte(DS2431->usart);

    if(!onewire_reset(DS2431->usart)) {
        return;
    }

    onewire_write_byte(DS2431->usart, 0xCC);
    onewire_write_byte(DS2431->usart, 0xAA);

	for(int i = 0; i < (3 + 8); i++) {
		DS2431->scratchpad[i] = onewire_read_byte(DS2431->usart);
	}

    if(!onewire_reset(DS2431->usart)) {
        return;
    }

    onewire_write_byte(DS2431->usart, 0xCC);
    onewire_write_byte(DS2431->usart, 0x55);
    onewire_write_byte(DS2431->usart, DS2431->scratchpad[0]);
    onewire_write_byte(DS2431->usart, DS2431->scratchpad[1]);
    onewire_write_byte(DS2431->usart, DS2431->scratchpad[2]);

    for(uint32_t i = 0; i < 0x1FFFF; i++);
	DS2431->success = onewire_read_byte(DS2431->usart);

	DS2431->status = DS2431_READY;
}

void DS2431_read_data_polling(DS2431_TypeDef *DS2431) {
	if(!onewire_reset(DS2431->usart)) {
		return;
	}

	onewire_write_byte(DS2431->usart, 0xCC);
	onewire_write_byte(DS2431->usart, 0xF0);
	onewire_write_byte(DS2431->usart, DS2431->cmd.address);
	onewire_write_byte(DS2431->usart, 0x00);

	for(int i = 0; i < 8; i++) {
		DS2431->cmd.read_data_buffer[i] = onewire_read_byte(DS2431->usart);
	}
	DS2431->status = DS2431_READY;
}

void DS2431_set_write_cmd(DS2431_TypeDef *DS2431, uint8_t address, uint8_t *write_data_buffer) {
	DS2431_cmd item;

	item.address = address;
	item.cmd_t = write;
	item.write_data_buffer = write_data_buffer;

	DS2431_addcommand(DS2431, &item);
}

void DS2431_set_read_cmd(DS2431_TypeDef *DS2431, uint8_t address, uint8_t *read_data_buffer) {
	DS2431_cmd item;

	item.address = address;
	item.cmd_t = read;
	item.read_data_buffer = read_data_buffer;

	DS2431_addcommand(DS2431, &item);
}

void DS2431_addcommand(DS2431_TypeDef *DS2431, DS2431_cmd *item) {
	if(DS2431->count >= 4) {
		return;
	}

	DS2431->cmds[DS2431->tail] = *item;
	DS2431->tail = (DS2431->tail + 1) % 4;

	DS2431->count++;
}


void DS2431_init(DS2431_TypeDef *DS2431, USART_TypeDef *USARTx, DS2431_Mode mode) {
	DS2431->is_init = 0;
	DS2431->status = DS2431_INIT;
	DS2431->usart = USARTx;
	DS2431->mode = mode;

	DS2431->crc1 = 0;
	DS2431->crc2 = 0;
	DS2431->success = 0;
	DS2431->write_index = 0;

	DS2431->head = 0;
	DS2431->tail = 0;
	DS2431->count = 0;
	DS2431->busy = 0;
}

void DS2431_onewire_dma_init(DS2431_TypeDef *DS2431) {
	uint32_t dma_channel;

    // Определить параметры DMA и прерывания по USART
    if (DS2431->usart == USART2) {
        DS2431->dma = DMA1;
        DS2431->dma_tx_stream = LL_DMA_STREAM_6;
        DS2431->dma_rx_stream = LL_DMA_STREAM_5;
        dma_channel = LL_DMA_CHANNEL_4;

        NVIC_SetPriority(DMA1_Stream5_IRQn, 0);
        NVIC_EnableIRQ(DMA1_Stream5_IRQn);
        NVIC_SetPriority(DMA1_Stream6_IRQn, 0);
        NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    }
    else if (DS2431->usart == USART1) {
        DS2431->dma = DMA2;
        DS2431->dma_tx_stream = LL_DMA_STREAM_7;
        DS2431->dma_rx_stream = LL_DMA_STREAM_5;
        dma_channel = LL_DMA_CHANNEL_4;

        NVIC_SetPriority(DMA2_Stream5_IRQn, 0);
        NVIC_EnableIRQ(DMA2_Stream5_IRQn);
        NVIC_SetPriority(DMA2_Stream7_IRQn, 0);
        NVIC_EnableIRQ(DMA2_Stream7_IRQn);
    }
    else if (DS2431->usart == USART3) {
        DS2431->dma = DMA1;
        DS2431->dma_tx_stream = LL_DMA_STREAM_3;
        DS2431->dma_rx_stream = LL_DMA_STREAM_1;
        dma_channel = LL_DMA_CHANNEL_4;

        NVIC_SetPriority(DMA1_Stream1_IRQn, 0);
        NVIC_EnableIRQ(DMA1_Stream1_IRQn);
        NVIC_SetPriority(DMA1_Stream3_IRQn, 0);
        NVIC_EnableIRQ(DMA1_Stream3_IRQn);
    }
    #ifdef UART4
    else if (DS2431->usart == UART4) {
        DS2431->dma = DMA1;
        DS2431->dma_tx_stream = LL_DMA_STREAM_4;
        DS2431->dma_rx_stream = LL_DMA_STREAM_2;
        dma_channel = LL_DMA_CHANNEL_4;

        NVIC_SetPriority(DMA1_Stream2_IRQn, 0);
        NVIC_EnableIRQ(DMA1_Stream2_IRQn);
        NVIC_SetPriority(DMA1_Stream4_IRQn, 0);
        NVIC_EnableIRQ(DMA1_Stream4_IRQn);
    }
    #endif
    #ifdef UART5
    else if (DS2431->usart == UART5) {
        DS2431->dma = DMA1;
        DS2431->dma_tx_stream = LL_DMA_STREAM_7;
        DS2431->dma_rx_stream = LL_DMA_STREAM_0;
        dma_channel = LL_DMA_CHANNEL_4;

        NVIC_SetPriority(DMA1_Stream0_IRQn, 0);
        NVIC_EnableIRQ(DMA1_Stream0_IRQn);
        NVIC_SetPriority(DMA1_Stream7_IRQn, 0);
        NVIC_EnableIRQ(DMA1_Stream7_IRQn);
    }
    #endif
    #ifdef USART6
    else if (DS2431->usart == USART6) {
        DS2431->dma = DMA2;
        DS2431->dma_tx_stream = LL_DMA_STREAM_6;
        DS2431->dma_rx_stream = LL_DMA_STREAM_1;
        dma_channel = LL_DMA_CHANNEL_5;

        NVIC_SetPriority(DMA2_Stream1_IRQn, 0);
        NVIC_EnableIRQ(DMA2_Stream1_IRQn);
        NVIC_SetPriority(DMA2_Stream6_IRQn, 0);
        NVIC_EnableIRQ(DMA2_Stream6_IRQn);
    }
    #endif
    else {
        return;
    }

    // Включить тактирование DMA
    if (DS2431->dma == DMA1) {
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);
    } else {
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);
    }

    // === Настройка DMA для TX ===
    LL_DMA_SetChannelSelection(DS2431->dma, DS2431->dma_tx_stream, dma_channel);
    LL_DMA_SetDataTransferDirection(DS2431->dma, DS2431->dma_tx_stream, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetStreamPriorityLevel(DS2431->dma, DS2431->dma_tx_stream, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(DS2431->dma, DS2431->dma_tx_stream, LL_DMA_MODE_NORMAL);
    LL_DMA_SetPeriphIncMode(DS2431->dma, DS2431->dma_tx_stream, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DS2431->dma, DS2431->dma_tx_stream, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DS2431->dma, DS2431->dma_tx_stream, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DS2431->dma, DS2431->dma_tx_stream, LL_DMA_MDATAALIGN_BYTE);
    LL_DMA_DisableFifoMode(DS2431->dma, DS2431->dma_tx_stream);

    // === Настройка DMA для RX ===
    LL_DMA_SetChannelSelection(DS2431->dma, DS2431->dma_rx_stream, dma_channel);
    LL_DMA_SetDataTransferDirection(DS2431->dma, DS2431->dma_rx_stream, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetStreamPriorityLevel(DS2431->dma, DS2431->dma_rx_stream, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(DS2431->dma, DS2431->dma_rx_stream, LL_DMA_MODE_NORMAL);
    LL_DMA_SetPeriphIncMode(DS2431->dma, DS2431->dma_rx_stream, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DS2431->dma, DS2431->dma_rx_stream, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DS2431->dma, DS2431->dma_rx_stream, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DS2431->dma, DS2431->dma_rx_stream, LL_DMA_MDATAALIGN_BYTE);
    LL_DMA_DisableFifoMode(DS2431->dma, DS2431->dma_rx_stream);

    // === Подключить DMA к UART ===
    LL_USART_EnableDMAReq_TX(DS2431->usart);
    LL_USART_EnableDMAReq_RX(DS2431->usart);
}

void DS2431_cmd_handler(DS2431_TypeDef *DS2431) {
	if(DS2431->count == 0  || DS2431->busy == 1) {
		return;
	}

	DS2431->busy = 1;
	DS2431->cmd = DS2431->cmds[DS2431->head];


	if(DS2431->mode == polling) {
		if(DS2431->cmd.cmd_t == write) {
			DS2431->status = DS2431_WORK;
			DS2431_write_data_polling(DS2431);
		}

		else if(DS2431->cmd.cmd_t == read) {
			DS2431->status = DS2431_WORK;
			DS2431_read_data_polling(DS2431);
		}
	}

	else if(DS2431->mode == irq) {
		if(DS2431->cmd.cmd_t == write) {
			DS2431->status = DS2431_WORK;
			DS2431_write_IT(DS2431);
		}

		else if(DS2431->cmd.cmd_t == read) {
			DS2431->status = DS2431_WORK;
			DS2431_read_IT(DS2431);
		}

	}

	else if(DS2431->mode == dma) {
		if(DS2431->cmd.cmd_t == write) {
			DS2431->status = DS2431_WORK;
			DS2431_write_data_DMA(DS2431);
		}

		else if(DS2431->cmd.cmd_t == read) {
			DS2431->status = DS2431_WORK;
			DS2431_read_data_DMA(DS2431);
		}
	}
}

void DS2431_handler(DS2431_TypeDef *DS2431) {
	switch (DS2431->status) {
		case DS2431_INIT:
			onewire_uart_init(DS2431->usart);
			if (DS2431->mode == irq) {
				onewire_uart_irq_init(DS2431->usart);
			}
			else if(DS2431->mode == dma) {
				DS2431_onewire_dma_init(DS2431);
			}

			DS2431->is_init = 1;
			DS2431->status = DS2431_WAIT;
			break;

		case DS2431_WAIT:
			DS2431_cmd_handler(DS2431);
			break;

		case DS2431_WORK:
			break;

		case DS2431_READY:
			DS2431->head = (DS2431->head + 1) % 4;
			DS2431->count--;
			DS2431->busy = 0;
			DS2431->cmd.status = 0;
			DS2431->status = DS2431_WAIT;
			break;
	}

}

void DS2431_write_IT(DS2431_TypeDef *DS2431) {
	DS2431->write_state = WRITE_STATE_RESET_1;
	DS2431->write_index = 0;
	DS2431->read_active = 0;
	DS2431->write_active = 1;

	DS2431_write_next_IT(DS2431);
}

void DS2431_write_next_IT(DS2431_TypeDef *DS2431) {
    switch(DS2431->write_state) {
		case WRITE_STATE_RESET_1:
			if(DS2431->write_index == 0) {
				onewire_reset_IT_1(DS2431->usart);
				DS2431->write_index++;
			} else {
				onewire_reset_IT(DS2431->usart);
				DS2431->write_index = 0;
				DS2431->write_state = WRITE_STATE_SEND_CMD_1;
			}
			break;

		case WRITE_STATE_SEND_CMD_1: {
            if (DS2431->write_index == 0) {
				onewire_write_byte_IT(0xCC);
				DS2431->write_index++;
			} else if (DS2431->write_index == 1) {
				onewire_write_byte_IT(0x0F);
				DS2431->write_index++;
			} else if (DS2431->write_index == 2) {
				onewire_write_byte_IT(DS2431->cmd.address);
				DS2431->write_index++;
			} else {
				onewire_write_byte_IT(0x00);
				DS2431->write_index = 0;
				DS2431->write_state = WRITE_STATE_SEND_DATA;
			}
			break;
		}
		case WRITE_STATE_SEND_DATA:
			if(DS2431->write_index < (8 - 1)) {
				onewire_write_byte_IT(DS2431->cmd.write_data_buffer[DS2431->write_index++]);
			} else {
				onewire_write_byte_IT(DS2431->cmd.write_data_buffer[DS2431->write_index]);
				DS2431->write_index = 0;
				DS2431->write_state = WRITE_STATE_READ_CRC;
			}
			break;
		case WRITE_STATE_READ_CRC:
            if(DS2431->write_index == 0) {
                onewire_read_byte_IT(&DS2431->crc1);
                DS2431->write_index++;
            } else {
            	onewire_read_byte_IT(&DS2431->crc2);
            	DS2431->write_index = 0;
				DS2431->write_state = WRITE_STATE_RESET_2;
            }
            break;

		case WRITE_STATE_RESET_2:
			if(DS2431->write_index == 0) {
				onewire_reset_IT_1(DS2431->usart);
				DS2431->write_index++;
			} else {
				onewire_reset_IT(DS2431->usart);
				DS2431->write_index = 0;
				DS2431->write_state = WRITE_STATE_READ_SCRATCHPAD;
			}

			break;

		case WRITE_STATE_READ_SCRATCHPAD:
			if(DS2431->write_index == 0) {
				onewire_write_byte_IT(0xCC);
				DS2431->write_index++;
			}
			else if(DS2431->write_index == 1) {
				onewire_write_byte_IT(0xAA);
				DS2431->write_index++;
			}
			else if(DS2431->write_index > 1 && DS2431->write_index < 12) {
				onewire_read_byte_IT(&DS2431->scratchpad[DS2431->write_index - 2]);
				DS2431->write_index++;
			}
			else {
				DS2431->write_index = 0;
				onewire_read_byte_IT(&DS2431->scratchpad[10]);
				DS2431->write_state = WRITE_STATE_RESET_3;
			}

			break;
		case WRITE_STATE_RESET_3:
			if(DS2431->write_index == 0) {
				onewire_reset_IT_1(DS2431->usart);
				DS2431->write_index++;
			} else {
				onewire_reset_IT(DS2431->usart);
				DS2431->write_index = 0;
				DS2431->write_state = WRITE_STATE_COPY_SCRATCHPAD;
			}

			break;
        case WRITE_STATE_COPY_SCRATCHPAD:
        	if(DS2431->write_index == 0) {
    			onewire_write_byte_IT(0xCC);
				DS2431->write_index++;
        	}
        	else if(DS2431->write_index == 1) {
    			onewire_write_byte_IT(0x55);
				DS2431->write_index++;
        	}
			else if(DS2431->write_index == 2) {
				onewire_write_byte_IT(DS2431->scratchpad[0]);
				DS2431->write_index++;
			} else if(DS2431->write_index == 3) {
				onewire_write_byte_IT(DS2431->scratchpad[1]);  // TA1
				DS2431->write_index++;
			} else if(DS2431->write_index == 4) {
				onewire_write_byte_IT(DS2431->scratchpad[2]);  // TA2
				DS2431->write_state = WRITE_STATE_READ_SUCCESS;
			}
			break;
        case WRITE_STATE_READ_SUCCESS:
            for(uint32_t i = 0; i < 0xFFFFF; i++);
            onewire_read_byte_IT(&DS2431->success);

			DS2431->write_state = WRITE_STATE_DONE;
			break;

        case WRITE_STATE_DONE:
        	DS2431->status = DS2431_READY;
        	break;
    }
}


void DS2431_read_IT(DS2431_TypeDef *DS2431) {
	DS2431->write_index = 0;
	DS2431->write_state = READ_STATE_RESET;
	DS2431->read_active = 1;
	DS2431->write_active = 0;

    DS2431_read_next_IT(DS2431);
}

DS2431_read_next_IT(DS2431_TypeDef *DS2431) {
    switch(DS2431->write_state) {
    	case READ_STATE_RESET:
    		if(DS2431->write_index == 0) {
				onewire_reset_IT_1(DS2431->usart);
				DS2431->write_index++;
			} else {
				onewire_reset_IT(DS2431->usart);
				DS2431->write_index = 0;
				DS2431->write_state = READ_STATE_SEND_CMD;
			}

    		break;
    	case READ_STATE_SEND_CMD: {
            static uint8_t cmds[] = {0xCC, 0xF0, 0x20, 0x00};
            cmds[2] = DS2431->cmd.address;

			if(DS2431->write_index < 3) {
				onewire_write_byte_IT(cmds[DS2431->write_index++]);
			} else {
				DS2431->write_state = READ_STATE_READ_DATA;
				onewire_write_byte_IT(cmds[DS2431->write_index]);
				DS2431->write_index = 0;
			}
 			break;
    	}
    	case READ_STATE_READ_DATA:
    		if(DS2431->write_index < 7) {
				onewire_read_byte_IT(&DS2431->cmd.read_data_buffer[DS2431->write_index++]);
			} else {
				DS2431->write_state = READ_STATE_DONE;
				onewire_read_byte_IT(&DS2431->cmd.read_data_buffer[DS2431->write_index]);
				DS2431->write_index = 0;
			}
			break;
    	 case READ_STATE_DONE:
			DS2431->status = DS2431_READY;

			break;
    }
}

void onewire_uart_irq_handler(DS2431_TypeDef *DS2431) {
	extern onewire_irq_handler_t ow_irq_handler;
    USART_TypeDef *USARTx = ow_irq_handler.usart;

	if(LL_USART_IsActiveFlag_TC(USARTx) && LL_USART_IsEnabledIT_TC(USARTx)) {
		LL_USART_ClearFlag_TC(USARTx);

		if(ow_irq_handler.tx_busy) {
			ow_irq_handler.tx_index++;

			if(ow_irq_handler.tx_index < 8) {
				LL_USART_TransmitData8(USARTx, ow_irq_handler.tx_buffer[ow_irq_handler.tx_index]);
			}
			else {
	            LL_USART_DisableIT_TC(USARTx);
	            ow_irq_handler.tx_busy = 0;

	            if(DS2431->write_active) {
		            DS2431_write_next_IT(DS2431);
	            } else if (DS2431->read_active) {
		            DS2431_read_next_IT(DS2431);
	            }


			}
		}
	}

	if(LL_USART_IsActiveFlag_RXNE(USARTx) && LL_USART_IsEnabledIT_RXNE(USARTx)) {
        uint8_t rx_data = LL_USART_ReceiveData8(USARTx);
        LL_USART_ClearFlag_RXNE(USARTx);

		if(ow_irq_handler.rx_busy) {
            if(rx_data & 0x01) {
            	ow_irq_handler.rx_current_byte |= (1 << ow_irq_handler.rx_index);
            }

            ow_irq_handler.rx_index++;

            if(ow_irq_handler.rx_index < 8) {
			   LL_USART_TransmitData8(USARTx, 0xFF);
		   } else {
				LL_USART_DisableIT_RXNE(ow_irq_handler.usart);
			   ow_irq_handler.rx_busy = 0;
			   *ow_irq_handler.rx_byte = ow_irq_handler.rx_current_byte;
			   ow_irq_handler.rx_current_byte = 0;
			   ow_irq_handler.rx_index = 0;


			    if(DS2431->write_active) {
					DS2431_write_next_IT(DS2431);
				} else if (DS2431->read_active) {
					DS2431_read_next_IT(DS2431);
				}
		   }
		}

		if(ow_irq_handler.reset_busy) {
			LL_USART_DisableIT_RXNE(ow_irq_handler.usart);
		    LL_USART_SetBaudRate(USARTx, ow_irq_handler.periph_clock, ow_irq_handler.over_sampling, 115200);
		    for(uint32_t i = 0; i < 0xFFFF; i++);

        	ow_irq_handler.presence = (rx_data & 0x10) ? 0 : 1;
        	ow_irq_handler.reset_busy = 0;
            if(DS2431->write_active) {
				DS2431_write_next_IT(DS2431);
			} else if (DS2431->read_active) {
				DS2431_read_next_IT(DS2431);
			}
		}
	}
}

void D2431_write_DMA(DS2431_TypeDef *DS2431, uint8_t *data, uint16_t len) {
	for(int i = 0; i < len; i++) {
		DS2431->tx_buffer[i] = data[i];
	}

    LL_DMA_SetDataLength(DS2431->dma, DS2431->dma_tx_stream, len);
    LL_DMA_SetMemoryAddress(DS2431->dma, DS2431->dma_tx_stream, (uint32_t)DS2431->tx_buffer);
    LL_DMA_SetPeriphAddress(DS2431->dma, DS2431->dma_tx_stream, (uint32_t)&DS2431->usart->DR);

    LL_DMA_EnableIT_TC(DS2431->dma, DS2431->dma_tx_stream);

    LL_DMA_EnableStream(DS2431->dma, DS2431->dma_tx_stream);
}

void DS2431_read_DMA(DS2431_TypeDef *DS2431, uint16_t len, uint8_t *rx_buffer) {
	DS2431->rx_dma_active = 1;

    LL_DMA_SetDataLength(DS2431->dma, DS2431->dma_rx_stream, len);
    LL_DMA_SetMemoryAddress(DS2431->dma, DS2431->dma_rx_stream, (uint32_t)rx_buffer);
    LL_DMA_SetPeriphAddress(DS2431->dma, DS2431->dma_rx_stream, (uint32_t)&DS2431->usart->DR);

    LL_DMA_EnableIT_TC(DS2431->dma, DS2431->dma_rx_stream);

    LL_DMA_EnableStream(DS2431->dma, DS2431->dma_rx_stream);
}

void DS2431_write_data_DMA(DS2431_TypeDef *DS2431) {
	DS2431->write_state_dma = WRITE_STATE_DMA_SEND_CMD_1;
	DS2431->write_active = 1;
	DS2431->read_active = 0;

	DS2431_write_next_DMA(DS2431);
}

void DS2431_read_data_DMA(DS2431_TypeDef *DS2431) {
	DS2431->write_state_dma = READ_STATE_DMA_SEND_CMD;
	DS2431->read_active = 1;
	DS2431->write_active = 0;

	DS2431_read_next_DMA(DS2431);
}

void DS2431_read_next_DMA(DS2431_TypeDef *DS2431) {
    uint8_t buffer[64];

	switch(DS2431->write_state_dma) {
		case READ_STATE_DMA_SEND_CMD:
			if(!onewire_reset(DS2431->usart)) {
				return;
			}

			onewire_write_byte(DS2431->usart, 0xCC);
			build_uart_buffer(0xF0, &buffer[0]);
			build_uart_buffer(DS2431->cmd.address, &buffer[8]);
			build_uart_buffer(0x00, &buffer[16]);

			D2431_write_DMA(DS2431, buffer, 24);
			DS2431->write_state_dma = READ_STATE_DMA_READ_DATA;

			break;
		case READ_STATE_DMA_READ_DATA:
			for(int i = 0; i < 8; i++) {
				build_uart_buffer(0xFF, &buffer[i * 8]);
			}

			DS2431_read_DMA(DS2431, 67, DS2431->rx_buffer);
			for(int i = 0; i < 0xFFFF; i++);
			D2431_write_DMA(DS2431, buffer, 64);

			for(int i = 0; i < 0xFFFF; i++);

			DS2431->write_state_dma = READ_STATE_DMA_DONE;
			break;

		case READ_STATE_DMA_DONE:
			for(int i = 0; i < 8; i++) {
				DS2431->cmd.read_data_buffer[i] = uart_buffer_to_byte(&DS2431->rx_buffer[3 + (i * 8)]);
			}
			DS2431->status = DS2431_READY;
			break;
	}
}

void DS2431_write_next_DMA(DS2431_TypeDef *DS2431) {
    uint8_t buffer[256];

	switch(DS2431->write_state_dma) {
		case WRITE_STATE_DMA_SEND_CMD_1:
			if(!onewire_reset(DS2431->usart)) {
				return;
			}

			onewire_write_byte(DS2431->usart, 0xCC);
			build_uart_buffer(0x0F, &buffer[0]);
			build_uart_buffer(DS2431->cmd.address, &buffer[8]);
			build_uart_buffer(0x00, &buffer[16]);

			D2431_write_DMA(DS2431, buffer, 24);
			DS2431->write_state_dma = WRITE_STATE_DMA_SEND_DATA;


			break;
		case WRITE_STATE_DMA_SEND_DATA:
			for(int i = 0; i < 8; i++) {
				build_uart_buffer(DS2431->cmd.write_data_buffer[i], &buffer[(i) * 8]);
			}

			D2431_write_DMA(DS2431, buffer, 64);
		    for(int i = 0; i < 0xFFFF; i++);

			DS2431->write_state_dma = WRITE_STATE_DMA_READ_CRC;

			break;

		case WRITE_STATE_DMA_READ_CRC:
			build_uart_buffer(0xFF, &buffer[0]);
			build_uart_buffer(0xFF, &buffer[8]);

		    DS2431_read_DMA(DS2431, 19, DS2431->rx_buffer);
		    for(int i = 0; i < 0xFFFF; i++);
			D2431_write_DMA(DS2431, buffer, 16);

		    for(int i = 0; i < 0xFFFF; i++);
			DS2431->write_state_dma = WRITE_STATE_DMA_SEND_CMD_2;
			break;

		case WRITE_STATE_DMA_SEND_CMD_2:
			DS2431->crc1 = uart_buffer_to_byte(&DS2431->rx_buffer[3]);
			DS2431->crc2 = uart_buffer_to_byte(&DS2431->rx_buffer[11]);

			if(!onewire_reset(DS2431->usart)) {
				return;
			}
			build_uart_buffer(0xCC, &buffer[0]);
			D2431_write_DMA(DS2431, buffer, 8);
		    for(int i = 0; i < 0xFFFF; i++);

			DS2431->write_state_dma = WRITE_STATE_DMA_READ_SCRATCHPAD;
			break;
		case WRITE_STATE_DMA_READ_SCRATCHPAD:
		    onewire_write_byte(DS2431->usart, 0xAA);

			for(int i = 0; i < 12; i++) {
				build_uart_buffer(0xFF, &buffer[i * 8]);
			}

			DS2431_read_DMA(DS2431, 90, DS2431->rx_buffer);
		    for(int i = 0; i < 0xFFFF; i++);
			D2431_write_DMA(DS2431, buffer, 88);

		    for(int i = 0; i < 0xFFFF; i++);

			DS2431->write_state_dma = WRITE_STATE_DMA_COPY_SCRATCHPAD;
			break;
		case WRITE_STATE_DMA_COPY_SCRATCHPAD:
			for(int i = 0; i < 11; i++) {
				DS2431->scratchpad[i] = uart_buffer_to_byte(&DS2431->rx_buffer[1 + (i * 8)]);
			}

		    if(!onewire_reset(DS2431->usart)) {
		        return;
		    }

		    onewire_write_byte(DS2431->usart, 0xCC);
			build_uart_buffer(0x55, &buffer[0]);
			build_uart_buffer(DS2431->scratchpad[0], &buffer[8]);
			build_uart_buffer(DS2431->scratchpad[1], &buffer[16]);
			build_uart_buffer(DS2431->scratchpad[2], &buffer[24]);

			D2431_write_DMA(DS2431, buffer, 32);
			DS2431->write_state_dma = WRITE_STATE_DMA_READ_SUCCESS;
			break;
		case WRITE_STATE_DMA_READ_SUCCESS:
			build_uart_buffer(0xFF, &buffer[0]);
			build_uart_buffer(0xFF, &buffer[8]);

		    for(int i = 0; i < 0xFFFF; i++);
		    DS2431_read_DMA(DS2431, 9, DS2431->rx_buffer);
		    for(int i = 0; i < 0xFFFF; i++);
			D2431_write_DMA(DS2431, buffer, 8);
		    for(int i = 0; i < 0xFFFF; i++);

			DS2431->write_state_dma = WRITE_STATE_DMA_DONE;
			break;
		case WRITE_STATE_DMA_DONE:
			DS2431->success = uart_buffer_to_byte(&DS2431->rx_buffer[1]);
			DS2431->status = DS2431_READY;
	}
}

void onewire_uart_dma_handler(DS2431_TypeDef *DS2431) {
   if((LL_DMA_IsActiveFlag_TC5(DS2431->dma) && DS2431->dma_rx_stream == LL_DMA_STREAM_5) ||
	   (LL_DMA_IsActiveFlag_TC0(DS2431->dma) && DS2431->dma_rx_stream == LL_DMA_STREAM_0) ||
	   (LL_DMA_IsActiveFlag_TC1(DS2431->dma) && DS2431->dma_rx_stream == LL_DMA_STREAM_1) ||
	   (LL_DMA_IsActiveFlag_TC2(DS2431->dma) && DS2431->dma_rx_stream == LL_DMA_STREAM_2) ||
	   (LL_DMA_IsActiveFlag_TC3(DS2431->dma) && DS2431->dma_rx_stream == LL_DMA_STREAM_3) ||
	   (LL_DMA_IsActiveFlag_TC4(DS2431->dma) && DS2431->dma_rx_stream == LL_DMA_STREAM_4) ||
	   (LL_DMA_IsActiveFlag_TC6(DS2431->dma) && DS2431->dma_rx_stream == LL_DMA_STREAM_6) ||
	   (LL_DMA_IsActiveFlag_TC7(DS2431->dma) && DS2431->dma_rx_stream == LL_DMA_STREAM_7))
   {

		switch(DS2431->dma_rx_stream) {
			case LL_DMA_STREAM_0: LL_DMA_ClearFlag_TC0(DS2431->dma); break;
			case LL_DMA_STREAM_1: LL_DMA_ClearFlag_TC1(DS2431->dma); break;
			case LL_DMA_STREAM_2: LL_DMA_ClearFlag_TC2(DS2431->dma); break;
			case LL_DMA_STREAM_3: LL_DMA_ClearFlag_TC3(DS2431->dma); break;
			case LL_DMA_STREAM_4: LL_DMA_ClearFlag_TC4(DS2431->dma); break;
			case LL_DMA_STREAM_5: LL_DMA_ClearFlag_TC5(DS2431->dma); break;
			case LL_DMA_STREAM_6: LL_DMA_ClearFlag_TC6(DS2431->dma); break;
			case LL_DMA_STREAM_7: LL_DMA_ClearFlag_TC7(DS2431->dma); break;
		}

		LL_DMA_DisableStream(DS2431->dma, DS2431->dma_rx_stream);
		LL_DMA_DisableIT_TC(DS2431->dma, DS2431->dma_rx_stream);

		if(DS2431->write_active){
			DS2431_write_next_DMA(DS2431);
		} else if (DS2431->read_active) {
			DS2431_read_next_DMA(DS2431);
		}
    }

   if((LL_DMA_IsActiveFlag_TC6(DS2431->dma) && DS2431->dma_tx_stream == LL_DMA_STREAM_6) ||
          (LL_DMA_IsActiveFlag_TC0(DS2431->dma) && DS2431->dma_tx_stream == LL_DMA_STREAM_0) ||
          (LL_DMA_IsActiveFlag_TC1(DS2431->dma) && DS2431->dma_tx_stream == LL_DMA_STREAM_1) ||
          (LL_DMA_IsActiveFlag_TC2(DS2431->dma) && DS2431->dma_tx_stream == LL_DMA_STREAM_2) ||
          (LL_DMA_IsActiveFlag_TC3(DS2431->dma) && DS2431->dma_tx_stream == LL_DMA_STREAM_3) ||
          (LL_DMA_IsActiveFlag_TC4(DS2431->dma) && DS2431->dma_tx_stream == LL_DMA_STREAM_4) ||
          (LL_DMA_IsActiveFlag_TC5(DS2431->dma) && DS2431->dma_tx_stream == LL_DMA_STREAM_5) ||
          (LL_DMA_IsActiveFlag_TC7(DS2431->dma) && DS2431->dma_tx_stream == LL_DMA_STREAM_7))
   {
	   switch(DS2431->dma_tx_stream) {
		   case LL_DMA_STREAM_0: LL_DMA_ClearFlag_TC0(DS2431->dma); break;
		   case LL_DMA_STREAM_1: LL_DMA_ClearFlag_TC1(DS2431->dma); break;
		   case LL_DMA_STREAM_2: LL_DMA_ClearFlag_TC2(DS2431->dma); break;
		   case LL_DMA_STREAM_3: LL_DMA_ClearFlag_TC3(DS2431->dma); break;
		   case LL_DMA_STREAM_4: LL_DMA_ClearFlag_TC4(DS2431->dma); break;
		   case LL_DMA_STREAM_5: LL_DMA_ClearFlag_TC5(DS2431->dma); break;
		   case LL_DMA_STREAM_6: LL_DMA_ClearFlag_TC6(DS2431->dma); break;
		   case LL_DMA_STREAM_7: LL_DMA_ClearFlag_TC7(DS2431->dma); break;
	   }

		LL_DMA_DisableStream(DS2431->dma, DS2431->dma_tx_stream);
		LL_DMA_DisableIT_TC(DS2431->dma, DS2431->dma_tx_stream);
        LL_DMA_DisableStream(DS2431->dma, DS2431->dma_rx_stream);

        if(!DS2431->rx_dma_active) {
            if(DS2431->write_active){
                DS2431_write_next_DMA(DS2431);
            } else if (DS2431->read_active) {
                DS2431_read_next_DMA(DS2431);
            }
        } else {
            DS2431->rx_dma_active = 0;
        }
    }
}



