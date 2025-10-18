#include <Arduino.h>
#include "target.h"

Heltec_CT62_Board board;

RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY);
WRAPPER_CLASS radio_driver(radio, board);

ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
SensorManager sensors;

#ifdef DISPLAY_CLASS
// 占位显示屏引脚 (请根据真实硬件修改)，使用 LoRa SPI 时钟与 MOSI 复用：SCK=P_LORA_SCLK, SDA=P_LORA_MOSI
#ifndef PIN_LCD_CS
#define PIN_LCD_CS 20
#endif
#ifndef PIN_LCD_RST
#define PIN_LCD_RST 0
#endif
#ifndef PIN_LCD_DC
#define PIN_LCD_DC 1
#endif
DISPLAY_CLASS display(PIN_LCD_CS, PIN_LCD_RST, PIN_LCD_DC, P_LORA_SCLK, P_LORA_MOSI);

// 占位用户按键引脚
#ifndef PIN_USER_BTN
#define PIN_USER_BTN 21
#endif
  MomentaryButton user_btn(PIN_USER_BTN, 1000, true, true);
#endif

bool radio_init() {
  fallback_clock.begin();
  rtc_clock.begin(Wire);  
  return radio.std_init(&SPI);
}

uint32_t radio_get_rng_seed() {
  return radio.random(0x7FFFFFFF);
}

void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr) {
  radio.setFrequency(freq);
  radio.setSpreadingFactor(sf);
  radio.setBandwidth(bw);
  radio.setCodingRate(cr);
}

void radio_set_tx_power(uint8_t dbm) {
  radio.setOutputPower(dbm);
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);  // create new random identity
}