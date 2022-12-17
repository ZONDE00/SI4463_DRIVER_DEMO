/*
 * SI4463.c
 *
 *  Created on: Jul 6, 2022
 *      Author: Kristers
 */

#include "SI4463.h"
#include "radio_config_Si4463.h"
#include "radio_config_Si4463_rtty.h"
#include "radio_config_Si4463_30.h"
#include "radio_config_Si4463_rtty_30.h"
#include <string.h>
/*
 * Generatre RADIO_CONFIGURATION_DATA_ARRAY using Wireless Development Suite at silicon labs
 * Delete first line of RADIO_CONFIGURATION_DATA_ARRAY
 * remove patch bit from second line of RADIO_CONFIGURATION_DATA_ARRAY
 */

uint8_t FSK_26[] = RADIO_CONFIGURATION_DATA_ARRAY;
uint8_t RTTY_26[] = RADIO_CONFIGURATION_DATA_RTTY_ARRAY;
uint8_t FSK_30[] = RADIO_CONFIGURATION_DATA_ARRAY_30;
uint8_t RTTY_30[] = RADIO_CONFIGURATION_DATA_RTTY_ARRAY_30;

// GPIO settings
uint8_t confGPIO[8];

/*
 * @Brief Waits till CTS is high and receives data stored in CMD_BUFF
 * @Param *buf pointer to buffer where received data will be written to
 * @Param size size of data to receive
 * @Param timeout how long will try to receive
 * @Retval status of CTS and Receive
 */
SI4463_StatusTypeDef SI4463_Init(SI4463_Handle *handle) {
    if (handle->spi == 0x00) {
        return SI4463_ERROR;
    }
    if (handle->CSPort == 0x00) {
        return SI4463_ERROR;
    }

    // deselect
    SI4463_Deselect(handle);
    // Shutdown TRX
    HAL_GPIO_WritePin(handle->SDNPort, handle->SDNPin, GPIO_PIN_SET);
    HAL_Delay(10);
    // turn on TRX
    HAL_GPIO_WritePin(handle->SDNPort, handle->SDNPin, GPIO_PIN_RESET);
    HAL_Delay(14);

    uint8_t rxData[8];
    uint8_t idCmd = 0x01;
    SI4463_SPI_Receive(handle, idCmd, rxData, 8, 1000);
    // check for correct device ID
    if( rxData[1] != 0x44 || rxData[2] != 0x63){
        return SI4463_ERROR;
    }

    // config device
    SI4463_Config(handle);

    // check device id
    uint8_t data[12] = { 0 };
    uint8_t cmd = 0;
    SI4463_StatusTypeDef retVal = SI4463_ERROR;

    cmd = SI4463_CMD_PART_INFO;
    retVal = SI4463_SPI_Receive(handle, cmd, data, 8, 1000);
    if (retVal != SI4463_OK) {
        return retVal;
    }
    // check if device ID matches
    if (data[1] != 0x44 || data[2] != 0x63) {
        return SI4463_ERROR;
    }

    return SI4463_OK;
}

/*
 * @Brief configures SI4463 by checking changes in handle
 * @Param *handle pointer to handle
 * @Retval status of configuration
 */
SI4463_StatusTypeDef SI4463_Reconfigure(SI4463_Handle *handle) {
//    SI4463_StatusTypeDef retVal;
//
//    uint8_t cmd = 0x00;
//    uint8_t rxData[8] = { 0 };
//    // send NOP before everything else
//    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);
//    HAL_SPI_Transmit(handle->spi, &cmd, 1, 20);
//    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET);
//
//    while (*config != 0) {
//        uint8_t len = *config;
//        if (len > 16) {
//            return SI4463_ERROR;
//        }
//
//        config++;
//        uint8_t buf[16];
//        memcpy(buf, config, len);
//        config += len;
//
//        retVal = SI4463_SPI_Transmit(buf[0], &buf[1], (len - 1), 1000);
//        if (retVal != SI4463_OK) {
//            return retVal;
//        }
//
//        SI4463_SPI_Transmit(cmd, rxData, 3, 1000);
//
//        cmd = SI4463_CMD_CHIP_STATUS;
//        if (buf[0] != SI4463_CMD_POWER_UP) {
//            SI4463_SPI_Receive(cmd, rxData, 8, 1000);
//            if (rxData[3] == buf[0]) {
//                while (1);  // error in configuration pls fix
//            }
//        } else {
//            // if SI4463_CMD_POWER_UP then wait for chip ready bit
//            while (1) {
//                SI4463_SPI_Receive(cmd, rxData, 8, 1000);
//                if (rxData[0] & 0b100) {
//                    break;
//                }
//            }
//        }
//    }

    return SI4463_OK;
}

/*
 * @Brief configures SI4463 from from WDS generated config header
 * @Param *config pointer to generated config array
 * @Retval status of configuration
 */
volatile GPIO_PinState test = 0;
SI4463_StatusTypeDef SI4463_Config(SI4463_Handle *handle) {
    SI4463_StatusTypeDef retVal;
    uint8_t *config;
    uint8_t cmd = 0x00;
    uint8_t rxData[8] = { 0 };

    // send NOP before everything else
    SI4463_Select(handle);
    HAL_SPI_Transmit(handle->spi, &cmd, 1, 20);
    SI4463_Deselect(handle);

    if (handle->config == SI4463_CONFIG_FSK) {
        if (handle->freq == SI4463_FREQ_26MHZ) {
            config = FSK_26;
        } else if (handle->freq == SI4463_FREQ_30MHZ) {
            config = FSK_30;
        } else {
            return SI4463_ERROR;
        }
    } else if (handle->config == SI4463_CONFIG_RTTY) {
        if (handle->freq == SI4463_FREQ_26MHZ) {
            config = RTTY_26;
        } else if (handle->freq == SI4463_FREQ_30MHZ) {
            config = RTTY_30;
        } else {
            return SI4463_ERROR;
        }
    } else {
        return SI4463_ERROR;
    }

    // go trough configuration file and configure SI4463
    while (*config != 0) {
        uint8_t len = *config;
        if (len > 16) {
            return SI4463_ERROR;
        }
         // 0x02, 0x01, 0x01, 0x01, 0x8C, 0xBA, 0x80
        config++;
        uint8_t buf[16];
        memcpy(buf, config, len);
        config += len;

        // hijack power level settings and replace it with user set one
        if (buf[1] == SI4463_GRP_PA) {
            buf[5] = handle->power;
        }

        // hijack GPIO SETTINGS
        if (buf[0] == SI4463_CMD_GPIO_PIN_CFG) {
            // set as TX in default
            buf[3] = SI4463_GPIO_HIGH;
            buf[4] = SI4463_GPIO_LOW;

            // copy to local buffer so we dont need to read it every time we change GPIO state
            memcpy(confGPIO, buf, 8);

            // set as in TX mode
            handle->isInTX = 1;
        }

        retVal = SI4463_SPI_Transmit(handle, buf[0], &buf[1], (len - 1), 1000);
        if (retVal != SI4463_OK) {
            return retVal;
        }

        SI4463_SPI_Transmit(handle, cmd, rxData, 3, 1000);

        cmd = SI4463_CMD_CHIP_STATUS;
        if (buf[0] != SI4463_CMD_POWER_UP) {
            SI4463_SPI_Receive(handle, cmd, rxData, 8, 1000);
            if (rxData[3] == buf[0]) {
                while (1);  // error in configuration pls fix
            }
        } else {
            // if SI4463_CMD_POWER_UP then wait for chip ready bit
            // timeout in 100 ms
            uint32_t endTime = HAL_GetTick() + 100;
            while (1) {
                SI4463_SPI_Receive(handle, cmd, rxData, 8, 1000);
                if (rxData[0] & 0b100) { // chip ready bit
                    break;
                }

                if (rxData[0] & 0b1000) { // cmd error bit
                    return SI4463_ERROR;
                }

                if (endTime < HAL_GetTick()) {
                    return SI4463_ERROR;
                }
            }
        }
    }

    return SI4463_OK;
}

/*
 * @Brief Sets GPIO state
 * @Param gpio to which gpio set state
 * @Param state what state to set
 * @Retval status of CTS and Receive
 */
SI4463_StatusTypeDef SI4463_GPIO_Set(SI4463_Handle *handle, uint8_t gpio, uint8_t state) {
    if (state == SI4463_GPIO_HIGH || state == SI4463_GPIO_LOW) {
        if (gpio < 4) {
            confGPIO[gpio + 1] = state;
            return SI4463_SPI_Transmit(handle, *confGPIO, &confGPIO[1], 7, 1000);
        }
    }

    return SI4463_ERROR;
}

/*
 * @Brief Waits till CTS is high and receives data stored in CMD_BUFF
 * @Param *buf pointer to buffer where received data will be written to
 * @Param size size of data to receive
 * @Param timeout how long will try to receive
 * @Retval status of CTS and Receive
 */
SI4463_StatusTypeDef SI4463_PollCTSAndReceive(SI4463_Handle *handle, uint8_t *buf, uint8_t size, uint32_t timeout) {

    uint32_t endTime = timeout + HAL_GetTick();
    uint8_t tmp = SI4463_CMD_BUFF;
    uint8_t data = 0;

    // CTS check
    while (1) {
        SI4463_Select(handle);
        HAL_SPI_Transmit(handle->spi, &tmp, 1, 10);
        HAL_SPI_Receive(handle->spi, &data, 1, 10);
        if (data == 0xff) {
            HAL_SPI_Receive(handle->spi, buf, size, 10);
            SI4463_Deselect(handle);
            break;
        }
        SI4463_Deselect(handle);
        if (endTime < HAL_GetTick()) {
            return SI4463_TIMEOUT;
        }
    }

    return SI4463_OK;
}

/*
 * @Brief Tries to receive data over radio
 * @Param *buf pointer to buffer where received data will be written to
 * @Param size size of data to receive
 * @Param timeout how long will try to receive
 * @Retval status of CTS and Receive
 */
SI4463_StatusTypeDef SI4463_Receive_FSK(SI4463_Handle *handle, uint8_t *buf, uint8_t size, uint32_t timeout) {
    // must be configured in FSK mode
    if (handle->config != SI4463_CONFIG_FSK) {
        handle->config = SI4463_CONFIG_FSK;
        SI4463_Config(handle);
    }

    SI4463_StatusTypeDef retVal = SI4463_ERROR;
    uint32_t endTime = timeout + HAL_GetTick();
    uint8_t cmd = SI4463_CMD_START_RX;

    // can receive 8 bytes in single packet
    if (size != 8) {
        return SI4463_ERROR;
    }

    // toggle RF switch to input if set as output
    if (handle->isInTX) {
        retVal = SI4463_GPIO_Set(handle, 2, SI4463_GPIO_LOW);
        if (retVal != SI4463_OK) {
            return retVal;
        }
        retVal = SI4463_GPIO_Set(handle, 3, SI4463_GPIO_HIGH);
        if (retVal != SI4463_OK) {
            return retVal;
        }
        handle->isInTX = 0;
    }

    uint8_t data[8];
    data[0] = 0;
    data[1] = 0;
    data[2] = 0; // len msb
    data[3] = size; // len lsb
    data[4] = 0; // stay in rx if timedout
    data[5] = 8; // stay in rx if received
    data[6] = 8; // stay in rx if invalid
    SI4463_SPI_Transmit(handle, cmd, data, 7, 10);

    // poll for RX received interrupt
    while (1) {
        SI4463_SPI_Receive(handle, SI4463_CMD_INT_STATUS, data, 8, 3);
        if (data[3] & 0b10000 || data[3] & 0b1) {
            break;
        }
        if (endTime < HAL_GetTick()) {
            return SI4463_TIMEOUT;
        }
    }

    // get data from FIFO
    cmd = SI4463_CMD_RX_FIFO;
    SI4463_Select(handle);
    HAL_SPI_Transmit(handle->spi, &cmd, 1, 20);
    HAL_SPI_Receive(handle->spi, buf, size, 20);
    SI4463_Deselect(handle);

    return SI4463_OK;
}

/*
 * @Brief Transmits data using RTTY protocol
 * @Param data pointer to data that will be transmitted
 * @Param size size of data to transmit
 * @Retval status of transmit
 */
SI4463_StatusTypeDef SI4463_Transmit_RTTY(SI4463_Handle *handle, uint8_t *data, uint8_t size) {
    // must be configured in RTTY mode
    if (handle->config != SI4463_CONFIG_RTTY) {
        handle->config = SI4463_CONFIG_RTTY;
        SI4463_Config(handle);
    }

    // enable transmitter
    uint8_t startTx[8];
    startTx[0] = SI4463_CMD_START_TX;
    startTx[1] = 0; // channel
    startTx[2] = 128; // after TX enter RX
    startTx[3] = 0; // size msb
    startTx[4] = 0; // size lsb
    startTx[5] = 0; // delay
    startTx[6] = 0; // repeat
    SI4463_SPI_Transmit(handle, startTx[0], startTx + 1, 6, 1000);

    // enable output by setting high first
    HAL_GPIO_WritePin(handle->GPIO0_Port, handle->GPIO0_Pin, GPIO_PIN_SET);
    HAL_Delay(200);

    // transmit data
    for (uint8_t i = 0; i < size; i++) {

        uint8_t tmp = 0;
        // start bit, low
        HAL_GPIO_WritePin(handle->GPIO0_Port, handle->GPIO0_Pin, GPIO_PIN_RESET);
        HAL_Delay(handle->baudDelay);

        tmp = data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (tmp & (1 << j)) {
                HAL_GPIO_WritePin(handle->GPIO0_Port, handle->GPIO0_Pin, GPIO_PIN_SET);
            } else {
                HAL_GPIO_WritePin(handle->GPIO0_Port, handle->GPIO0_Pin, GPIO_PIN_RESET);
            }
            HAL_Delay(handle->baudDelay);
        }
        // stop bit 1.5, high
        HAL_GPIO_WritePin(handle->GPIO0_Port, handle->GPIO0_Pin, GPIO_PIN_SET);
        HAL_Delay((handle->baudDelay * 3) >> 1);

    }

    // disable transmitter
    startTx[0] = SI4463_CMD_START_RX;
    startTx[1] = 0; // channel
    startTx[2] = 128; // after TX enter RX
    startTx[3] = 0; // size msb
    startTx[4] = 0; // size lsb
    startTx[5] = 0; // delay
    startTx[6] = 0; // repeat
    SI4463_SPI_Transmit(handle, startTx[0], startTx + 1, 6, 1000);

    HAL_Delay(handle->baudDelay * 5);

    return SI4463_OK;
}

/*
 * @Brief Transmits data over radio
 * @Param *buf pointer to buffer to data that will be transmitted
 * @Param size size of data to receive
 * @Param timeout how long will try to transmit
 * @Retval status of CTS and transmit
 */
SI4463_StatusTypeDef SI4463_Transmit_FSK(SI4463_Handle *handle, uint8_t *buf, uint8_t size, uint32_t timeout) {
    uint32_t endTime = HAL_GetTick() + timeout;
    // must be configured in FSK mode
    if (handle->config != SI4463_CONFIG_FSK) {
        handle->config = SI4463_CONFIG_FSK;
        SI4463_Config(handle);
    }

    SI4463_StatusTypeDef retVal = SI4463_ERROR;

    // can only send 8 bytes in single packet, for now
    if (size != 8) {
        return SI4463_ERROR;
    }

    // toggle RF switch to input if set as output
    if (!handle->isInTX) {
        retVal = SI4463_GPIO_Set(handle, 3, SI4463_GPIO_LOW);
        if (retVal != SI4463_OK) {
            return retVal;
        }
        retVal = SI4463_GPIO_Set(handle, 2, SI4463_GPIO_HIGH);
        if (retVal != SI4463_OK) {
            return retVal;
        }
        handle->isInTX = 1;
    }

    // write data to send to TX FIFO
    uint8_t data[8];
    data[0] = SI4463_CMD_WRITE_TX_FIFO;
    retVal = SI4463_SPI_Transmit(handle, data[0], buf, size, timeout);
    if (retVal != SI4463_OK) {
        return retVal;
    }

    // start TX
    data[0] = SI4463_CMD_START_TX;
    data[1] = 0; // channel
    data[2] = 128; // after TX enter RX
    data[3] = 0; // size msb
    data[4] = size; // size lsb
    data[5] = 0; // delay
    data[6] = 0; // repeat
    retVal = SI4463_SPI_Transmit(handle, data[0], data + 1, 6, timeout);
    if (retVal != SI4463_OK) {
        return retVal;
    }

    // poll for packet sent
    while (1) {
        SI4463_SPI_Receive(handle, SI4463_CMD_INT_STATUS, data, 8, 3);
        if (data[3] & 0b100000) {
            break;
        }
        if (endTime < HAL_GetTick()) {
            return SI4463_TIMEOUT;
        }
    }

    return SI4463_OK;
}

/*
 * @Brief Waits till SI4463 is ready to process next command
 * @Param timeout how long will try wait for SI4463 to be ready
 * @Retval status of CTS
 */
SI4463_StatusTypeDef SI4463_PollCTS(SI4463_Handle *handle, uint32_t timeout) {
    uint32_t endTime = timeout + HAL_GetTick();
    uint8_t tmp = SI4463_CMD_BUFF;
    uint8_t data = 0;

    // CTS check
    while (1) {
        SI4463_Select(handle);
        HAL_SPI_Transmit(handle->spi, &tmp, 1, 10);
        HAL_SPI_Receive(handle->spi, &data, 1, 10);
        SI4463_Deselect(handle);
        if (data == 0xff) {
            break;
        }
        if (endTime < HAL_GetTick()) {
            return SI4463_TIMEOUT;
        }
    }

    return SI4463_OK;
}

/*
 * @Brief high level receive function using SPI from Si4463
 * @Param cmd command to send
 * @Param *buf pointer to buffer where received data will be written to
 * @Param size size of data to receive
 * @Param timeout how long will try to receive
 * @Retval status of receive
 */
SI4463_StatusTypeDef SI4463_SPI_Receive(SI4463_Handle *handle, uint8_t cmd, uint8_t *buf, uint8_t size, uint32_t timeout) {
    SI4463_StatusTypeDef retVal = SI4463_ERROR;

    retVal = SI4463_PollCTS(handle, timeout);   // wait for ready
    if (retVal != SI4463_OK) {
        return retVal;
    }

    SI4463_Select(handle);
    HAL_SPI_Transmit(handle->spi, &cmd, 1, 10);
    SI4463_Deselect(handle);
    retVal = SI4463_PollCTSAndReceive(handle, buf, size, timeout);

    return retVal;
}

/*
 * @Brief transmits specified command and data ti SI4463
 * @Param cmd command to send
 * @Param *data pointer to data to send
 * @Param size size of data to send
 * @Param timeout how long will try to send
 * @Retval status of transmit
 */

SI4463_StatusTypeDef SI4463_SPI_Transmit(SI4463_Handle *handle, uint8_t cmd, uint8_t *data, uint8_t size, uint32_t timeout) {
    SI4463_StatusTypeDef retVal = SI4463_ERROR;

    retVal = SI4463_PollCTS(handle, timeout);   // wait for ready
    if (retVal != SI4463_OK) {
        return retVal;
    }

    SI4463_Select(handle);
    HAL_SPI_Transmit(handle->spi, &cmd, 1, 20);
    HAL_SPI_Transmit(handle->spi, data, size, 20);
    SI4463_Deselect(handle);

    return SI4463_OK;
}

void SI4463_Select(SI4463_Handle *handle) {
    HAL_GPIO_WritePin(handle->CSPort, handle->CSPin, GPIO_PIN_RESET);
    // dumb delay, for somewhat correct timing
    uint32_t clockFreq = HAL_RCC_GetSysClockFreq() / 100000;
    for(uint32_t i = 0; i < clockFreq; i++);
}

void SI4463_Deselect(SI4463_Handle *handle) {
    HAL_GPIO_WritePin(handle->CSPort, handle->CSPin, GPIO_PIN_SET);
}
