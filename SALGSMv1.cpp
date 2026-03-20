#include "SALGSMv1.h"
//#include <avr/wdt.h>

SALGSMv1::SALGSMv1(Stream* serial, const char* APN, bool debug){
  this->my_serial = serial;
  strncpy(this->my_APN, APN, sizeof(this->my_APN)-1);
  this->my_APN[sizeof(this->my_APN)-1] = '\0';
}

const char* SALGSMv1::init(void){

  char response[200] = {0};

  this->sendAT("AT+CSQ", response, sizeof(response), 2000);

  Serial.println(response);
  this->clear(response, sizeof(response));

  delay(1200);


  this->IMSI();
  Serial.print("my_IMSI: "); Serial.println(my_IMSI);

  this->con_to_internet();

}

void SALGSMv1::IMSI(void){
  char response[200] = {0};

  this->sendAT("AT+CIMI", response, sizeof(response), 2000);
  this->extractID(response);

  strncpy(this->my_IMSI, this->extractID(response), sizeof(this->my_IMSI)-1);
  response[sizeof(my_IMSI)-1] = '\0';

  // //const char * res = this->IMSI();
  // Serial.print("my_IMSI - ");
  // Serial.print(my_IMSI);
  // Serial.print(" -> size -> ");
  // Serial.print(sizeof(my_IMSI));
  // Serial.println(" /");
}

bool SALGSMv1::con_to_internet(void){
  char response[200] = {0};

  do{
    this->clear(response, sizeof(response));
    this->sendAT("AT+CGATT=1", response, sizeof(response), 3000);
    Serial.print("[con_to_internet()] AT+CGATT=1 -> "); Serial.println(response);
  } while(strstr(response, "OK") == NULL);

  do{
    this->clear(response, sizeof(response));
    this->sendAT("AT+CSTT=\"internet\"", response, sizeof(response), 1500);
    Serial.print("[con_to_internet()] AT+CSTT=\"internet\" -> "); Serial.println(response);
  } while(strstr(response, "OK") == NULL);

  do{
    this->clear(response, sizeof(response));
    this->sendAT("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", response, sizeof(response), 1000);
    Serial.print("[con_to_internet()] AT+CSTT=\"internet\" -> "); Serial.println(response);
  } while(strstr(response, "OK") == NULL);

  do{
    this->clear(response, sizeof(response));
    char c_payload[100];
    snprintf(c_payload, sizeof(c_payload),"AT+SAPBR=3,1,\"APN\",\"%s\"", this->my_APN);
    this->sendAT(c_payload, response, sizeof(response), 1000);
    Serial.print("[con_to_internet()] AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\" -> "); Serial.println(response);
  } while(strstr(response, "OK") == NULL);

  do{
    this->clear(response, sizeof(response));
    this->sendAT("AT+SAPBR=1,1", response, sizeof(response), 2000);
    Serial.print("[con_to_internet()] AT+SAPBR=1,1 -> "); Serial.println(response);
  } while(strstr(response, "OK") == NULL);
 
  do{
    this->clear(response, sizeof(response));
    this->sendAT("AT+SAPBR=2,1", response, sizeof(response), 2000);
    Serial.print("[con_to_internet()] AT+SAPBR=1,1 -> "); Serial.println(response);
  } while(strstr(response, "OK") == NULL);

  //response posiada na ten moment numer IP
 char* ptr = strstr(response, "+SAPBR:");
  if (ptr) {
    if (sscanf(ptr, "+SAPBR: %*d,%*d,\"%19[^\"]\"", this->IP) == 1) {
      Serial.print("IP: ");
      Serial.println(this->IP);
    } else {
      Serial.println("Blad parsowania IP");
    }
  } else {
    Serial.println("Brak +SAPBR");
  }
  
  return this->STATUS;
}


void SALGSMv1::http_get_(const char* cmd){
  char response[200] = {0};
  //url += "&lacDec="+String(this->lacDec)+"&cellDec="+String(this->cellDec)+"&netop="+this->network_operator;


  this->clear(response, sizeof(response));
  this->sendAT("AT+HTTPINIT", response, sizeof(response), 2000);

  delay(2000);

  this->clear(response, sizeof(response));
  this->sendAT("AT+HTTPPARA=\"CID\",1", response, sizeof(response), 2000);

  delay(2000);

  this->clear(response, sizeof(response));
  this->sendAT(cmd, response, sizeof(response), 2000);

  delay(2000);

  this->clear(response, sizeof(response));
  this->sendAT("AT+HTTPACTION=0", response, sizeof(response), 2000);

  delay(4000);
  
  this->clear(response, sizeof(response));
  this->sendAT("AT+HTTPREAD=0,100", response, sizeof(response), 2000);
    
  delay(2000);

  this->clear(response, sizeof(response));
  this->sendAT("AT+HTTPTERM", response, sizeof(response), 2000);
  
}

void SALGSMv1::clear(char* buf, size_t size) {
  memset(buf, 0, size);
}

const char* SALGSMv1::sendAT(const char* cmd, char* out, size_t outSize, unsigned long timeout = 2000) {
  size_t idx = 0;

  this->my_serial->println(cmd);
  if (DEBUG) Serial.write("[DEBUG] ");
  unsigned long start = millis();
  while (millis() - start < 2000) {
    while (this->my_serial->available()) {
      char c = this->my_serial->read();
      if (DEBUG) Serial.write(c);  // wypisuj bezpośrednio

      if (idx < outSize - 1) {
        out[idx++] = c;
      }
    }
  }
  if (DEBUG) Serial.write("[DEBUG END] ");
  out[idx] = '\0'; // ✅ zakończenie stringa
}

const char* SALGSMv1::extractID(const char* resp)
{
    static char imsi[32];   // miejsce na wynik
    int j = 0;

    // przejdź przez cały tekst i wyciągnij tylko cyfry
    for (int i = 0; resp[i] != '\0'; i++) {
        if (resp[i] >= '0' && resp[i] <= '9') {
            if (j < sizeof(imsi) - 1) {
                imsi[j++] = resp[i];
            }
        }
    }

    imsi[j] = '\0'; // zakończ string
    return imsi;
}

bool SALGSMv1::isModuleAlive(void) {
  char response[200] = {0};
  this->sendAT("AT", response, sizeof(response), 10000);
  return (strstr(response, "OK") != NULL);
}

bool SALGSMv1::reset(void){
  char response[200] = {0};

  do{
    this->clear(response, sizeof(response));
    this->sendAT("AT+CFUN=1,1", response, sizeof(response), 10000);
    Serial.print("[reset()] AT+CFUN=1,1 -> "); Serial.println(response);
  } while(strstr(response, "OK") == NULL);
}