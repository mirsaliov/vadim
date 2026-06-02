#ifndef INC_DS2431_H_
#define INC_DS2431_H_

typedef enum {
    WRITE_STATE_RESET_1,
    WRITE_STATE_SEND_CMD_1,
    WRITE_STATE_SEND_DATA,
    WRITE_STATE_READ_CRC,
    WRITE_STATE_RESET_2,
    WRITE_STATE_READ_SCRATCHPAD,
    WRITE_STATE_RESET_3,
    WRITE_STATE_COPY_SCRATCHPAD,
    WRITE_STATE_READ_SUCCESS,
    WRITE_STATE_DONE,

    READ_STATE_RESET,
    READ_STATE_SEND_CMD,
    READ_STATE_READ_DATA,
    READ_STATE_DONE
} WriteState_irq_t;

typedef enum {
	WRITE_STATE_DMA_SEND_CMD_1,
	WRITE_STATE_DMA_SEND_DATA,
    WRITE_STATE_DMA_READ_CRC,
	WRITE_STATE_DMA_SEND_CMD_2,
    WRITE_STATE_DMA_READ_SCRATCHPAD,
    WRITE_STATE_DMA_COPY_SCRATCHPAD,
    WRITE_STATE_DMA_READ_SUCCESS,
    WRITE_STATE_DMA_DONE,

    READ_STATE_DMA_SEND_CMD,
    READ_STATE_DMA_READ_DATA,
    READ_STATE_DMA_DONE

} WriteState_dma_t;

typedef enum {
	DS2431_INIT,
	DS2431_WAIT,
	DS2431_WORK,
	DS2431_READY,
} DS2431_State;

typedef enum {
	polling,
	irq,
	dma,
} DS2431_Mode;


typedef enum {
	write,
	read,
} DS2431_cmd_t;

typedef struct {
	uint8_t address;
	DS2431_cmd_t cmd_t;
	uint8_t *read_data_buffer;
	uint8_t *write_data_buffer;
	uint8_t status;
} DS2431_cmd;

typedef struct {
	USART_TypeDef *usart;
	DS2431_Mode mode;

	DS2431_cmd cmds[4];
	volatile uint8_t head;
	volatile uint8_t tail;
	volatile uint8_t count;
	volatile uint8_t busy;

	DS2431_cmd cmd;

	volatile DS2431_State status;
	volatile uint8_t is_init;

	volatile WriteState_dma_t write_state_dma;
	volatile WriteState_irq_t write_state;

	volatile uint8_t write_index;
	volatile uint8_t write_active;
	volatile uint8_t read_active;

	volatile uint8_t scratchpad[11];
	volatile uint8_t crc1, crc2;
	volatile uint8_t success;

    DMA_TypeDef *dma;
    uint32_t dma_tx_stream;
    uint32_t dma_rx_stream;

    uint8_t rx_dma_active;

    uint8_t tx_buffer[256];
    uint8_t rx_buffer[256];

} DS2431_TypeDef;


void timer_start(TIM_TypeDef *TIMx);
void timer_stop(TIM_TypeDef *TIMx);

void DS2431_write_data_polling(DS2431_TypeDef *DS2431);
void DS2431_read_data_polling(DS2431_TypeDef *DS2431);

void DS2431_write_IT(DS2431_TypeDef *DS2431);
void DS2431_write_next_IT(DS2431_TypeDef *DS2431);
void DS2431_read_IT(DS2431_TypeDef *DS2431);
void DS2431_read_next_IT(DS2431_TypeDef *DS2431);

void D2431_write_DMA(DS2431_TypeDef *DS2431, uint8_t *data, uint16_t len);
void DS2431_write_data_DMA(DS2431_TypeDef *DS2431);
void DS2431_write_next_DMA(DS2431_TypeDef *DS2431);
void DS2431_read_DMA(DS2431_TypeDef *DS2431, uint16_t len, uint8_t *rx_buffer);
void DS2431_read_data_DMA(DS2431_TypeDef *DS2431);
void DS2431_read_next_DMA(DS2431_TypeDef *DS2431);

void DS2431_set_write_cmd(DS2431_TypeDef *DS2431, uint8_t address, uint8_t *write_data_buffer);
void DS2431_set_read_cmd(DS2431_TypeDef *DS2431, uint8_t address, uint8_t *read_data_buffer);
void DS2431_addcommand(DS2431_TypeDef *DS2431, DS2431_cmd *item);

void DS2431_init(DS2431_TypeDef *DS2431, USART_TypeDef *USARTx, DS2431_Mode mode);
void DS2431_onewire_dma_init(DS2431_TypeDef *DS2431);

void DS2431_cmd_handler(DS2431_TypeDef *DS2431);
void DS2431_handler(DS2431_TypeDef *DS2431);

void onewire_uart_irq_handler(DS2431_TypeDef *DS2431);
void onewire_uart_dma_handler(DS2431_TypeDef *DS2431);

#endif /* INC_DS2431_H_ */
