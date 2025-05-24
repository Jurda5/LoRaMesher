/**
 *  PicoHal.h – RP2040/FreeRTOS implementation of RadioLibHal
 *  Place in  LoRaMesher/src/hal/rp2040/
 */

#ifndef PICO_HAL_H
#define PICO_HAL_H

#include "RadioLib.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

/* ───── Provide Arduino-style constants if not already present ───── */
#ifndef INPUT
  #define INPUT    0
  #define OUTPUT   1
  #define LOW      0
  #define HIGH     1
  #define RISING   1
  #define FALLING  2
#endif

/* ───── RadioLib HAL compatibility fall-backs ───────── */
#ifndef RADIOLIB_HAL_PIN_MODE_INPUT
  #define RADIOLIB_HAL_PIN_MODE_INPUT           0
  #define RADIOLIB_HAL_PIN_MODE_OUTPUT          1
  #define RADIOLIB_HAL_PIN_MODE_OUTPUT_OPEN_DRAIN 2
#endif

#ifndef RADIOLIB_NC
  #define RADIOLIB_NC  0xFF            /* “not connected” fallback */
#endif

class PicoHal : public RadioLibHal {
 public:
  /**
   *  @param sck   GPIO for SPI-SCK
   *  @param miso  GPIO for SPI-MISO
   *  @param mosi  GPIO for SPI-MOSI
   *  @param spi   spi0 (default) or spi1
   *  @param hz    SPI clock (Hz) – LoRa chips like ≤ 8 MHz
   */
  explicit PicoHal(uint8_t sck,
                   uint8_t miso,
                   uint8_t mosi,
                   spi_inst_t *spi = spi0,
                   uint32_t hz = 8'000'000);

  /* RadioLibHal interface */
  void     init()                                    override;
  void     term()                                    override;

  void     pinMode(uint32_t pin, uint32_t mode)      override;
  void     digitalWrite(uint32_t pin, uint32_t val)  override;
  uint32_t digitalRead(uint32_t pin)                 override;

  void     attachInterrupt(uint32_t pin,
                           void (*cb)(void),
                           uint32_t mode)            override;
  void     detachInterrupt(uint32_t pin)             override;

  void     delay(unsigned long ms)                   override;
  void     delayMicroseconds(unsigned long us)       override;
  unsigned long millis()                             override;
  unsigned long micros()                             override;
  long     pulseIn(uint32_t pin,
                   uint32_t state,
                   unsigned long timeout)            override;

  void     spiBegin()                                override;
  void     spiBeginTransaction()                     override;
  void     spiTransfer(uint8_t *out,
                       size_t len,
                       uint8_t *in)                  override;
  void     spiEndTransaction()                       override;
  void     spiEnd()                                  override;

 private:
  static void irqDispatcher(uint gpio, uint32_t events);

  spi_inst_t *spi_;
  uint32_t    spi_hz_;
  uint8_t     sck_, miso_, mosi_;
  SemaphoreHandle_t spiMutex_;

  static void (*userCbs_[NUM_BANK0_GPIOS])(void);
};

#endif /* PICO_HAL_H */
