#include "Arduino.h"
#include "SALGSMv1.h"
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <avr/wdt.h>

void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));

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

extern int __bss_end;
extern int __heap_start;

void setup() {
  MCUSR = 0;      // bardzo ważne
  wdt_disable(); 

  clearRAM();     // opcjonalnie

  pinMode(5, OUTPUT);
  
  Serial.begin(9600);
  Serial.setTimeout(1000);  
  GSM_serial.begin(9600);

  Serial.println("");
  Serial.println("");
  Serial.println("START SYSTEMU");

  digitalWrite(5, LOW);
  delay(100);
  digitalWrite(5, HIGH);
  delay(15000);

  //GSM_dev.reset();
  //wdt_reset();
  GSM_dev.init();

  //wdt_enable(WDTO_8S);
  
}



void loop() {
 delay(1000);
 //wdt_reset();

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

  //AT+SENDMAIL=david@wp.pl;hejhej;TESTTEST;;
  if (strstr(input, "AT+SENDMAIL=") != NULL) {
    char mail[40];
    char message[50];
    char title[50];

    if (parse_(input, mail, title, message)) {
        // Serial.println(phone);
        // Serial.println(message);
    }else
    {
        Serial.println("Błąd: Nie znaleziono średników!");
        return;
    }

    char url[160];
    char ID_[50] = "901405180011350";
    char KEY_[10] = "9999";
    snprintf(url, sizeof(url), "AT+HTTPPARA=\"URL\",\"http://dlb.com.pl/api/v1/telemetry.php?ID=%s&KEY=%s&mail=%s&mail_title=%s&message=%s\"",ID_, KEY_, mail, title,message);
    int x=0;
    // while(url[x] != NULL){
    //   Serial.write(url[x]);
    //   delay(1);
    //   x++;
    // }
    // Serial.write("\n");

    Serial.println("url -> OK ;-) ");

    GSM_dev.http_get_(url);

    memset(input, 0, sizeof(input)); //czysci tablice
  }


  if (strstr(input, "AT+DEBUG=1") != NULL) {
    GSM_dev.setDEBUG(true);
    memset(input, 0, sizeof(input)); //czysci tablice
  }

  if (strstr(input, "AT+DEBUG=0") != NULL) {
    GSM_dev.setDEBUG(false);
    memset(input, 0, sizeof(input)); //czysci tablice
  }
  
  //if(receivedString.indexOf("AT+DIAG?")>-1) 
  if (strstr(input, "AT+DIAG?") != NULL) { 
    //GSM_dev.networkDiagnosis(); 
    memset(input, 0, sizeof(input)); //czysci tablice
  }

}


void clearRAM() {
  uint8_t* p = (uint8_t*)&__bss_end;

  while (p < (uint8_t*)RAMEND) {
    *p++ = 0;
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