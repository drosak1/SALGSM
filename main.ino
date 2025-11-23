#include "Arduino.h"
#include "SALGSMv1.h"
#include <EEPROM.h>
#include <SoftwareSerial.h>
//#include <avr/wdt.h>

SoftwareSerial GSM(2, 3);  // RX = D2, TX = D3

#define EEPROM_SIZE 512  // Arduino Nano ma 1024 bajty EEPROM
#define STRING_ADDR 0    // Adres początkowy dla stringa

SALGSMv1 GSM_dev("sensor.net", true); 
char KEY[100] = "";

char input[200];

const char * readFromEEPROM(int addr);

void setup() {
  // Wyłącz watchdog na starcie (ważne przy restartach)
  Serial.begin(9600);
  Serial.setTimeout(1000);  
  GSM.begin(9600);
  delay(100);

  // wdt_disable();
  // delay(100);
  // // Włącz watchdog – maksymalny czas: 8 sekund
  //sei();                     // wymusza globalne przerwania
  // wdt_enable(WDTO_8S);
  // wdt_reset();

  const char* str = readFromEEPROM(0);
  memcpy(KEY, str, strlen(str)+1); // +1 dla '\0'

  GSM_dev.set_serial(&GSM);

  if (GSM_dev.debug()) Serial.println("\nSTART\n");

  GSM_dev.STATUS = true;
  Serial.println(GSM_dev.init());


  Serial.println(GSM_dev.IMSI());

  //GSM_dev.set_band("ALL_BAND");

  GSM_dev.con_to_internet();


  //GSM_dev.get_ip();
  //GSM_dev.location_area_code();

}


/*
AT+CIMI
901405180011350


START ->
AT
AT+CGATT=1
AT+CSTT="internet"
AT+SAPBR=3,1,"CONTYPE","GPRS"
AT+SAPBR=3,1,"APN","sensor.net"
AT+SAPBR=1,1
AT+SAPBR=2,1

AT+HTTPINIT
AT+HTTPPARA="CID",1
AT+HTTPPARA="URL","http://dlb.com.pl/api/v1/telemetry.php?ID=901405180011350&KEY=9999&payload=xxyy"
AT+HTTPPARA="URL","http://dlb.com.pl/api/v1/telemetry.php?ID=123456&KEY=9999&phone=48609105069&sms=GSMTEST"
AT+HTTPACTION=0
AT+HTTPREAD=0,33
AT+HTTPREAD=33,66
AT+HTTPTERM
*/

void loop() {
  char buf[100];                  // bufor docelowy
  // nic
  // if(GSM_dev.STATUS!="OK") {
  //   Serial.println("RESET GSM");
  //   GSM_dev.reset();
  //   GSM_dev.con_to_internet();
  // }

  //Serial.println(GSM_dev.http_get_("http://dlb.com.pl/api.php?name=demo&device="+GSM_dev.IMSI()+"&command=TIME",2000));

  //if (Serial1.available()) Serial.write(Serial1.read());
  //if (Serial.available())  Serial1.write(Serial.read());
  



 // Sprawdzaj czy są dostępne dane do odczytu
 // if (Serial1.available() > 0) Serial.write(Serial1.read());

  //if(strstr(buf,"AT+ID?")) GSM_dev.IMSI();
  delay(1000);
  Serial.println(input);
  //wdt_reset();
  // if(receivedString.indexOf("AT+ID?")>-1) Serial.println(GSM_dev.IMSI());
  if (strstr(input, "AT+ID?") != NULL) {
      Serial.println(GSM_dev.IMSI());
      memset(input, 0, sizeof(input)); //czysci tablice
  }

  // if(receivedString.indexOf("AT+KEY?")>-1) 
  if (strstr(input, "AT+KEY?") != NULL) {
    Serial.println(readFromEEPROM(0));
    memset(input, 0, sizeof(input)); //czysci tablice
  }

  // if(receivedString.indexOf("AT+INIT=ON")>-1) Serial.println(GSM_dev.init());

  // //GSM_dev.set_band("ALL_BAND");

  // if(receivedString.indexOf("AT+INTERNET=ON")>-1) GSM_dev.con_to_internet();

  // if (receivedString.indexOf("AT+IP?")>-1) GSM_dev.get_ip();

  // if(receivedString.indexOf("AT+LOCATION?")>-1) GSM_dev.location_area_code();

  // if(receivedString.indexOf("AT+HTTPINIT?")>-1) GSM_dev.sendAT("AT+HTTPINIT",1000);

  // if(receivedString.indexOf("AT+HTTPPARA?")>-1) GSM_dev.sendAT("AT+HTTPPARA=\"CID\",1",1000);

  // if(receivedString.indexOf("AT+TIME?")>-1) {
  //   char url[256];
  //   String ID_="901405180011350";
  //   String KEY_ ="9999";
  //   String payload_ = "dupa";
  //   snprintf(url, sizeof(url), "AT+HTTPPARA=\"URL\",\"http://dlb.com.pl/api/v1/telemetry.php?ID=%s&KEY=%s&payload=%s\"",ID_.c_str(), KEY_.c_str(), payload_.c_str());
  //   GSM_dev.http_get_(url,1000);
  // }

  // if(receivedString.indexOf("AT+KEY=")>-1){
  if (strstr(input, "AT+KEY=") != NULL) {
    const char* pom_key = GSM_dev.extractID(input);
    Serial.println(pom_key);
    writeToEEPROM(0, pom_key);
    memset(input, 0, sizeof(input)); //czysci tablice
  }

  // if(receivedString.indexOf("AT+SEND=")>-1){
  //   String text = receivedString;
  //   text.remove(0,text.indexOf("=")+1); // usuwa AT+...=
  //   //if (GSM_dev.debug()) String.println(text);
  //   //GSM_dev.http_get_("http://dlb.com.pl/api/v1/telemetry.php?ID="+GSM_dev.IMSI()+"&KEY="+KEY+"&command=TIME&payload="+text);
  // }


  //if(receivedString.indexOf("AT+SENDSMS=+")>-1){
  if (strstr(input, "AT+SENDSMS=+") != NULL) {
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

    char url[160];
    char ID_[50] = "901405180011350";
    char KEY_[10] = "9999";
    snprintf(url, sizeof(url), "AT+HTTPPARA=\"URL\",\"http://dlb.com.pl/api/v1/telemetry.php?ID=%s&KEY=%s&phone=%s&sms=%s\"",ID_, KEY_, phone, message);
    //int x=0;
    // while(url[x] != NULL){
    //   Serial.write(url[x]);
    //   delay(1);
    //   x++;
    // }
    // Serial.write("\n");

    Serial.println("OK ;-) ");
    delay(1000);
    //Serial.println(url);
    http_get_(url);
    //GSM_dev.http_get_(url,1000);

    memset(input, 0, sizeof(input)); //czysci tablice
  }

//https://debug.dlb.com.pl/api/v1/telemetry.php?ID=123456&KEY=9999&mail=dawid.rosak@gmail.com&mail_title=ALARM

  //if(receivedString.indexOf("AT+SENDMAIL=")>-1){
  if (strstr(input, "AT+SENDMAIL=") != NULL) {
    char mail[40];
    char message[50];
    char title[50];

    if (parse_(input, mail, title, message)) {
        // Serial.println(phone);    // 48609105069
        // Serial.println(message);  // dupa
    }else
    {
        Serial.println("Błąd: Nie znaleziono średników!");
        return;
    }

    char url[160];
    char ID_[50] = "901405180011350";
    char KEY_[10] = "9999";
    snprintf(url, sizeof(url), "AT+HTTPPARA=\"URL\",\"http://dlb.com.pl/api/v1/telemetry.php?ID=%s&KEY=%s&mail=%s&mail_title=%s&payload=%s\"",ID_, KEY_, mail, title,message);
    int x=0;
    // while(url[x] != NULL){
    //   Serial.write(url[x]);
    //   delay(1);
    //   x++;
    // }
    // Serial.write("\n");

    Serial.println("OK ;-) ");
    delay(1000);
    //Serial.println(url);
    http_get_(url);
    //GSM_dev.http_get_(url,1000);

    memset(input, 0, sizeof(input)); //czysci tablice
  }

  if (strstr(input, "AT+DEBUG") != NULL) {
      GSM_dev.setDEBUG(true);
      memset(input, 0, sizeof(input)); //czysci tablice
  }
  
  //if(receivedString.indexOf("AT+DIAG?")>-1) 
  if (strstr(input, "AT+DIAG?") != NULL) { 
    GSM_dev.networkDiagnosis(); 
    memset(input, 0, sizeof(input)); //czysci tablice
  }

  //if (Serial1.available()) Serial1.write(Serial1.read());
  //if (Serial.available()) Serial1.write(Serial.read());
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







// Funkcja zapisująca string do EEPROM
void writeToEEPROM(int addr, const char * data) {
  int pos = addr;
  
  for (int i = 0; i < strlen(data); i++) {
    EEPROM.update(pos++, data[i]);
  }

  EEPROM.update(pos, '\0');  // znak końca
}

// Funkcja odczytująca string z EEPROM
const char * readFromEEPROM(int addr) {
  char out[100] = "";
  int pos = addr;
  char c;
  uint8_t step=0;
  while (((c = EEPROM.read(pos++)) != '\0') and (step<100)) {
    out[step] = c;
    step ++;
  }
  return out;
}

// Funkcja czyszcząca EEPROM
void clearEEPROM(int startAddr, int length) {
  for (int i = startAddr; i < startAddr + length && i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0xFF); // Lub 0 - zależy od preferencji
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


// bool parse_(const char* input, char* phone, char* text) {

//     // 1. Znajdź znak '='
//     const char* eq = strchr(input, '=');
//     if (!eq) return false;
//     eq++;  // przejdź za '='

//     // 2. Pierwszy znak telefonu to '+' — pomijamy
//     if (*eq == '+') eq++;

//     // 3. Kopiuj numer do ';'
//     const char* semicolon = strchr(eq, ';');
//     if (!semicolon) return false;

//     int phoneLen = semicolon - eq;
//     strncpy(phone, eq, phoneLen);
//     phone[phoneLen] = '\0';

//     // 4. Dalej jest tekst: "dupa"
//     const char* txtStart = semicolon + 1;
//     const char* txtEnd = strchr(txtStart, ';');
//     if (!txtEnd) return false;

//     int textLen = txtEnd - txtStart;
//     strncpy(text, txtStart, textLen);
//     text[textLen] = '\0';

//     return true;
// }





void http_get_(const char* cmd){
  //url += "&lacDec="+String(this->lacDec)+"&cellDec="+String(this->cellDec)+"&netop="+this->network_operator;

  GSM.println("AT+HTTPINIT");

  delay(2000);

  GSM.println("AT+HTTPPARA=\"CID\",1");

  delay(2000);

  GSM.println(cmd);

  delay(2000);

  GSM.println("AT+HTTPACTION=0");

  delay(2000);
  
  GSM.println("AT+HTTPTERM");

}
