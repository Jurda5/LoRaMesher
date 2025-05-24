/**
 *  PicoHal.cpp – RP2040/FreeRTOS implementation of RadioLibHal
 */

#include "PicoHal.h"

/* ───────────── static data ───────────── */
void (*PicoHal::userCbs_[NUM_BANK0_GPIOS])(void) = {nullptr};

/* helpers */
static inline unsigned long msSinceBoot() {
  return to_ms_since_boot(get_absolute_time());
}
static inline unsigned long usSinceBoot() {
  return to_us_since_boot(get_absolute_time());
}

/* ───────────── constructor ───────────── */
PicoHal::PicoHal(uint8_t sck,
                 uint8_t miso,
                 uint8_t mosi,
                 spi_inst_t *spi,
                 uint32_t hz)
  : RadioLibHal(INPUT, OUTPUT, LOW, HIGH, RISING, FALLING),   // ← explicit
    spi_(spi),
    spi_hz_(hz),
    sck_(sck),
    miso_(miso),
    mosi_(mosi) {

  spiMutex_ = xSemaphoreCreateMutex();
}

/* ───────────── lifecycle ───────────── */
void PicoHal::init() {
  spi_init(spi_, spi_hz_);
  gpio_set_function(sck_,  GPIO_FUNC_SPI);
  gpio_set_function(mosi_, GPIO_FUNC_SPI);
  gpio_set_function(miso_, GPIO_FUNC_SPI);

  gpio_set_drive_strength(sck_, GPIO_DRIVE_STRENGTH_4MA);

  /* register shared dispatcher once */
  gpio_set_irq_enabled_with_callback(0, 0, false, irqDispatcher);
}

void PicoHal::term() { spi_deinit(spi_); }

/* ───────────── GPIO ───────────── */
void PicoHal::pinMode(uint32_t pin, uint32_t mode) {
  if (pin == RADIOLIB_NC) return;
  gpio_init(pin);

  switch (mode) {
    case RADIOLIB_HAL_PIN_MODE_INPUT:
      gpio_set_dir(pin, false); gpio_disable_pulls(pin);          break;
    case RADIOLIB_HAL_PIN_MODE_OUTPUT:
      gpio_set_dir(pin, true);                                    break;
    case RADIOLIB_HAL_PIN_MODE_OUTPUT_OPEN_DRAIN:
      gpio_set_dir(pin, true); gpio_pull_down(pin);               break;
  }
}

void PicoHal::digitalWrite(uint32_t pin, uint32_t val) {
  if (pin != RADIOLIB_NC) gpio_put(pin, val);
}
uint32_t PicoHal::digitalRead(uint32_t pin) {
  return (pin == RADIOLIB_NC) ? 0 : gpio_get(pin);
}

/* ───────────── IRQ fan-out ───────────── */
void PicoHal::attachInterrupt(uint32_t pin, void (*cb)(void), uint32_t mode) {
  if (pin == RADIOLIB_NC) return;

  uint32_t edge = (mode == RISING)  ? GPIO_IRQ_EDGE_RISE :
                  (mode == FALLING) ? GPIO_IRQ_EDGE_FALL :
                                      GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL;

  userCbs_[pin] = cb;
  gpio_set_irq_enabled(pin, edge, true);
}

void PicoHal::detachInterrupt(uint32_t pin) {
  if (pin == RADIOLIB_NC) return;
  gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
  userCbs_[pin] = nullptr;
}

void PicoHal::irqDispatcher(uint gpio, uint32_t /*events*/) {
  if (gpio < NUM_BANK0_GPIOS && userCbs_[gpio]) {
    userCbs_[gpio]();
    portYIELD_FROM_ISR(pdTRUE);
  }
}

/* ───────────── timing ───────────── */
void PicoHal::delay(unsigned long ms)                 { vTaskDelay(pdMS_TO_TICKS(ms)); }
void PicoHal::delayMicroseconds(unsigned long us)     { busy_wait_us_32(us); }
unsigned long PicoHal::millis()                       { return msSinceBoot(); }
unsigned long PicoHal::micros()                       { return usSinceBoot(); }

long PicoHal::pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) {
  absolute_time_t deadline = delayed_by_us(get_absolute_time(), timeout);

  while (gpio_get(pin) == state)
    if (absolute_time_diff_us(get_absolute_time(), deadline) <= 0) return 0;
  while (gpio_get(pin) != state)
    if (absolute_time_diff_us(get_absolute_time(), deadline) <= 0) return 0;

  absolute_time_t start = get_absolute_time();
  while (gpio_get(pin) == state)
    if (absolute_time_diff_us(get_absolute_time(), deadline) <= 0) return 0;

  return absolute_time_diff_us(start, get_absolute_time());
}

/* ───────────── SPI ───────────── */
void PicoHal::spiBegin() { /* already initialised in init() */ }

void PicoHal::spiBeginTransaction() { xSemaphoreTake(spiMutex_, portMAX_DELAY); }

void PicoHal::spiTransfer(uint8_t *out, size_t len, uint8_t *in) {
  uint8_t dummy;
  if (!out) out = &dummy;
  if (!in)  in  = &dummy;
  spi_write_read_blocking(spi_, out, in, len);
}

void PicoHal::spiEndTransaction() { xSemaphoreGive(spiMutex_); }

void PicoHal::spiEnd() { /* nothing – term() already called spi_deinit() */ }
