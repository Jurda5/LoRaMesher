#include "BuildOptions.h"

const char* LM_TAG = "LoRaMesher";
const char* LM_VERSION = "0.0.8";

#ifdef ARDUINO

size_t getFreeHeap() {
    return ESP.getFreeHeap();
}

#elif defined(ESP_PLATFORM)
  #include <esp_timer.h>
  #include <esp_heap_caps.h>

  size_t getFreeHeap() {
      return heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  }
  extern "C" unsigned long IRAM_ATTR millis() {
      return static_cast<unsigned long>(esp_timer_get_time() / 1000ULL);
  }

#else // Generic FreeRTOS (RP2040, STM32, ...)
  #include "pico/stdlib.h"
  #include "FreeRTOS.h"
  #include "task.h"

  size_t getFreeHeap() {
      return static_cast<size_t>(xPortGetFreeHeapSize());
  }
  extern "C++" unsigned long millis() {
      return static_cast<unsigned long>(to_ms_since_boot(get_absolute_time()));
  }
#endif

long random(long howsmall, long howbig);
long random(long howbig) {
    if (howbig == 0) {
        return 0;
    }
    if (howbig < 0) {
        return (random(0, -howbig));
    }
    // if randomSeed was called, fall back to software PRNG
    uint32_t val = rand();
    return val % howbig;
}

long random(long howsmall, long howbig) {
    if (howsmall >= howbig) {
        return howsmall;
    }
    long diff = howbig - howsmall;
    return random(diff) + howsmall;
}
