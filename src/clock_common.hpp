
#ifndef _INCLUDE_CLOCK_COMMON_HPP_
#define _INCLUDE_CLOCK_COMMON_HPP_

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#define ROT_ENC_A_GPIO      GPIO_NUM_25
#define ROT_ENC_B_GPIO      GPIO_NUM_27
#define ROT_ENC_BUTTON_GPIO GPIO_NUM_32
#define ROT_ENC_BUTTON_INVERTED      1

#define DISPLAY_MOSI_GPIO   GPIO_NUM_13
#define DISPLAY_SCLK_GPIO   GPIO_NUM_14
#define DISPLAY_DC_GPIO     GPIO_NUM_2
#define DISPLAY_RST_GPIO    GPIO_NUM_4
#define DISPLAY_BL_GPIO     GPIO_NUM_21
#define DISPLAY_CS_GPIO     GPIO_NUM_15

#define MP3_PLAYER_UART_PORT_NUM     ((uart_port_t)UART_NUM_2)
#define MP3_PLAYER_TX       GPIO_NUM_17
#define MP3_PLAYER_RX       GPIO_NUM_16

#define LIGHT_ADC_CHANNEL   ADC1_CHANNEL_6  // = GPIO34 on ESP32
#define LIGHT_ADC_ATTEN     ADC_ATTEN_DB_0

// DAC speaker output (ESP32 internal DAC2)
#define SPEAKER_DAC_GPIO    GPIO_NUM_26

typedef struct {
    uint8_t hour;
    uint8_t minute;
} clock_time_t;

typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
} wifi_credentials_t;

#endif // _INCLUDE_CLOCK_COMMON_HPP_