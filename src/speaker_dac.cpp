#include "speaker_dac.hpp"
#include "esp_timer.h"

static TaskHandle_t s_beep_task = nullptr;
static volatile bool s_beep_active = false;

dac_channel_t SpeakerDAC::resolveChannel() {
#if SPEAKER_DAC_GPIO == GPIO_NUM_26
    return DAC_CHANNEL_2; // GPIO26
#elif SPEAKER_DAC_GPIO == GPIO_NUM_25
    return DAC_CHANNEL_1; // GPIO25
#else
#error "SPEAKER_DAC_GPIO must be GPIO25 or GPIO26 on ESP32"
#endif
}

void SpeakerDAC::init() {
    auto ch = resolveChannel();
    dac_output_enable(ch);
    dac_output_voltage(ch, 0);
}

void SpeakerDAC::beepTask(void* pvParam) {
    dac_channel_t ch = resolveChannel();
    const uint8_t amp_min = 16;   // start quietly
    const uint8_t amp_max = 204;  // target ~80% DAC
    const uint32_t ramp_ms = 60000; // 60 seconds ramp
    const uint32_t off_ms = 500; // 0.5s pause between tri-tones

    uint32_t start_ms = (uint32_t)(esp_timer_get_time() / 1000);
    while (s_beep_active) {
        // Tri-tone sequence: 1600Hz 500ms, 1200Hz 500ms, 1600Hz 500ms, then 500ms pause
        const struct { uint16_t freq; uint16_t dur_ms; } seq[] = {
            {1600, 500}, {1200, 500}, {1600, 500}
        };

        for (int s = 0; s < 3 && s_beep_active; ++s) {
            uint32_t seg_start_us = (uint32_t)esp_timer_get_time();
            uint32_t seg_dur_us = (uint32_t)seq[s].dur_ms * 1000U;
            // half-period in microseconds for chosen frequency
            uint32_t half_us = (uint32_t)(500000U / seq[s].freq);
            if (half_us == 0) half_us = 1;

            while (s_beep_active) {
                uint32_t now_us = (uint32_t)esp_timer_get_time();
                if ((now_us - seg_start_us) >= seg_dur_us) break;

                // Update amplitude based on elapsed time since start (linear ramp)
                uint32_t now_ms = now_us / 1000U;
                uint32_t elapsed = now_ms - start_ms;
                uint8_t amp;
                if (elapsed >= ramp_ms) {
                    amp = amp_max;
                } else {
                    uint32_t delta = (uint32_t)(amp_max - amp_min);
                    amp = (uint8_t)(amp_min + (delta * elapsed) / ramp_ms);
                }

                // toggle DAC for one full cycle at current frequency
                dac_output_voltage(ch, amp);
                esp_rom_delay_us(half_us);
                dac_output_voltage(ch, 0);
                esp_rom_delay_us(half_us);
            }
        }
        // pause between tri-tones
        vTaskDelay(pdMS_TO_TICKS(off_ms));
    }
    dac_output_voltage(ch, 0);
    vTaskDelete(nullptr);
}

void SpeakerDAC::startTestBeep() {
    if (s_beep_task) return;
    s_beep_active = true;
    xTaskCreate(beepTask, "dac_beep", 2048, nullptr, 5, &s_beep_task);
}

void SpeakerDAC::stopTestBeep() {
    if (!s_beep_task) return;
    s_beep_active = false;
    // task will delete itself
    s_beep_task = nullptr;
}
