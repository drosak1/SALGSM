#include "SALGSMv1.h"

#include <avr/wdt.h>

SALGSMv1::SALGSMv1(Stream &serial, Stream &debugSerial, const char *apn,  const char *apikey, const char *serverurl, bool debug, uint8_t GSMenable_pin_cpu, uint8_t GSMreset_pin_cpu)
: GSM_ENABLE(GSMenable_pin_cpu), GSM_RESET(GSMreset_pin_cpu), DEBUG(debug), my_serial(&serial), debug_serial(&debugSerial) {
  strncpy(my_APN, apn, sizeof(my_APN) - 1);
  my_APN[sizeof(my_APN) - 1] = '\0';
  strncpy(my_APIKEY, apikey, sizeof(my_APIKEY) - 1);
  my_APIKEY[sizeof(my_APIKEY) - 1] = '\0';
  strncpy(my_SERVERURL, serverurl, sizeof(my_SERVERURL) - 1);
  my_SERVERURL[sizeof(my_SERVERURL) - 1] = '\0';

  pinMode(this->GSM_RESET, OUTPUT);
  pinMode(this->GSM_ENABLE, OUTPUT);
  digitalWrite(this->GSM_ENABLE, LOW);  
  digitalWrite(this->GSM_RESET, HIGH);
}

//switch on
bool SALGSMv1::switch_on() {
  digitalWrite(this->GSM_ENABLE, HIGH);  
  delay(1000);
  digitalWrite(this->GSM_RESET, LOW); // SIM800L RST jest aktywny w stanie niskim.
  delay(150);
  digitalWrite(this->GSM_RESET, HIGH);
  delay(15000);

  for (uint8_t attempt = 0; attempt < 10; ++attempt) {
    delay(1500);
    IMSI();
    debug_serial->print("IMSI: ");
    debug_serial->println(my_IMSI);
    if (strlen(my_IMSI) >= 10) {
      break;
    }
  }

  if (strlen(my_IMSI) < 10) {
    debug_serial->println("Brak poprawnego IMSI - reset.");
    reset_cpu_();
  }

  if (!con_to_internet()) {
    debug_serial->println("Brak polaczenia GPRS - reset.");
    reset_cpu_();
  }

  return true;
}

//switch on
bool SALGSMv1::switch_off() {
  digitalWrite(this->GSM_ENABLE, LOW);  
  return true;
}

void SALGSMv1::IMSI() {
  char response[200] = {0};
  sendAT("AT+CIMI", response, sizeof(response), 2000);

  strncpy(my_IMSI, extractID(response), sizeof(my_IMSI) - 1);
  my_IMSI[sizeof(my_IMSI) - 1] = '\0';
}

bool SALGSMv1::con_to_internet() {
  char response[200] = {0};
  uint8_t count = 0;
  STATUS = true;

  do {
    clear(response, sizeof(response));
    sendAT("AT+CGATT=1", response, sizeof(response), 3000);
    debug_serial->print("[con()] AT+CGATT=1 -> ");
    debug_serial->println(response);
    ++count;
  } while (strstr(response, "OK") == nullptr && count < 2);
  if (strstr(response, "OK") == nullptr) {
    STATUS = false;
  }

  count = 0;
  do {
    clear(response, sizeof(response));
    char command[130];
    snprintf(command, sizeof(command), "AT+CSTT=\"%s\"", my_APN);
    sendAT(command, response, sizeof(response), 1500);
    debug_serial->print("[con()] AT+CSTT -> ");
    debug_serial->println(response);
    ++count;
  } while (strstr(response, "OK") == nullptr && count < 3);
  if (strstr(response, "OK") == nullptr) {
    STATUS = false;
  }

  count = 0;
  do {
    clear(response, sizeof(response));
    sendAT("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", response, sizeof(response), 1000);
    debug_serial->print("[con()] CONTYPE -> ");
    debug_serial->println(response);
    ++count;
  } while (strstr(response, "OK") == nullptr && count < 3);
  if (strstr(response, "OK") == nullptr) {
    STATUS = false;
  }

  count = 0;
  do {
    clear(response, sizeof(response));
    char command[130];
    snprintf(command, sizeof(command), "AT+SAPBR=3,1,\"APN\",\"%s\"", my_APN);
    sendAT(command, response, sizeof(response), 1000);
    debug_serial->print("[con()] APN -> ");
    debug_serial->println(response);
    ++count;
  } while (strstr(response, "OK") == nullptr && count < 3);
  if (strstr(response, "OK") == nullptr) {
    STATUS = false;
  }

  count = 0;
  do {
    clear(response, sizeof(response));
    sendAT("AT+SAPBR=1,1", response, sizeof(response), 2000);
    debug_serial->print("[con()] AT+SAPBR=1,1 -> ");
    debug_serial->println(response);
    ++count;
  } while (strstr(response, "OK") == nullptr && count < 3);
  if (strstr(response, "OK") == nullptr) {
    STATUS = false;
  }

  count = 0;
  do {
    clear(response, sizeof(response));
    sendAT("AT+SAPBR=2,1", response, sizeof(response), 2000);
    debug_serial->print("[con()] AT+SAPBR=2,1 -> ");
    debug_serial->println(response);
    ++count;
  } while (strstr(response, "OK") == nullptr && count < 3);
  if (strstr(response, "OK") == nullptr) {
    STATUS = false;
  }

  char *ptr = strstr(response, "+SAPBR:");
  if (ptr != nullptr && sscanf(ptr, "+SAPBR: %*d,%*d,\"%19[^\"]\"", IP) == 1) {
    debug_serial->print("IP: ");
    debug_serial->println(IP);
  } else {
    debug_serial->println("Brak poprawnego adresu IP.");
    STATUS = false;
  }

  return STATUS;
}

bool SALGSMv1::http_get_(const char *cmd) {
  char response[60] = {0};
  bool requestSucceeded = false;
  STATUS = true;

  clear(response, sizeof(response));
  sendAT("AT+HTTPINIT", response, sizeof(response), 2000);
  if (strstr(response, "OK") == nullptr) {
    STATUS = false;
  }
  delay(2000);

  clear(response, sizeof(response));
  sendAT("AT+HTTPPARA=\"CID\",1", response, sizeof(response), 2000);
  if (strstr(response, "OK") == nullptr) {
    STATUS = false;
  }
  delay(2000);

  clear(response, sizeof(response));
  sendAT(cmd, response, sizeof(response), 2000);
  if (strstr(response, "OK") == nullptr) {
    STATUS = false;
  }
  delay(2000);

  clear(response, sizeof(response));
  sendAT("AT+HTTPACTION=0", response, sizeof(response), 2000);
  if (strstr(response, "OK") == nullptr) {
    STATUS = false;
  }
  delay(4000);

  clear(response, sizeof(response));
  sendAT("AT+HTTPREAD=0,100", response, sizeof(response), 5000);
  if (strstr(response, "OK") == nullptr) {
    STATUS = false;
  }
  delay(2000);
  requestSucceeded = strstr(response, "SETOK") != nullptr;

  clear(response, sizeof(response));
  sendAT("AT+HTTPTERM", response, sizeof(response), 2000);
  if (strstr(response, "OK") == nullptr) {
    STATUS = false;
  }

  if (!STATUS) {
    reset_cpu_();
  }

  return requestSucceeded;
}

void SALGSMv1::clear(char *buf, size_t size) {
  memset(buf, 0, size);
}

void SALGSMv1::sendAT(const char *cmd,
                      char *out,
                      size_t outSize,
                      unsigned long timeout) {
  if (out == nullptr || outSize == 0) {
    return;
  }

  size_t index = 0;
  my_serial->println(cmd);
  if (DEBUG) {
    debug_serial->print("[DEBUG] ");
  }

  const unsigned long start = millis();
  while (millis() - start < timeout) {
    while (my_serial->available()) {
      const char c = static_cast<char>(my_serial->read());
      if (DEBUG) {
        debug_serial->write(c);
      }

      if (index < outSize - 1 && c != ' ' && c != '\n' && c != '\r' && c != '\t') {
        out[index++] = c;
      }
    }
  }

  out[index] = '\0';
  if (DEBUG) {
    debug_serial->println("[DEBUG END]");
  }
}

const char *SALGSMv1::extractID(const char *resp) {
  static char imsi[32];
  size_t outputIndex = 0;

  for (size_t index = 0; resp[index] != '\0'; ++index) {
    if (resp[index] >= '0' && resp[index] <= '9' && outputIndex < sizeof(imsi) - 1) {
      imsi[outputIndex++] = resp[index];
    }
  }
  imsi[outputIndex] = '\0';
  return imsi;
}

bool SALGSMv1::isModuleAlive() {
  char response[200] = {0};
  sendAT("AT", response, sizeof(response), 10000);
  return strstr(response, "OK") != nullptr;
}

void SALGSMv1::reset_cpu_() {
  if (DEBUG) {
    debug_serial->println("RESET MCU");
  }
  wdt_enable(WDTO_1S);
  while (true) {
    // Oczekiwanie na reset watchdogiem.
  }
}

void SALGSMv1::setDEBUG(bool state) {
  DEBUG = state;
}
