#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include <Arduino.h>

#if !defined(__AVR_ATmega4809__)
#error "Ten projekt wymaga mikrokontrolera ATmega4809."
#endif

#if !defined(MEGACOREX) && !defined(MCUDUDE_MEGACOREX)
#error "Ten projekt wymaga platformy MegaCoreX."
#endif

#if !defined(MEGACOREX_DEFAULT_48PIN_PINOUT)
#error "W MegaCoreX wybierz pinout: 48 pin standard."
#endif

// Schemat elektryczny, U206 (ATmega4809-M, TQFP48):
// - CH340 / konsola: USART1, PC0 (TX) i PC1 (RX)
// - SIM800L:          USART3, PB0 (TX) i PB1 (RX)
#define DEBUG_SERIAL Serial1
#define GSM_SERIAL   Serial3

namespace BoardPins {
constexpr uint8_t GSM_RESET  = PIN_PB2; // RSTGSM, aktywny stan niski
constexpr uint8_t GSM_ENABLE = PIN_PC3; // ENABLEGSM, aktywny stan wysoki
constexpr uint8_t WATER_PULSE = PIN_PA3; // IMPWATER
constexpr uint8_t GAS_PULSE   = PIN_PA4; // IMPGAS (rezerwa)
constexpr uint8_t LED_1 = PIN_PF3;
constexpr uint8_t LED_2 = PIN_PF4;
constexpr uint8_t BUTTON = PIN_PF5;
} // namespace BoardPins

static_assert(PIN_HWSERIAL1_TX == PIN_PC0 && PIN_HWSERIAL1_RX == PIN_PC1,
              "Niezgodne piny USART1 - sprawdz pinout MegaCoreX.");
static_assert(PIN_HWSERIAL3_TX == PIN_PB0 && PIN_HWSERIAL3_RX == PIN_PB1,
              "Niezgodne piny USART3 - sprawdz pinout MegaCoreX.");

#endif
