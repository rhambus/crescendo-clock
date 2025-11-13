#ifndef _INCLUDE_SPEAKER_DAC_HPP_
#define _INCLUDE_SPEAKER_DAC_HPP_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/dac.h"
#include "esp_rom_sys.h"
#include "clock_common.hpp"

class SpeakerDAC {
   public:
    void init();
    void startTestBeep();
    void stopTestBeep();

   private:
    static void beepTask(void* pvParam);
    static dac_channel_t resolveChannel();
};

#endif // _INCLUDE_SPEAKER_DAC_HPP_