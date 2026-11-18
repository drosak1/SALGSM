#include <Arduino.h>
#include <EEPROM.h>
#include <avr/wdt.h>
#include "hardware_config.h"
#include "SALGSMv1.h"

namespace {
                                            //min  sek    milis
  constexpr unsigned long SEND_INTERVAL_MS = 15UL * 60UL * 1000UL;
  constexpr unsigned long PULSE_DEBOUNCE_MS = 50UL;
  constexpr uint16_t EEPROM_WRITES_PER_SLOT = 60000U;

  struct __attribute__((packed)) CounterRecord {
    uint16_t writeCounter;
    uint32_t value;
  };

  static_assert(sizeof(CounterRecord) == 6, "Niezgodny format rekordu EEPROM.");

  constexpr int EEPROM_RECORD_STRIDE = sizeof(CounterRecord) + 1;

  //GSM==========================================================================================================================
  constexpr char APN[] = "sensor.net";
  constexpr char API_KEY[] = "9999";
  constexpr char SERVER_URL[] = "dlb.com.pl";
  SALGSMv1 gsm(GSM_SERIAL, DEBUG_SERIAL, APN, API_KEY, SERVER_URL, true, PIN_PC3, PIN_PB2);
  //=============================================================================================================================


  char input[201] = {0};
  size_t inputLength = 0;
  bool serialCommandReady = false;

  volatile uint32_t pulseCounter = 0;
  volatile unsigned long lastPulseMs = 0;
  volatile bool pulseEvent = false;

  CounterRecord counterRecord = {0, 0};
  int eepromAddress = 0;
  unsigned long previousSendMs = 0;

  bool eepromRecordFits(int address);
  void eraseCounterStorage();
  void loadCounter();
  void saveCounter();
  uint32_t counterSnapshot();
  void setCounter(uint32_t value);
  void readConsole();
  void processConsoleCommand();
  void pulseIsr();
  void resetGsmHardware();
} // namespace

// Watchdog uruchomiony przez reset systemu musi byc wylaczony przed setup().
void disableWatchdogEarly(void) __attribute__((naked, section(".init3")));
void disableWatchdogEarly(void) {
  wdt_disable();
}

void setup() {
  wdt_disable();
  const uint8_t resetFlags = RSTCTRL.RSTFR;
  RSTCTRL.RSTFR = resetFlags; // zapis jedynki kasuje ustawione flagi resetu

  pinMode(BoardPins::LED_1, OUTPUT);

  pinMode(BoardPins::WATER_PULSE, INPUT_PULLUP);
  pinMode(BoardPins::GAS_PULSE, INPUT_PULLUP);

  digitalWrite(BoardPins::LED_1, HIGH);

  DEBUG_SERIAL.begin(9600);
  DEBUG_SERIAL.setTimeout(1000);
  GSM_SERIAL.begin(9600);

  DEBUG_SERIAL.println();
  DEBUG_SERIAL.println("START ATmega4809 / MegaCoreX");
  DEBUG_SERIAL.print("Reset flags: 0x");
  DEBUG_SERIAL.println(resetFlags, HEX);

  loadCounter();

  attachInterrupt(BoardPins::WATER_PULSE, pulseIsr, FALLING);

  previousSendMs = millis();


  //GSM==========================================================================================================================
  gsm.switch_on();

  //przykłady
  double temperatura = random(30, 42);
  // randomNumber will hold a random value between minValue (inclusive) and maxValue (exclusive)
  gsm.send("temperatura", temperatura);
  // gsm.send("wilgotnosc", 65.45);
  // gsm.send("status", "OK");
  //=============================================================================================================================
}

void loop() {
  readConsole();

  const unsigned long currentMs = millis();
  if (currentMs - previousSendMs >= SEND_INTERVAL_MS) {
    previousSendMs = currentMs;
    saveCounter();
    DEBUG_SERIAL.print("Licznik -> ");
    DEBUG_SERIAL.println(counterRecord.value);
    gsm.send("licznik", counterRecord.value);
  }

  if (pulseEvent) {
    noInterrupts();
    pulseEvent = false;
    interrupts();
    DEBUG_SERIAL.print('*');
  }

  processConsoleCommand();
}

namespace {
void pulseIsr() {
  const unsigned long now = millis();
  if (now - lastPulseMs >= PULSE_DEBOUNCE_MS) {
    ++pulseCounter;
    lastPulseMs = now;
    pulseEvent = true;
  }
}

uint32_t counterSnapshot() {
  noInterrupts();
  const uint32_t value = pulseCounter;
  interrupts();
  return value;
}

void setCounter(uint32_t value) {
  noInterrupts();
  pulseCounter = value;
  interrupts();
}

bool eepromRecordFits(int address) {
  const int eepromLength = static_cast<int>(EEPROM.length());
  return address >= 0 &&
         address + static_cast<int>(sizeof(CounterRecord)) <= eepromLength;
}

void eraseCounterStorage() {
  const int eepromLength = static_cast<int>(EEPROM.length());
  for (int address = 0; address < eepromLength; ++address) {
    EEPROM.update(address, 0);
  }
}

void loadCounter() {
  CounterRecord firstRecord;
  EEPROM.get(0, firstRecord);

  if (firstRecord.writeCounter == UINT16_MAX && firstRecord.value == UINT32_MAX) {
    eraseCounterStorage();
  }

  bool validRecordFound = false;
  uint32_t greatestValue = 0;

  for (int address = 0; eepromRecordFits(address); address += EEPROM_RECORD_STRIDE) {
    CounterRecord candidate;
    EEPROM.get(address, candidate);

    const bool emptyRecord = candidate.writeCounter == 0 && candidate.value == 0;
    const bool validRecord = candidate.writeCounter > 0 &&
                             candidate.writeCounter <= EEPROM_WRITES_PER_SLOT;
    if (emptyRecord || !validRecord) {
      break;
    }

    validRecordFound = true;
    eepromAddress = address;
    counterRecord = candidate;
    if (candidate.value > greatestValue) {
      greatestValue = candidate.value;
    }

    DEBUG_SERIAL.print("EEPROM adr=");
    DEBUG_SERIAL.print(address);
    DEBUG_SERIAL.print(" zapisow=");
    DEBUG_SERIAL.print(candidate.writeCounter);
    DEBUG_SERIAL.print(" wartosc=");
    DEBUG_SERIAL.println(candidate.value);

    if (candidate.writeCounter < EEPROM_WRITES_PER_SLOT) {
      break;
    }
  }

  if (!validRecordFound) {
    eepromAddress = 0;
    counterRecord = {0, 0};
  } else {
    counterRecord.value = greatestValue;
  }

  setCounter(counterRecord.value);
  DEBUG_SERIAL.print("Licznik z EEPROM: ");
  DEBUG_SERIAL.println(counterRecord.value);
}

void saveCounter() {
  if (counterRecord.writeCounter >= EEPROM_WRITES_PER_SLOT) {
    const int nextAddress = eepromAddress + EEPROM_RECORD_STRIDE;
    if (eepromRecordFits(nextAddress)) {
      eepromAddress = nextAddress;
    } else {
      eraseCounterStorage();
      eepromAddress = 0;
    }
    counterRecord.writeCounter = 0;
  }

  counterRecord.value = counterSnapshot();
  ++counterRecord.writeCounter;
  EEPROM.put(eepromAddress, counterRecord);
}


void readConsole() {
  while (DEBUG_SERIAL.available()) {
    const char c = static_cast<char>(DEBUG_SERIAL.read());

    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      input[inputLength] = '\0';
      serialCommandReady = inputLength > 0;
      inputLength = 0;
      continue;
    }

    if (inputLength < sizeof(input) - 1) {
      input[inputLength++] = c;
      input[inputLength] = '\0';
    } else {
      inputLength = 0;
      input[0] = '\0';
    }
  }
}

void processConsoleCommand() {
  if (!serialCommandReady) {
    return;
  }
  serialCommandReady = false;

  if (strstr(input, "clear") != nullptr) {
    const uint32_t currentValue = counterSnapshot();
    eraseCounterStorage();
    eepromAddress = 0;
    counterRecord = {0, currentValue};
    DEBUG_SERIAL.println("EEPROM wyczyszczony.");
  }

  memset(input, 0, sizeof(input));
}
} // namespace
