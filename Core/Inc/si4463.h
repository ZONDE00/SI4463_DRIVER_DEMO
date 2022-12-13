/*
 * SI4463.h
 *
 *  Created on: Jul 6, 2022
 *      Author: Kristers
 */

#include "main.h"

#ifndef INC_SI4463_H_
#define INC_SI4463_H_

typedef enum {
    SI4463_NOTHING = 0x00U,
    SI4463_TRISTATE = 0x01U,
    SI4463_OUTPUT_LOW = 0x02U,
    SI4463_OUTPUT_HIGH = 0x03U,
    SI4463_INPUT = 0x04U
} SI4463_GPIOTypeDef;

typedef enum {
    SI4463_OK = 0x00U,
    SI4463_ERROR = 0x01U,
    SI4463_BUSY = 0x02U,
    SI4463_TIMEOUT = 0x03U
} SI4463_StatusTypeDef;

typedef enum {
    SI4463_FREQ_26MHZ = 0x00U,
    SI4463_FREQ_30MHZ = 0x01U
} SI4463_FreqTypeDef;

typedef enum {
    SI4463_CONFIG_FSK = 0x00U,
    SI4463_CONFIG_RTTY = 0x01U
} SI4463_ConfigTypeDef;

typedef struct {
    /* USER MUST SET THESE */
    SPI_HandleTypeDef *spi;
    // IO
    GPIO_TypeDef *SDNPort;
    uint16_t SDNPin;
    GPIO_TypeDef *CSPort;
    uint16_t CSPin;
    GPIO_TypeDef *GPIO0_Port;
    uint16_t GPIO0_Pin;
    GPIO_TypeDef *GPIO1_Port;
    uint16_t GPIO1_Pin;
    // general
    uint8_t power;              // transmit power
    SI4463_FreqTypeDef freq;    // TCXO or crystall frequency at XIN
    // RTTY specific
    uint16_t baudDelay;         // baudRate = (1000 / baudDelay), baudRate of RTTY protocol
    // status type
    SI4463_ConfigTypeDef config;

    /* THESES ARE SET AUTOMATICALLY, DO NOT EDIT*/
    uint8_t isInTX;         // current state, DO NOT EDIT
    uint8_t gpio[4];        // SI4463_GPIOTypeDef GPIO state, io of SI4463
} SI4463_Handle;

// transmit power table, should be tested for each board
// 0x20 = 15 dBm
// 0x10 = 15 dBm
// 0x0A = 11.5 dBm
// 0x09 = 10.6 dBm
// 0x08 = 9.5 dBm
// 0x03 = 1 dBm
// 0x02 = -3 dBm
// 0x01 = -10 dBm
// 0x00 = -40 dBm

// commands / registers as per Si4463 revC2A Command/Property API Documentation
#define SI4463_CMD_POWER_UP 		0x02
#define SI4463_CMD_PART_INFO 		0x01
#define SI4463_CMD_SET_PROPERTY 	0x11
#define SI4463_CMD_GPIO_PIN_CFG     0x13
#define SI4463_CMD_START_TX			0x31
#define SI4463_CMD_WRITE_TX_FIFO	0x66
#define SI4463_CMD_TX_HOP			0x37
#define SI4463_CMD_BUFF				0x44
#define SI4463_CMD_INT_STATUS		0x20
#define SI4463_CMD_CHIP_STATUS		0x23
#define SI4463_CMD_PH_STATUS		0x21	// Packet Handler
#define SI4463_CMD_RX_FIFO			0x77
#define SI4463_CMD_START_RX			0x32

// groups as per Si4463 revC2A Command/Property API Documentation
#define SI4463_GRP_PA               0x22

// GPIO STATES as per Si4463 revC2A Command/Property API Documentation
#define SI4463_GPIO_LOW               0x02
#define SI4463_GPIO_HIGH              0x03

// external
SI4463_StatusTypeDef SI4463_Init(SI4463_Handle *handle);
SI4463_StatusTypeDef SI4463_Reconfigure(SI4463_Handle *handle);
SI4463_StatusTypeDef SI4463_Receive_FSK(SI4463_Handle *handle, uint8_t *buf, uint8_t size, uint32_t timeout);
SI4463_StatusTypeDef SI4463_Transmit_FSK(SI4463_Handle *handle, uint8_t *buf, uint8_t size, uint32_t timeout);
SI4463_StatusTypeDef SI4463_Transmit_RTTY(SI4463_Handle *handle, uint8_t *data, uint8_t size);

// internal
SI4463_StatusTypeDef SI4463_Config(SI4463_Handle *handle);
SI4463_StatusTypeDef SI4463_GPIO_Set(SI4463_Handle *handle, uint8_t gpio, uint8_t state);
SI4463_StatusTypeDef SI4463_SPI_Receive(SI4463_Handle *handle, uint8_t cmd, uint8_t *buf, uint8_t size, uint32_t timeout);
SI4463_StatusTypeDef SI4463_SPI_Transmit(SI4463_Handle *handle, uint8_t cmd, uint8_t *data, uint8_t size, uint32_t timeout);
SI4463_StatusTypeDef SI4463_PollCTSAndReceive(SI4463_Handle *handle, uint8_t *buf, uint8_t size, uint32_t timeout);
SI4463_StatusTypeDef SI4463_PollCTS(SI4463_Handle *handle, uint32_t timeout);
void SI4463_Select(SI4463_Handle *handle);
void SI4463_Deselect(SI4463_Handle *handle);

#endif /* INC_SI4463_H_ */
