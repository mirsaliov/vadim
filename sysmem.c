#include "main.h"
#include "my_uart.h"

LL_RCC_ClocksTypeDef rcc_clocks;
volatile uint32_t timer_counter = 0;

void build_uart_buffer(uint8_t byte, uint8_t *buffer) {
    for(int i = 0; i < 8; i++) {
        uint8_t bit = (byte >> i) & 0x01;
        buffer[i] = bit ? 0xFF : 0x00;
    }
}

uint8_t uart_buffer_to_byte(uint8_t *buffer) {
	uint8_t byte = 0;

    for(int i = 0; i < 8; i++) {
        uint8_t bit = buffer[i] & 0x01;
        byte |= (bit) << i;
    }

    return byte;
}

uint8_t onewire_reset(USART_TypeDef *USARTx) {

    uint8_t rx_data = 0;
    uint32_t periph_clock = 0;
    uint32_t over_sampling = 0;
    uint8_t presence = 0;
    uint32_t timeout = 50000;


    LL_RCC_GetSystemClocksFreq(&rcc_clocks);

    if (USARTx == USART1 || USARTx == USART6) {
        periph_clock = rcc_clocks.PCLK2_Frequency;
    } else {
        periph_clock = rcc_clocks.PCLK1_Frequency ;
    }

    over_sampling = LL_USART_GetOverSampling(USARTx);

    LL_USART_SetBaudRate(USARTx, periph_clock, over_sampling, 9600);

    for (uint32_t i = 0; i < 100; i++);

    LL_USART_ClearFlag_TC(USARTx);


    LL_USART_TransmitData8(USARTx, 0xF0);


	while(!LL_USART_IsActiveFlag_RXNE(USARTx) && timeout > 0) {
		timeout--;
	};

	if (LL_USART_IsActiveFlag_RXNE(USARTx)) {
		rx_data = LL_USART_ReceiveData8(USARTx);
	} else {
		rx_data = 0xFF;
	}

    presence = (rx_data & 0x10) ? 0 : 1;


    LL_USART_SetBaudRate(USARTx, periph_clock, over_sampling, 115200);

    return presence;
}

void onewire_write_bit(USART_TypeDef *USARTx, uint8_t bit) {
	if(bit == 1) {
		while (!LL_USART_IsActiveFlag_TXE(USARTx));
	    LL_USART_TransmitData8(USARTx, 0xFF);
	} else {
		while (!LL_USART_IsActiveFlag_TXE(USARTx));
	    LL_USART_TransmitData8(USARTx, 0x00);
	}

    while(!LL_USART_IsActiveFlag_TC(USARTx));
}

void onewire_write_byte(USART_TypeDef *USARTx, uint8_t byte) {
	uint8_t bit = 0;
	for(int i = 0; i < 8; i++) {
		bit = byte & 0x01;

		onewire_write_bit(USARTx, bit);

		byte = byte >> 1;
	}

}


uint8_t onewire_read_bit(USART_TypeDef *USARTx) {
    uint8_t data;
    (void)LL_USART_ReceiveData8(USARTx);

    /* Отправка 0xFF для чтения бита */
    LL_USART_TransmitData8(USARTx, 0xFF);
    while(!LL_USART_IsActiveFlag_TC(USARTx));

    /* Ожидание и чтение ответа */
    while(!LL_USART_IsActiveFlag_RXNE(USARTx));
    data = LL_USART_ReceiveData8(USARTx);

    return (data & 0x01);
}

uint8_t onewire_read_byte(USART_TypeDef *USARTx) {
	uint8_t rx_data = 0;

	for(int i = 0; i < 8; i++) {
		rx_data |= onewire_read_bit(USARTx) << i;
	}


	return rx_data;
}

void onewire_uart_deinit(USART_TypeDef *USARTx) {
    GPIO_TypeDef *GPIOx;
    uint32_t tx_pin;
    uint32_t usart_periph_clock;
    uint32_t gpio_periph_clock;

    if (USARTx == USART1) {
        GPIOx = GPIOA;
        tx_pin = LL_GPIO_PIN_9;
        usart_periph_clock = LL_APB2_GRP1_PERIPH_USART1;
        gpio_periph_clock = LL_AHB1_GRP1_PERIPH_GPIOA;
    }
    else if (USARTx == USART2) {
        GPIOx = GPIOA;
        tx_pin = LL_GPIO_PIN_2;
        usart_periph_clock = LL_APB1_GRP1_PERIPH_USART2;
        gpio_periph_clock = LL_AHB1_GRP1_PERIPH_GPIOA;
    }
    else if (USARTx == USART3) {
        GPIOx = GPIOB;
        tx_pin = LL_GPIO_PIN_10;
        usart_periph_clock = LL_APB1_GRP1_PERIPH_USART3;
        gpio_periph_clock = LL_AHB1_GRP1_PERIPH_GPIOB;
    }
    #ifdef USART4
    else if (USARTx == USART4) {
        GPIOx = GPIOA;
        tx_pin = LL_GPIO_PIN_0;
        usart_periph_clock = LL_APB1_GRP1_PERIPH_USART4;
        gpio_periph_clock = LL_AHB1_GRP1_PERIPH_GPIOA;
    }
    #endif
    #ifdef USART5
    else if (USARTx == USART5) {
        GPIOx = GPIOC;
        tx_pin = LL_GPIO_PIN_12;
        usart_periph_clock = LL_APB1_GRP1_PERIPH_USART5;
        gpio_periph_clock = LL_AHB1_GRP1_PERIPH_GPIOC;
    }
    #endif
    #ifdef USART6
    else if (USARTx == USART6) {
        GPIOx = GPIOC;
        tx_pin = LL_GPIO_PIN_6;
        usart_periph_clock = LL_APB2_GRP1_PERIPH_USART6;
        gpio_periph_clock = LL_AHB1_GRP1_PERIPH_GPIOC;
    }
    #endif
    else {
        return;
    }

    /* Отключение UART */
    LL_USART_Disable(USARTx);

    /* Сброс настроек UART в состояние по умолчанию */
    LL_USART_DeInit(USARTx);

    /* Сброс пина TX в состояние по умолчанию (аналоговый вход) */
    LL_GPIO_SetPinMode(GPIOx, tx_pin, LL_GPIO_MODE_ANALOG);
    LL_GPIO_SetPinPull(GPIOx, tx_pin, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinOutputType(GPIOx, tx_pin, LL_GPIO_OUTPUT_PUSHPULL);

    /* Отключение тактирования USART */

    if (USARTx == USART1 || USARTx == USART6) {
        LL_APB2_GRP1_DisableClock(usart_periph_clock);
    } else {
        LL_APB1_GRP1_DisableClock(usart_periph_clock);
    }
}

void onewire_uart_init_IT(USART_TypeDef *usart) {
	ow_irq_handler.usart = usart;
	ow_irq_handler.tx_busy = 0;
	ow_irq_handler.tx_index = 0;

    LL_RCC_GetSystemClocksFreq(&rcc_clocks);

    if (usart == USART1 || usart == USART6) {
    	ow_irq_handler.periph_clock = rcc_clocks.PCLK2_Frequency;
    } else {
    	ow_irq_handler.periph_clock  = rcc_clocks.PCLK1_Frequency ;
    }

    ow_irq_handler.over_sampling  = LL_USART_GetOverSampling(usart);


	NVIC_SetPriority(USART2_IRQn, 6);
    NVIC_EnableIRQ(USART2_IRQn);
}

void onewire_write_byte_IT(uint8_t byte) {
	if(ow_irq_handler.tx_busy) return;

	build_uart_buffer(byte, ow_irq_handler.tx_buffer);

	ow_irq_handler.tx_busy = 1;
	ow_irq_handler.tx_index = 0;

    LL_USART_EnableIT_TC(ow_irq_handler.usart);

    LL_USART_TransmitData8(ow_irq_handler.usart, ow_irq_handler.tx_buffer[0]);
}


void onewire_read_byte_IT(uint8_t *result) {
    if(ow_irq_handler.rx_busy) return;
    if(ow_irq_handler.tx_busy) return;

    ow_irq_handler.rx_byte = result;
    ow_irq_handler.rx_current_byte = 0;
    ow_irq_handler.rx_index = 0;
    ow_irq_handler.rx_busy = 1;

    LL_USART_EnableIT_RXNE(ow_irq_handler.usart);
    (void)LL_USART_ReceiveData8(ow_irq_handler.usart);

    LL_USART_TransmitData8(ow_irq_handler.usart, 0xFF);

}


void onewire_reset_IT(USART_TypeDef *USARTx) {
	(void)LL_USART_ReceiveData8(USARTx);
	if(!ow_irq_handler.rx_busy && !ow_irq_handler.reset_busy) {
 	    ow_irq_handler.reset_busy = 1;
	    ow_irq_handler.presence = 0;


	    LL_USART_SetBaudRate(USARTx, ow_irq_handler.periph_clock, ow_irq_handler.over_sampling, 9600);

	    for (uint32_t i = 0; i < 0xFFFF; i++);
	    LL_USART_ClearFlag_TC(USARTx);
	    LL_USART_EnableIT_RXNE(ow_irq_handler.usart);

 	    LL_USART_TransmitData8(USARTx, 0xF0);
	}
}

void onewire_reset_IT_1(USART_TypeDef *USARTx) {
	(void)LL_USART_ReceiveData8(USARTx);
	if(!ow_irq_handler.rx_busy && !ow_irq_handler.reset_busy) {
 	    ow_irq_handler.reset_busy = 1;
	    ow_irq_handler.presence = 0;


	    LL_USART_SetBaudRate(USARTx, ow_irq_handler.periph_clock, ow_irq_handler.over_sampling, 9600);
	    for (uint32_t i = 0; i < 0xFFFF; i++);
	    LL_USART_ClearFlag_TC(USARTx);
	    LL_USART_EnableIT_RXNE(ow_irq_handler.usart);

 	    LL_USART_TransmitData8(USARTx, 0xFF);
	}
}


void onewire_uart_init(USART_TypeDef *USARTx) {
    LL_USART_InitTypeDef USART_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    uint32_t usart_periph_clock;

    if (USARTx == USART1) {
        usart_periph_clock = LL_APB2_GRP1_PERIPH_USART1;
    }
    else if (USARTx == USART2) {
        usart_periph_clock = LL_APB1_GRP1_PERIPH_USART2;
    }
    else if (USARTx == USART3) {
        usart_periph_clock = LL_APB1_GRP1_PERIPH_USART3;
    }
    #ifdef UART4
    else if (USARTx == UART4) {
        usart_periph_clock = LL_APB1_GRP1_PERIPH_UART4;
    }
    #endif
    #ifdef UART5
    else if (USARTx == UART5) {
        usart_periph_clock = LL_APB1_GRP1_PERIPH_UART5;
    }
    #endif
    #ifdef USART6
    else if (USARTx == USART6) {
        usart_periph_clock = LL_APB2_GRP1_PERIPH_USART6;
    }
    #endif
    else {
        return;
    }

    /* Включение тактирования USART и GPIO */
    if (USARTx == USART1 || USARTx == USART6) {
        LL_APB2_GRP1_EnableClock(usart_periph_clock);
    } else {
        LL_APB1_GRP1_EnableClock(usart_periph_clock);
    }

    /* Настройка UART */
    USART_InitStruct.BaudRate = 115200;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USARTx, &USART_InitStruct);
    LL_USART_Enable(USARTx);
}


void onewire_uart_irq_init(USART_TypeDef *USARTx) {
	ow_irq_handler.usart = USARTx;
	ow_irq_handler.tx_busy = 0;
	ow_irq_handler.tx_index = 0;

    LL_RCC_GetSystemClocksFreq(&rcc_clocks);

    if (USARTx == USART1 || USARTx == USART6) {
    	ow_irq_handler.periph_clock = rcc_clocks.PCLK2_Frequency;
    } else {
    	ow_irq_handler.periph_clock  = rcc_clocks.PCLK1_Frequency ;
    }

    ow_irq_handler.over_sampling  = LL_USART_GetOverSampling(USARTx);

    // Определить IRQn по типу USART
    if (USARTx == USART1) {
        NVIC_SetPriority(USART1_IRQn, 1);
        NVIC_EnableIRQ(USART1_IRQn);
    }
    else if (USARTx == USART2) {
        NVIC_SetPriority(USART2_IRQn, 1);
        NVIC_EnableIRQ(USART2_IRQn);
    }
    else if (USARTx == USART3) {
        NVIC_SetPriority(USART3_IRQn, 1);
        NVIC_EnableIRQ(USART3_IRQn);
    }
    #ifdef UART4
    else if (USARTx == UART4) {
        NVIC_SetPriority(UART4_IRQn, 1);
        NVIC_EnableIRQ(UART4_IRQn);
    }
    #endif
    #ifdef UART5
    else if (USARTx == UART5) {
        NVIC_SetPriority(UART5_IRQn, 1);
        NVIC_EnableIRQ(UART5_IRQn);
    }
    #endif
    #ifdef USART6
    else if (USARTx == USART6) {
        NVIC_SetPriority(USART6_IRQn, 1);
        NVIC_EnableIRQ(USART6_IRQn);
    }
    #endif
}



