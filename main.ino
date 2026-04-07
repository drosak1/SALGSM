#include "Arduino.h"
#include "SALGSMv1.h"
#include <EEPROM.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

// Sketch uses 10070 bytes (31%) of program storage space. Maximum is 32384 bytes.
// Global variables use 1397 bytes (68%) of dynamic memory, leaving 651 bytes for local variables. Maximum is 2048 bytes.
// Po przekroczeniu 68% dynamicznej pamieci system nie działa -> propozycja to przejscie na kontroler ATMEGA4808-AF IC: mikrokontroler AVR; TQFP32; Interfejs: I2C,SPI,UPDI,USART x4

void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));

//#include <SoftwareSerial.h>
//SoftwareSerial GSM_serial(2, 3);  // RX = D2, TX = D3 (Dzielnik napięcia)

//#include <AltSoftSerial.h>
//AltSoftSerial GSM_serial; // RX=8, TX=9

//#include <AltSoftSerial.h>
//AltSoftSerial GSM_serial; // RX=8, TX=9

#include <NeoSWSerial.h>
NeoSWSerial GSM_serial(7, 8); //

/* _____                                   _____
  |     |                                 |     |
  |  C  |-< Rx[D7] <---------------- Tx <-|  G  | 
  |  P  |                                 |  S  |
  |  U  |-> Tx[D8] -> (5V -> 3.3V)-> Rx >-|  M  |
  |_____|-< [D4] <----GSM RESET---- RST <-|_____|
  |     |-< [D2] <-LICZNIK
  |_____|
*/
SALGSMv1 GSM_dev(GSM_serial, "sensor.net", true);

#define EEPROM_SIZE 1024  // Arduino Nano ma 1024 bajty EEPROM
#define STRING_ADDR 0    // Adres początkowy dla stringa

char input[201];

char KEY_[10] = "9999";

extern int __bss_end;
extern int __heap_start;

volatile unsigned long licznik = 0; //volatile - inaczej kompilator może robić optymalizacje i będą błędy

bool s_event = false;

unsigned long previousMillis = 0;

const unsigned long interval = 15UL * 60UL * 1000UL; // 15 minut w ms


volatile unsigned long lastInterrupt = 0;

volatile bool przerwanie = false;

uint8_t startAddr = 0;

struct data {
  uint16_t write_counter;
  unsigned long value;
};

data MyData;

void isr() {
  unsigned long now = millis();
  if (now - lastInterrupt > 70) {
    licznik++;
    lastInterrupt = now;
    przerwanie = true;
  }
}

void setup() {
  MCUSR = 0;      // bardzo ważne
  wdt_disable(); 

  clearRAM();     // opcjonalnie

  pinMode(4, OUTPUT);

  pinMode(2, INPUT); //przerwanie

  Serial.begin(9600);
  Serial.setTimeout(1000);  
  GSM_serial.begin(9600);

  /////////////////////EEPROM //////////////////////////////////////////////////////////
  do{
    EEPROM.get(startAddr, MyData);
    if (MyData.write_counter==65535){ //czyszczenie EEPROM przy pierwszym uruchomieniu licznika
      for (int i = 0; i < EEPROM.length(); i++) EEPROM.update(i, 0);
    }
    else if (MyData.write_counter>60000) startAddr = startAddr + sizeof(MyData) + 1;
    Serial.print("adr-> "); Serial.print(startAddr); Serial.print(" E->w_c-> "); Serial.print(MyData.write_counter); Serial.print(" E->val-> "); Serial.println(MyData.value);
  }while(MyData.write_counter>60000);

  licznik = MyData.value;
  /////////////////////EEPROM  END/////////////////////////////////////////////////////

  Serial.println("");
  Serial.println("START v1");

  digitalWrite(4, LOW);
  delay(100);
  digitalWrite(4, HIGH);
  delay(15000);

  attachInterrupt(digitalPinToInterrupt(2), isr, FALLING);

  GSM_dev.init();

}



void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    Serial.print(licznik);

    noInterrupts();       // wyłącz przerwania
      MyData.value = MyData.value + licznik;
      licznik = 0;
    interrupts();         // włącz przerwania

    MyData.write_counter = MyData.write_counter + 1;
    if (MyData.write_counter>60000) { startAddr = startAddr + sizeof(MyData) + 1; MyData.write_counter = 1; }
    EEPROM.put(startAddr, MyData);

    // Tutaj funkcja co 15 minut
    Serial.println(" - przerwanie 15 minut!");
      char url[200];
      char pom_buf[20];
      sprintf(pom_buf, "%d", licznik);
      snprintf(url, sizeof(url), "AT+HTTPPARA=\"URL\",\"http://dlb.com.pl/api/tlm/v1/set.php?did=1&imsi=%s&key=%s&ip=%s&payload=%s\"",GSM_dev.my_IMSI, KEY_, GSM_dev.IP, pom_buf);
      Serial.println("url -> OK ;-) ");
      if(GSM_dev.http_get_(url))  Serial.println("htt_get_()!");
      memset(input, 0, sizeof(input)); //czysci tablice
  }
 
  if (przerwanie) { // zbocze opadające
    Serial.print("*");
    przerwanie = false;
  }

  //delay(5);
  
  if(s_event){
    //AT+SENDSMS=+48609105069;TYTUL;WIADOMOSC-hej-hej;;
    // if (strstr(input, "SENDSMS") != NULL) {
    //   s_event = false;
    //   Serial.println(input);

    //   char phone[20];
    //   char message[50];
    //   char title[10];

    //   if (parse_(input, phone, title, message)) {
    //       Serial.println(phone);    // 48609105069
    //       Serial.println(message);  // dupa
    //   } else
    //   {
    //       Serial.println("[F];!");
    //       return;
    //   }
    //   char url[200];
    //   snprintf(url, sizeof(url), "AT+HTTPPARA=\"URL\",\"http://dlb.com.pl/api/v1/telemetry.php?ID=%s&KEY=%s&phone=%s&sms=%s\"",GSM_dev.my_IMSI, KEY_, phone, message);
    //   Serial.println("url -> OK ;-) ");
    //   GSM_dev.http_get_(url);
    //   memset(input, 0, sizeof(input)); //czysci tablice
    // }

    //AT+SENDMAIL=david@wp.pl;TYTUL;WIADOMOSC;;
    // if (strstr(input, "AT+SENDMAIL=") != NULL) {
    //   char mail[40];
    //   char message[50];
    //   char title[10];

    //   if (parse_(input, mail, title, message)) {

    //   } else
    //   {
    //       Serial.println("[F];!");
    //       return;
    //   }
    //   char url[200];
    //   snprintf(url, sizeof(url), "AT+HTTPPARA=\"URL\",\"http://dlb.com.pl/api/v1/telemetry.php?ID=%s&KEY=%s&mail=%s&mail_title=%s&message=%s\"",GSM_dev.my_IMSI, KEY_, mail, title, message);
    //   Serial.println("url -> OK ;-) ");
    //   GSM_dev.http_get_(url);
    //   memset(input, 0, sizeof(input)); //czysci tablice
    // }


    // if (strstr(input, "AT+DEBUG=1") != NULL) {
    //   GSM_dev.setDEBUG(true);
    //   memset(input, 0, sizeof(input)); //czysci tablice
    // }

    // if (strstr(input, "AT+DEBUG=0") != NULL) {
    //   GSM_dev.setDEBUG(false);
    //   memset(input, 0, sizeof(input)); //czysci tablice
    // }
    
    // //if(receivedString.indexOf("AT+DIAG?")>-1) 
    // if (strstr(input, "AT+DIAG?") != NULL) { 
    //   //GSM_dev.networkDiagnosis(); 
    //   memset(input, 0, sizeof(input)); //czysci tablice
    // }

  }//s_event

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
        s_event = true;
        char c = Serial.read();
        input[pom] = c;
        input[pom+1]= '\0';
        delay(1);
        pom++;
        if(pom>200) pom = 0;
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