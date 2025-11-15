
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "driver/ledc.h"
#include <display.hpp>
#include <Antonio_SemiBold75pt7b.h>
#include <Antonio_Regular26pt7b.h>
#include <Antonio_Light16pt7b.h>

#if __has_include(<secrets.hpp>)
#include <secrets.hpp>
#endif

void Display::monitorBrightnessTask(void *pvParameter) {
    Display *pThis = (Display *)pvParameter;
    bool event;
    while (1) {
        xQueueReceive(pThis->queue, &event, 1000 / portTICK_PERIOD_MS);
        pThis->controlBrightness();
    }
}

void Display::init(void) {
    lcd.init();
    lcd.setRotation(0);
    lcd.setColorDepth(16);
    lcd.setBrightness(255);

    // ADC1 config for light sensor (legacy ADC1 API)
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
    ESP_ERROR_CHECK(adc1_config_channel_atten((adc1_channel_t)LIGHT_ADC_CHANNEL, LIGHT_ADC_ATTEN));

    xTaskCreate(this->monitorBrightnessTask, "monitor_brightness_task", 2048, this, 1, NULL);
    queue = xQueueCreate(1, sizeof(bool));
}

void Display::updateContent(display_element_t element, void *value, display_action_t action) {
    switch (element) {
        case D_E_TIME: {
            char time_buf[8];
            uint8_t disp_hour = static_cast<clock_time_t *>(value)->hour;
            uint8_t disp_min = static_cast<clock_time_t *>(value)->minute;
#ifdef TIME_FORMAT_12H
            uint8_t hour12 = disp_hour % 12;
            if (hour12 == 0) hour12 = 12;
            // No leading zero for 1–9 in 12h mode
            sprintf(time_buf, "%d:%02d", hour12, disp_min);
#else
            sprintf(time_buf, "%02d:%02d", disp_hour, disp_min);
#endif
            lcd.setTextDatum(top_center);

            switch (action) {
                case D_A_ON:
                    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                    break;
                default:
                    break;
            }
            {
                const int time_center_x = (lcd.width() / 2) + 5;
                const int time_top_y = 10;
                lcd.drawString(time_buf, time_center_x, time_top_y, &Antonio_SemiBold75pt7b);
            }
#ifdef TIME_FORMAT_12H
            {
                const bool is_pm = (disp_hour >= 12);
                char suffix_buf[2] = { is_pm ? 'p' : 'a', '\0' };
                // Compute right edge of time to place suffix snugly
                const int time_center_x = (lcd.width() / 2) + 5;
                const int time_top_y = 10;
                int time_w = lcd.textWidth(time_buf, &Antonio_SemiBold75pt7b);
                int sx = time_center_x + (time_w / 2) + 8; // padding
                int sy = time_top_y + 6;                  // slight vertical offset

                lcd.setTextDatum(top_left);
                lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                lcd.setFont(&fonts::Font2);
                lcd.setTextSize(3, 3);
                lcd.drawString(suffix_buf, sx, sy);
                lcd.setTextSize(1, 1);
            }
#endif
            break;
        }

        case D_E_ALARM_TIME: {
            char alarm_buf[8];
            {
                uint8_t ah = static_cast<clock_time_t *>(value)->hour;
                uint8_t am = static_cast<clock_time_t *>(value)->minute;
#ifdef TIME_FORMAT_12H
                uint8_t hour12 = ah % 12; if (hour12 == 0) hour12 = 12;
                sprintf(alarm_buf, "%02d:%02d", hour12, am);
#else
                sprintf(alarm_buf, "%02d:%02d", ah, am);
#endif
            }
            char alarm_symbol_buf[2];
            sprintf(alarm_symbol_buf, DISPLAY_SYMBOL_ALARM_ON);
            lcd.setTextColor(TFT_WHITE, TFT_BLACK);  // Normal case
			lcd.setTextDatum(middle_center);
            switch (action) {
                case D_A_OFF:
                    lcd.setTextColor(TFT_DARKGRAY, TFT_BLACK);
                    sprintf(alarm_symbol_buf, DISPLAY_SYMBOL_ALARM_OFF);
                    sprintf(alarm_buf, "     ");
                    break;
                case D_A_HIDE_HOURS:
                    alarm_buf[0] = alarm_buf[1] = ' ';
                    break;
                case D_A_HIDE_MINUTES:
                    alarm_buf[3] = alarm_buf[4] = ' ';
                    break;
                default:
                    break;
            }
            lcd.drawString(alarm_symbol_buf, 35, 205, &Antonio_Regular26pt7b);
            lcd.drawString(alarm_buf, 100, 200, &Antonio_Regular26pt7b);
#ifdef TIME_FORMAT_12H
            if (action != D_A_OFF) {
                // Append small suffix to alarm time as well, aligned to the right edge
                lcd.setTextColor((action == D_A_OFF) ? TFT_DARKGRAY : TFT_WHITE, TFT_BLACK);
                uint8_t ah = static_cast<clock_time_t *>(value)->hour;
                const bool is_pm = (ah >= 12);
                char suffix_buf[2] = { is_pm ? 'p' : 'a', '\0' };

                int alarm_w = lcd.textWidth(alarm_buf, &Antonio_Regular26pt7b);
                int ax_center = 100; // center x used when drawing alarm_buf
                int sx = ax_center + (alarm_w / 2) + 6; // small padding
                int sy = 200; // vertically centered with alarm text

                lcd.setTextDatum(middle_left);
                lcd.setFont(&fonts::Font2);
                lcd.setTextSize(3, 3);
                lcd.drawString(suffix_buf, sx, sy);
                lcd.setTextSize(1, 1);
            }
#endif
            break;

        }

        case D_E_BED_TIME: {
            char bed_time_buf[8];
            sprintf(bed_time_buf, "%01d:%02d", static_cast<clock_time_t *>(value)->hour, static_cast<clock_time_t *>(value)->minute);
            char bed_time_symbol_buf[3];
            sprintf(bed_time_symbol_buf, DISPLAY_SYMBOL_BED);
            lcd.setTextColor(TFT_LIGHTGRAY, TFT_BLACK);  // Normal case
			lcd.setTextDatum(middle_center);
            // We show the remaining bed time only when less than 9 hours
            if (static_cast<clock_time_t *>(value)->hour >= 9)
                action = D_A_OFF;

            switch (action) {
                case D_A_OFF:
                    sprintf(bed_time_buf, "    ");
                    sprintf(bed_time_symbol_buf, "  ");
                    break;
                default:
                    break;
            }
            lcd.drawString(bed_time_symbol_buf, 180, 205, &Antonio_Regular26pt7b);
            lcd.drawString(bed_time_buf, 235, 200, &Antonio_Regular26pt7b);
            break;

        }

        case D_E_SNOOZE_TIME: {
            char snooze_buf[8];
            lcd.setTextDatum(middle_center);
            lcd.setTextColor(TFT_ORANGE, TFT_BLACK);
            switch (action) {
                case D_A_OFF:
                    sprintf(snooze_buf, "     ");
                    lcd.drawString("  ", 175, 205, &Antonio_Regular26pt7b);
                    break;
                case D_A_ON:
                    uint8_t minutes;
                    uint8_t seconds;
                    uint16_t remaining_seconds;
                    remaining_seconds = *(static_cast<uint16_t *>(value));
                    minutes = remaining_seconds / 60;
                    seconds = remaining_seconds % 60;
                    sprintf(snooze_buf, "%01d:%02d", minutes, seconds);
                    lcd.drawString(DISPLAY_SYMBOL_SNOOZE, 175, 205, &Antonio_Regular26pt7b);
                    break;
                default:
                    break;
            }
            lcd.drawString(snooze_buf, 230, 200, &Antonio_Regular26pt7b);
            break;

        }

        default:
            break;
    }
}

void Display::updateContent(display_element_t element, display_action_t action) {
    switch (element) {
        case D_E_ALARM_ACTIVE:
            char alarm_active_symbol_buf[2];
            lcd.setTextColor(TFT_WHITE, TFT_BLACK);  // Normal case
            lcd.setTextDatum(middle_center);
            switch (action) {
                case D_A_OFF:
                    sprintf(alarm_active_symbol_buf, DISPLAY_SYMBOL_ALARM_L);
                    break;
                case D_A_ON:
                    sprintf(alarm_active_symbol_buf, DISPLAY_SYMBOL_ALARM_R);
                    break;
                default:
                    break;
            }
            lcd.drawString(alarm_active_symbol_buf, 35, 205, &Antonio_Regular26pt7b);
            break;

        case D_E_SNOOZE_CANCEL:
            switch (action) {
                case D_A_OFF:
                    // Clear all the bars
                    lcd.setColor(TFT_BLACK);
                    lcd.fillRect(160, 160, 75, 5);
                    break;
                case D_A_ONE_BAR:
                    // Draw only the first bar
                    lcd.setColor(TFT_ORANGE);
                    lcd.fillRect(160, 160, 35, 5);
                    break;
                case D_A_TWO_BARS:
                    // Draw only the second bar
                    lcd.setColor(TFT_ORANGE);
                    lcd.fillRect(200, 160, 35, 5);
                    break;
                default:
                    // There is no "case 3" where all 3 bars are shown
                    break;
            }
            break;

        case D_E_WIFI_STATUS:
            lcd.setTextDatum(middle_center);
            switch (action) {
                case D_A_OFF:
                    lcd.setTextColor(TFT_RED, TFT_BLACK);
                    #ifdef MQTT_ACTIVE
                    lcd.drawString(DISPLAY_SYMBOL_WIFI_OFF, 295, 170, &Antonio_Regular26pt7b);
                    #else
                    lcd.drawString(DISPLAY_SYMBOL_WIFI_OFF, 295, 205, &Antonio_Regular26pt7b);
                    #endif
                    break;
                case D_A_ON:
                    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                    #ifdef MQTT_ACTIVE
                    lcd.drawString(DISPLAY_SYMBOL_WIFI_ON, 295, 170, &Antonio_Regular26pt7b);
                    #else
                    lcd.drawString(DISPLAY_SYMBOL_WIFI_ON, 295, 205, &Antonio_Regular26pt7b);
                    #endif
                    break;
                default:
                    break;
            }
            break;

        #ifdef MQTT_ACTIVE
        case D_E_MQTT_STATUS:
            lcd.setTextDatum(middle_center);
            switch (action) {
                case D_A_OFF:
                    lcd.setTextColor(TFT_RED, TFT_BLACK);
                    lcd.drawString(DISPLAY_SYMBOL_MQTT_OFF, 295, 205, &Antonio_Regular26pt7b);
                    break;
                case D_A_ON:
                    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                    lcd.drawString(DISPLAY_SYMBOL_MQTT_ON, 295, 205, &Antonio_Regular26pt7b);
                    break;
                default:
                    break;
            }
            break;
        #endif

        case D_E_WIFI_SETTING:
            lcd.setTextDatum(middle_center);
            lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
            switch (action) {
                case D_A_ON:
                    lcd.drawString(DISPLAY_SYMBOL_WIFI_COG, 295, 170, &Antonio_Regular26pt7b);
                    lcd.drawString("PRESS", 220, 175, &Antonio_Light16pt7b);
                    lcd.drawString("WPS", 220, 210, &Antonio_Light16pt7b);
                    break;
                default:
                    lcd.drawString("  ", 295, 170, &Antonio_Regular26pt7b);
                    lcd.setColor(TFT_BLACK);
                    lcd.fillRect(180, 150, 90, 80);
                    break;
            }
            break;

        case D_E_AUDIO:
            lcd.setTextDatum(middle_center);
            lcd.setTextColor(TFT_RED, TFT_BLACK);
            switch (action) {
                case D_A_OFF:
                    lcd.drawString(DISPLAY_SYMBOL_AUDIO_OFF, 35, 170, &Antonio_Regular26pt7b);
                    break;
                default:
                    lcd.drawString("  ", 35, 170, &Antonio_Regular26pt7b);
                    break;
            }
            break;

        default:
            break;
    }
}

void Display::controlBrightness(void) {
    int adc_raw;
    uint16_t ambient_light = 0;

    adc_raw = adc1_get_raw((adc1_channel_t)LIGHT_ADC_CHANNEL);
    ambient_light = (uint16_t)adc_raw;

    // If ambient light cannot be read (e.g., sensor not connected), keep current level

    if (max_brightness_requested) {
        setBrightness(DISPLAY_BRIGHTNESS_LEVELS_NR);
    } else {
        // Adjust the brightness level if necessary
        if ((display_brightness_level > 0) &&
            (ambient_light < display_light_thd_down[display_brightness_level - 1])) {
            display_brightness_level--;
        } else if ((display_brightness_level < DISPLAY_BRIGHTNESS_LEVELS_NR) &&
                   (ambient_light > display_light_thd_up[display_brightness_level])) {
            display_brightness_level++;
        }
        if (increased_brightness_requested) {
            setBrightness(display_brightness_level + 1);
        } else {
            setBrightness(display_brightness_level);
        }
    }
}

void Display::setBrightness(uint8_t brightness_level) {
    // In case we have an increased brightness level, we saturate it to the last level
    if (brightness_level > DISPLAY_BRIGHTNESS_LEVELS_NR)
        brightness_level = DISPLAY_BRIGHTNESS_LEVELS_NR;

    lcd.setBrightness(display_light_brightness[brightness_level]);
}

void Display::setMaxBrightness(bool request_max_brightness) {
    max_brightness_requested = request_max_brightness;
    increased_brightness_requested = false;
    // This is to trigger an immediate change of brightness in monitorBrightnessTask
    bool queue_event = true;
    xQueueSend(queue, &queue_event, portMAX_DELAY);
}

void Display::setIncreasedBrightness(bool request_inc_brightness) {
     // This is to trigger an immediate change of brightness in monitorBrightnessTask
    if (increased_brightness_requested != request_inc_brightness) {
        bool queue_event = true;
        xQueueSend(queue, &queue_event, portMAX_DELAY);
    }
    increased_brightness_requested = request_inc_brightness;
    max_brightness_requested = false;
}

bool Display::isDisplayOn(void) {
    return (display_brightness_level > 0 || increased_brightness_requested);
}
