#include "Arduino.h"
#include "SALGSMv1.h"
#include <EEPROM.h>
#include <SoftwareSerial.h>
//#include <avr/wdt.h>

SoftwareSerial GSM_serial(2, 3);  // RX = D2, TX = D3 (Dzielnik napięcia)
/* _____                                   _____
  |     |                                 |     |
  |  M  |-> Rx <-------------------> Tx <-|  G  | 
  |  C  |                                 |  S  |
  |  U  |-> Tx[D3] <- (5V<->3.3V) -> Rx <-|  M  |
  |_____|                                 |_____|
*/
SALGSMv1 GSM_dev(&GSM_serial, "sensor.net", true);

#define EEPROM_SIZE 1024  // Arduino Nano ma 1024 bajty EEPROM
#define STRING_ADDR 0    // Adres początkowy dla stringa

char input[200];


void setup() {
  Serial.begin(9600);
  Serial.setTimeout(1000);  
  GSM_serial.begin(9600);

  Serial.println("START SYSTEMU");

  //GSM_dev.reset();
  //GSM_dev.init();

  
}



void loop() {
 delay(1000);

 //AT+SENDSMS=+48609105069;hejhej;TESTTEST;;
  if (strstr(input, "SENDSMS") != NULL) {

    Serial.print("Znaleziono SENDSMS w: ");
    Serial.println(input);

    char phone[20];
    char message[50];
    char title[10];

    if (parse_(input, phone, message, title)) {
        Serial.println(phone);    // 48609105069
        Serial.println(message);  // dupa
    }else
    {
        Serial.println("Błąd: Nie znaleziono średników!");
        return;
    }

    char url[250];
    char KEY_[10] = "9999";
    snprintf(url, sizeof(url), "AT+HTTPPARA=\"URL\",\"http://dlb.com.pl/api/v1/telemetry.php?ID=%s&KEY=%s&phone=%s&sms=%s\"",GSM_dev.my_IMSI, KEY_, phone, message);
    Serial.println("url -> OK ;-) ");

    GSM_dev.http_get_(url);

    memset(input, 0, sizeof(input)); //czysci tablice
  }
 
  
}

void serialEvent() {
    int pom=0;
    while (Serial.available()) {
        char c = Serial.read();
        input[pom] = c;
        pom++;
        //Serial.print(c);
        // obsłuż odebrany znak
    }
}

bool parse_(const char* input, char* phone, char* text, char* title) {
    // 1. Znajdź znak '='
    const char* eq = strchr(input, '=');
    if (!eq) return false;
    eq++;  // przejdź za '='

    // 2. Pomijamy '+' jeśli jest
    if (*eq == '+') eq++;

    const char* start = eq;
    const char* sep;

    // 3. Numer telefonu – od '=' lub '+' do pierwszego ';'
    sep = strchr(start, ';');
    if (!sep) return false;
    int len = sep - start;
    strncpy(phone, start, len);
    phone[len] = '\0';

    // 4. Tekst – od pierwszego ';' do drugiego ';'
    start = sep + 1;
    sep = strchr(start, ';');
    if (!sep) return false;
    len = sep - start;
    strncpy(text, start, len);
    text[len] = '\0';

    // 5. Trzecia zmienna – od drugiego ';' do trzeciego ';'
    start = sep + 1;
    sep = strchr(start, ';');
    if (!sep) return false;
    len = sep - start;
    strncpy(title, start, len);
    title[len] = '\0';

    return true;
}