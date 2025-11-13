#include "speaker_dac.hpp"

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
    const int on_cycles = 100; // ~200ms tone at ~500 Hz
    const uint8_t amp = 204;   // ~80% volume (255 * 0.8)
    while (s_beep_active) {
        for (int i = 0; i < on_cycles && s_beep_active; ++i) {
            dac_output_voltage(ch, amp);
            esp_rom_delay_us(1000);
            dac_output_voltage(ch, 0);
            esp_rom_delay_us(1000);
        }
        vTaskDelay(pdMS_TO_TICKS(800)); // ~0.8s silence
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
