#include "SALGSMv1.h"

#include <stdlib.h>

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
    DEBUG_PRINT("IMSI: ");
    DEBUG_PRINTLN(my_IMSI);
    if (strlen(my_IMSI) >= 10) {
      break;
    }
  }

  if (strlen(my_IMSI) < 10) {
    DEBUG_PRINTLN("[ERROR] Brak poprawnego IMSI - reset.");
    reset_cpu_();
  }

  if (!con_to_internet()) {
    DEBUG_PRINTLN("[ERROR] Brak polaczenia GPRS - reset.");
    reset_cpu_();
  }

  readNetworkData();

  return true;
}

//switch on
bool SALGSMv1::switch_off() {
  digitalWrite(this->GSM_ENABLE, LOW);  
  return true;
}

//https://dlb.com.pl/api/tlm/v2/set.php?ID=1234&value=34&description=temperatura&unit=stC
bool SALGSMv1::send(const char* key, const char* value) {
  char key_[20];
  char value_[20];
  char url[160];
  
  if (key == nullptr || value == nullptr) {
        return false;
    }

    // Sprawdzenie długości danych
    if (strlen(key) >= sizeof(key_) ||
        strlen(value) >= sizeof(value_)) {
        return false;
    }

    // Kopiowanie danych do wewnętrznych tablic
    strcpy(key_, key);
    strcpy(value_, value);


    const int length = snprintf(url,sizeof(url),
                  "AT+HTTPPARA=\"URL\",\"http://%s/api/tlm/v2/set.php?ID=%s&D=%s&V=%s&U=--&IP=%s&RSSI=%s&BAT=75&L=%s&VER=1\"",
                                this->my_SERVERURL,
                                this->my_IMSI,
                                key_,
                                value_,
                                this->IP,
                                this->RSSI,
                                this->LOCALISATION
                              );

  if (length < 0 || static_cast<size_t>(length) >= sizeof(url)) {
      DEBUG_PRINTLN("[ERROR] Blad: adres URL nie miesci sie w buforze.");
    } else if (this->http_get_(url)) {
      DEBUG_PRINTLN("[send()] HTTP GET -> OK");
    } else {
      // Reset mikrokontrolera spowoduje ponowna inicjalizacje i reset SIM800L.
      this->reset_cpu_();
    }

    return true;
}

//przeciazenie funkcji
bool SALGSMv1::send(const char* key, double value) {
    char buffer[32];
    dtostrf(value, 0, 3, buffer);
    return send(key, buffer);
}

void SALGSMv1::IMSI() {
  char response[200] = {0};
  sendAT("AT+CIMI", response, sizeof(response), 2000);

  strncpy(my_IMSI, extractID(response), sizeof(my_IMSI) - 1);
  my_IMSI[sizeof(my_IMSI) - 1] = '\0';
}

void SALGSMv1::readNetworkData() {
  char response[200] = {0};
  clear(RSSI, sizeof(RSSI));
  clear(LOCALISATION, sizeof(LOCALISATION));

  sendAT("AT+CSQ", response, sizeof(response), 2000);
  char *csq = strstr(response, "+CSQ:");
  int rssi = 99;
  if (csq != nullptr && sscanf(csq, "+CSQ:%d", &rssi) == 1 &&
      rssi >= 0 && rssi <= 99) {
    snprintf(RSSI, sizeof(RSSI), "%d", rssi);
  }

  // Pobierz identyfikatory komorki do pozniejszego wyszukania w bazie nadajnikow.
  clear(response, sizeof(response));
  sendAT("AT+CREG=2", response, sizeof(response), 2000);
  DEBUG_PRINT("CREG config: ");
  DEBUG_PRINTLN(response);

  clear(response, sizeof(response));
  sendAT("AT+CREG?", response, sizeof(response), 2000);
  DEBUG_PRINT("CELL CREG (n,status,LAC,CI): ");
  DEBUG_PRINTLN(response);

  char lac[5] = {0};
  char cellId[5] = {0};
  char operatorCode[7] = {0};
  int cregMode = -1;
  int registrationStatus = -1;

  char *creg = strstr(response, "+CREG:");
  bool cellDataValid =
      creg != nullptr &&
      sscanf(creg, "+CREG:%d,%d,\"%4[0-9A-Fa-f]\",\"%4[0-9A-Fa-f]\"",
             &cregMode, &registrationStatus, lac, cellId) == 4 &&
      cregMode == 2 && (registrationStatus == 1 || registrationStatus == 5);

  clear(response, sizeof(response));
  sendAT("AT+COPS=3,2", response, sizeof(response), 2000);
  clear(response, sizeof(response));
  sendAT("AT+COPS?", response, sizeof(response), 2000);
  DEBUG_PRINT("OPERATOR COPS (numeric MCC/MNC): ");
  DEBUG_PRINTLN(response);

  int copsMode = -1;
  int copsFormat = -1;
  char *cops = strstr(response, "+COPS:");
  bool operatorDataValid =
      cops != nullptr &&
      sscanf(cops, "+COPS:%d,%d,\"%6[0-9]\"",
             &copsMode, &copsFormat, operatorCode) == 3 &&
      strlen(operatorCode) >= 5;

  if (cellDataValid && operatorDataValid) {
    snprintf(LOCALISATION, sizeof(LOCALISATION), "%.3s_%s_%s_%s",
             operatorCode, operatorCode + 3, lac, cellId);
  }

  DEBUG_PRINT("RSSI (CSQ): ");
  DEBUG_PRINTLN(RSSI[0] ? RSSI : "brak danych");
  DEBUG_PRINT("Lokalizacja sieci (MCC/MNC_LAC_CI): ");
  DEBUG_PRINTLN(LOCALISATION[0] ? LOCALISATION : "brak danych");
}

bool SALGSMv1::con_to_internet() {
  char response[200] = {0};
  uint8_t count = 0;
  STATUS = true;

  do {
    clear(response, sizeof(response));
    sendAT("AT+CGATT=1", response, sizeof(response), 3000);
    DEBUG_PRINT("[con()] AT+CGATT=1 -> ");
    DEBUG_PRINTLN(response);
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
    DEBUG_PRINT("[con()] AT+CSTT -> ");
    DEBUG_PRINTLN(response);
    ++count;
  } while (strstr(response, "OK") == nullptr && count < 3);
  if (strstr(response, "OK") == nullptr) {
    STATUS = false;
  }

  count = 0;
  do {
    clear(response, sizeof(response));
    sendAT("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", response, sizeof(response), 1000);
    DEBUG_PRINT("[con()] CONTYPE -> ");
    DEBUG_PRINTLN(response);
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
    DEBUG_PRINT("[con()] APN -> ");
    DEBUG_PRINTLN(response);
    ++count;
  } while (strstr(response, "OK") == nullptr && count < 3);
  if (strstr(response, "OK") == nullptr) {
    STATUS = false;
  }

  count = 0;
  do {
    clear(response, sizeof(response));
    sendAT("AT+SAPBR=1,1", response, sizeof(response), 2000);
    DEBUG_PRINT("[con()] AT+SAPBR=1,1 -> ");
    DEBUG_PRINTLN(response);
    ++count;
  } while (strstr(response, "OK") == nullptr && count < 3);
  if (strstr(response, "OK") == nullptr) {
    STATUS = false;
  }

  count = 0;
  do {
    clear(response, sizeof(response));
    sendAT("AT+SAPBR=2,1", response, sizeof(response), 2000);
    DEBUG_PRINT("[con()] AT+SAPBR=2,1 -> ");
    DEBUG_PRINTLN(response);
    ++count;
  } while (strstr(response, "OK") == nullptr && count < 3);
  if (strstr(response, "OK") == nullptr) {
    STATUS = false;
  }

  char *ptr = strstr(response, "+SAPBR:");
  if (ptr != nullptr && sscanf(ptr, "+SAPBR: %*d,%*d,\"%19[^\"]\"", IP) == 1) {
    DEBUG_PRINT("IP: ");
    DEBUG_PRINTLN(IP);
  } else {
    DEBUG_PRINTLN("Brak poprawnego adresu IP.");
    STATUS = false;
  }

  return STATUS;
}

bool SALGSMv1::http_get_(const char *cmd) {
  char response[100] = {0};
  bool requestSucceeded = false;
  bool STAT_R = false;
  uint8_t pom = 0;
  STATUS = true;

  clear(response, sizeof(response));
  sendAT("AT+HTTPINIT", response, sizeof(response), 2000);
  if (strstr(response, "OK") == nullptr) {
    STATUS = false;
    DEBUG_PRINTLN("[http_get] AT+HTTPINIT !!!!! STATUS = false !!!!");
  }
  delay(1000);

  clear(response, sizeof(response));
  sendAT("AT+HTTPPARA=\"CID\",1", response, sizeof(response), 2000);
  if (strstr(response, "OK") == nullptr) {
    STATUS = false;
    DEBUG_PRINTLN("[http_get] AT+HTTPPARA !!!!! STATUS = false !!!!");
  }
  delay(2000);

  clear(response, sizeof(response));
  sendAT(cmd, response, sizeof(response), 2000);
  if (strstr(response, "URL") == nullptr) {
    STATUS = false;
    DEBUG_PRINTLN("[http_get] URL !!!!! STATUS = false !!!!");
  }
  delay(2000);

  clear(response, sizeof(response));
  sendAT("AT+HTTPACTION=0", response, sizeof(response), 2000);
  if (strstr(response, "OK") == nullptr) {
    STATUS = false;
    DEBUG_PRINTLN("[http_get] AT+HTTPACTION !!!!! STATUS = false !!!!");
  }
  delay(4000);

  //try read all buffer from GSM MODULE
  pom = 0;
  do{
    clear(response, sizeof(response));
    sendAT("AT+HTTPREAD=0,100", response, sizeof(response), 5000);
    if (strstr(response, "SETOK") == nullptr) {
      DEBUG_PRINTLN("[http_get] AT+HTTPREAD !!!!! STAT_R = false !!!!");
    } else STAT_R = true;
    delay(2000);
    requestSucceeded = strstr(response, "SETOK") != nullptr;
    pom ++;
  }while((STAT_R == false) and (pom < 3));

  if (STAT_R == false) STATUS = false;

  clear(response, sizeof(response));
  sendAT("AT+HTTPTERM", response, sizeof(response), 2000);
  if (strstr(response, "OK") == nullptr) {
    DEBUG_PRINTLN("[http_get] AT+HTTPTERM !!!!! STATUS = false !!!!");
    STATUS = false;
  }

  if (!STATUS) {
    DEBUG_PRINTLN("[http_get] reset()");
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
  if (DEBUG) { DEBUG_PRINT("[sendAT] "); DEBUG_PRINTLN(cmd);}
  delay(400);
  const unsigned long start = millis();
  while (millis() - start < timeout) {
    while (my_serial->available()) {
      const char c = static_cast<char>(my_serial->read());
      //if (DEBUG) debug_serial->write(c);
      if (index < outSize - 1 && c != ' ' && c != '\n' && c != '\r' && c != '\t') {
        out[index++] = c;
      }
    }
    delay(1);
  }

  out[index] = '\0';
  if (DEBUG) {
    DEBUG_PRINT("[sendAT] res ->");
    DEBUG_PRINTLN(out);
    DEBUG_PRINTLN("[sendAT END]");
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
    DEBUG_PRINTLN("RESET MCU");
  }
  wdt_enable(WDTO_1S);
  while (true) {
    // Oczekiwanie na reset watchdogiem.
  }
}

void SALGSMv1::setDEBUG(bool state) {
  DEBUG = state;
}
