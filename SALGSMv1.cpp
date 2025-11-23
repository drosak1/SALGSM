#include "SALGSMv1.h"
//#include <avr/wdt.h>

SALGSMv1::SALGSMv1(String APN, bool debug){
   this->my_APN = APN;
   this->DEBUG = debug;
}

String SALGSMv1::init(void){
  this->reset();
  return "=== Inicjalizacja SALGSMv1 ===";
}

String SALGSMv1::sendAT(const char* cmd, int wait) {
  //String wynik = "";
  char wynik[160]; 

  uint16_t pom = 0;
  uint16_t n_pom=0;
  delay(1000);

  while(cmd[pom] != NULL){
    my_serial->write(cmd[pom]);
    pom++;
  } 
  my_serial->write("\n");
  //my_serial->write("\r");
  pom=0;

  //if(DEBUG) Serial.print(" |");
  char c;
  //wdt_reset();
  // while ((n_pom < wait)) { //} and  not(strstr(wynik,"OK"))){
  //   if (my_serial->available())  { 
  //     c = my_serial->read(); 
  //     //Serial.print(c);
  //     if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
  //       wynik[pom] = c;
  //       //if(DEBUG) Serial.print(c);
  //       pom++;
  //     }
  //   }
  //   n_pom++;
  //   delay(1);
  // }

  delay(2000);
  //wynik = clean(wynik);
  
  if(DEBUG){
      //Serial.print(" |>> cmd >> ");
      //Serial.print(cmd);
      //Serial.print(" >> answere >> ");
      pom = 0;
      // while(wynik[pom] != NULL) {
      //   Serial.write(wynik[pom]);
      //   pom++;
      // }
      //Serial.print(wynik);
      //Serial.print(" >> remove AT prefix >> ");
      //Serial.print(removeATPrefix(cmd));
      //Serial.println(" >>>> "); 
  }

  if(strstr(wynik,"ERROR")) this->STATUS = false;
  if(strstr(wynik,"OK")) this->STATUS = true;
  //if(wynik.indexOf(HTTPPARA)>-1) and (cmd.indexOf(HTTPPARA)>-1)) )

  // if(strstr(wynik,"+HTTPACTION")){
  //   // +HTTPACTION: 0,200,41 
  //   char* last = strrchr(wynik, ',');   // znajdź ostatni przecinek

  //   int value = 0;
  //   if (last != NULL) {
  //       this->my_lengthToRead = atoi(last + 1);       // zamień tekst po przecinku na liczbę
  //   }
  //   Serial.print("\n last +HTTPACTION -> ");
  //   Serial.println(this->my_lengthToRead);            // wypisze: 65
  // }

  // if(wynik.indexOf(removeATPrefix("+HTTPREAD"))>-1){
  //   //Serial.println(extractHttpData(wynik));
  //   this->BUFF += extractHttpData(wynik);
  // }

  if(strstr(wynik,"+CIMI")){
    const char* id_cstr = extractID(wynik);
    memcpy(this->my_IMSI, id_cstr, strlen(id_cstr) + 1);   // +1 dla znaku końca '\0'
    //if (DEBUG) Serial.println(this->my_IMSI);
    
  }

  return wynik;
}


void SALGSMv1::set_serial(Stream * s){
   this->my_serial = s;   // poprawne przypisanie
}

const char* SALGSMv1::IMSI(void){

  if (strlen(this->my_IMSI)>8){
  }
  else{
    do {
      this->STATUS=true;
      this->sendAT("AT+CIMI",1200);
    } while(this->STATUS==false);
  }
  return this->my_IMSI;
}

String SALGSMv1::CSQ(void){
  return this->sendAT("AT+CSQ",1000);
}

String SALGSMv1::networks(void){
  return this->sendAT("AT+COPS=?",10000);
}

String SALGSMv1::bands(void){
  return this->sendAT("AT+CBAND?",1000);
}

//EGSM_MODE,ALL_BAND
String SALGSMv1::set_band(String band){
  char c_payload[100];
  snprintf(c_payload, sizeof(c_payload),"AT+CBAND=%s", band);
  return this->sendAT(c_payload);
}

bool SALGSMv1::reset(void){
  do{
    this->STATUS=true;
    this->sendAT("AT+CFUN=1,1",10000);
  } while(this->STATUS==false);
  return this->STATUS;
}

bool SALGSMv1::debug(){
  return this->DEBUG;
}

bool SALGSMv1::con_to_internet(void){
  
  do {
    this->STATUS=true;
    this->sendAT("AT+CGATT=1",3000);  //turn on internet
  } while(this->STATUS==false);
  
  do {
    this->STATUS=true;
    this->sendAT("AT+CSTT=\"internet\"", 1500);
  } while(this->STATUS==false);

  do {
    this->STATUS=true;
    this->sendAT("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
  } while(this->STATUS==false);

  do {
    this->STATUS=true;
    char c_payload[100];
    snprintf(c_payload, sizeof(c_payload),"AT+SAPBR=3,1,\"APN\",\"%s\"", this->my_APN.c_str());
    this->sendAT(c_payload,1000);
  } while(this->STATUS==false);
  
  do {
    this->STATUS=true;
    this->sendAT("AT+SAPBR=1,1");
  } while(this->STATUS==false);

  do {
    this->STATUS=true;
    this->sendAT("AT+SAPBR=2,1");
  } while(this->STATUS==false);

  return this->STATUS;
}

void SALGSMv1::location_area_code(void){
  String lacHex = "";
  String cellHex = "";
  String pom = "";

  this->sendAT("AT+CREG=2");
  pom = this->sendAT("AT+CREG?"); //+CREG: 2,5,"E182","AD91"

  int firstQuote = pom.indexOf('"');           // pozycja 1. cudzysłowu
  int secondQuote = pom.indexOf('"', firstQuote + 1);
  int thirdQuote = pom.indexOf('"', secondQuote + 1);
  int fourthQuote = pom.indexOf('"', thirdQuote + 1);

  lacHex = pom.substring(firstQuote + 1, secondQuote);   // E182
  cellHex = pom.substring(thirdQuote + 1, fourthQuote);   // AD91

  this->lacDec = strtol(lacHex.c_str(), NULL, 16);   // 57730
  this->cellDec = strtol(cellHex.c_str(), NULL, 16); // 44433

  pom = this->sendAT("AT+COPS?"); //+COPS: 0,0,"Era"
  firstQuote = pom.indexOf('"');           // pozycja 1. cudzysłowu
  secondQuote = pom.indexOf('"', firstQuote + 1);
  this->network_operator = pom.substring(firstQuote + 1, secondQuote);   // Era

  if(DEBUG){
    Serial.println(lacHex);
    Serial.println(cellHex);
    Serial.println(this->network_operator);
    Serial.println(this->lacDec);
    Serial.println(this->cellDec);
  }

}



void SALGSMv1::http_get_(const char* cmd, int wait){
  //url += "&lacDec="+String(this->lacDec)+"&cellDec="+String(this->cellDec)+"&netop="+this->network_operator;
  char payload[20];
  
    
  //snprintf(payload, sizeof(payload), "AT+HTTPINIT");
  do {
    this->STATUS=true;
    this->sendAT("AT+HTTPINIT",1000);
    //this->sendAT(payload);
  } while(this->STATUS==false);

  //snprintf(payload, sizeof(payload), "AT+HTTPPARA=\"CID\",1");
  do {
    this->STATUS=true;
    this->sendAT("AT+HTTPPARA=\"CID\",1",1000);
    //this->sendAT(payload,4000);
  } while(this->STATUS==false);

  do {
    //char url[256];
    //snprintf(url, sizeof(url), "AT+HTTPPARA=\"URL\",\"http://dlb.com.pl/api/v1/telemetry.php?ID=%s&KEY=%s&payload=%s\"",ID, KEY, payload);
    this->STATUS=true;
    //this->sendAT("AT+HTTPPARA=\"URL\",\"http://dlb.com.pl/api/v1/telemetry.php?ID=901405180011350&KEY=999&payload=xXx\"",2000);
    this->sendAT(cmd,wait);
  } while(this->STATUS==false);

  delay(2000);

  //snprintf(payload, sizeof(payload), "AT+HTTPACTION=0");
  do {
    this->STATUS=true;
    this->sendAT("AT+HTTPACTION=0", 1000);
  } while(this->STATUS==false);

  //nalezy poskladac do kupy odpowiedzi z tych dwoch zapytan
  // this->BUFF = "";
  // this->sendAT("AT+HTTPREAD=0,33",(wait));
  // if (this->my_lengthToRead > 33) this->sendAT("AT+HTTPREAD=33,66",(3*wait));
  // if (this->my_lengthToRead > 66) this->sendAT("AT+HTTPREAD=66,99",(3*wait));

  this->sendAT("AT+HTTPTERM",wait);

  return this->BUFF;
}

String SALGSMv1::get_ip(void){
  return this->sendAT("AT+SAPBR=2,1");
}



void SALGSMv1::networkDiagnosis() {
  Serial.println("\n=== NETWORK DIAGNOSIS ===");
  
  // 1. Sygnał
  Serial.print("1. Signal (CSQ): ");
  String csq = this->sendAT("AT+CSQ", 2000);
  Serial.println(csq);
  
  // 2. Rejestracja w sieci
  Serial.print("2. Network reg (CREG): ");
  Serial.println(this->sendAT("AT+CREG?", 2000));
  
  // 3. Operator
  Serial.print("3. Operator (COPS): ");
  Serial.println(this->sendAT("AT+COPS?", 2000));
  
  // 4. GPRS status
  Serial.print("4. GPRS attach (CGATT): ");
  Serial.println(this->sendAT("AT+CGATT?", 2000));
  
  // 5. Sprawdź IP
  Serial.print("5. IP address (CIFSR): ");
  Serial.println(this->sendAT("AT+SAPBR=2,1",2000));
  
  Serial.println("=== END DIAGNOSIS ===\n");
}


// String SALGSMv1::removeATPrefix(String text) {
//   // Usuwa pierwsze wystąpienia "AT" z tekstu
//   text.remove(text.indexOf("AT"), 3); // usuwa 2 znaki: "A" i "T"

//   if(text.indexOf("=")>-1){
//     text.remove(text.indexOf("="), (text.length()-text.indexOf("=")));
//   }
//   return text;
// }

// const char* SALGSMv1::removeATPrefix(const char* text)
// {
//     static char buf[128];   // bufor wynikowy
//     int j = 0;

//     // Kopiujemy wejście do bufora
//     for (int i = 0; text[i] != '\0' && j < sizeof(buf)-1; i++) {
//         buf[j++] = text[i];
//     }
//     buf[j] = '\0';

//     // Usuń pierwsze wystąpienie "AT"
//     char* at_ptr = strstr(buf, "AT");
//     if (at_ptr) {
//         // przesuwamy resztę znaków w lewo, nadpisując "AT"
//         memmove(at_ptr, at_ptr + 2, strlen(at_ptr + 2) + 1);
//     }

//     // Jeśli występuje "=", usuń wszystko od "=" do końca
//     char* eq_ptr = strchr(buf, '=');
//     if (eq_ptr) {
//         *eq_ptr = '\0';
//     }

//     return buf;  // zwracamy wskaźnik do statycznego bufora
// }


bool SALGSMv1::waitForNetwork() {
  String pom;
  for (int i = 0; i < 300; i++) { // do 30 prób (ok. 30s)
    pom = this->sendAT("AT+CREG?");
    delay(1000);
    if (pom.indexOf("+CREG: 0,1")>-1 || pom.indexOf("+CREG: 0,5")>-1 ) {
      Serial.println("Zarejestrowano w sieci!");
      return true;
    }
    Serial.print(".");
  }
  Serial.println("\nBrak rejestracji w sieci!");
  return false;
}

void SALGSMv1::setDEBUG(bool state){
  this->DEBUG = state;
}


// Funkcja wysyłania prostego HTTP POST przez TCP socket
void SALGSMv1::sendHTTPPOST(const char* host, int port, const char* path, const char* data) {
  char cmd[200];

  // Utworzenie socketu TCP
  sendAT("AT+CSOC=1,1,1"); // socket TCP IPv4

  // Połącz z hostem
  sprintf(cmd, "AT+CSOCON=0,%d,\"%s\"", port, host);
  sendAT(cmd);

  delay(2000); // poczekaj na połączenie

  // Wyślij dane
  int len = strlen(data);
  sprintf(cmd, "AT+CSOSEND=0,%d,%s", len, data);
  sendAT(cmd);

  // Zamknij socket
  sendAT("AT+CSOCL=0");
}

// String SALGSMv1::clean(String s) {
//   if (s.startsWith("\xEF\xBB\xBF")) {
//     return s.substring(3);
//   }
//   return s;
// }

String SALGSMv1::extractHttpData(String raw) {
  int start = raw.indexOf("+HTTPREAD:");
  if (start == -1) return ""; // brak +HTTPREAD

  // wyciągamy liczbę znaków po ":"
  int colon = raw.indexOf(":", start);
  int newline = raw.indexOf("\n", colon);
  if (colon == -1 || newline == -1) return "";

  String lenStr = raw.substring(colon + 1, newline);
  lenStr.trim();
  int lengthToRead = lenStr.toInt(); // np. 41

  // wyciągamy dokładnie tyle znaków po znaku nowej linii
  int dataStart = newline + 1;
  if (dataStart + lengthToRead > raw.length()) {
    lengthToRead = raw.length() - dataStart; // jeśli mniej danych niż zadeklarowane
  }

  String data = raw.substring(dataStart, dataStart + lengthToRead);

  // opcjonalnie usuń BOM
  if (data.startsWith("\xEF\xBB\xBF")) {
    data = data.substring(3);
  }

  return data;
}

void SALGSMv1::sendSMS(String number, String msg) {

  sendAT("AT+CMGF=1");  // tryb tekstowy
  delay(500);

  sendAT("AT+CSCS=\"GSM\"");
  delay(500);

  sendAT("AT+CMGS=\"+48609105069\"");
  delay(500);

  my_serial->println("TEST SMS z modulu GSMSALv1");
  delay(200);

  my_serial->write(26); // CTRL+Z
  my_serial->write(26); // CTRL+Z

}

// String SALGSMv1::extractIMSI(String resp) {
//   resp.replace("\r", "");
//   resp.replace("\n", "");
//   resp.replace(" ", "");

//   // usuń "AT+CIMI" jeśli występuje
//   resp.replace("AT+CIMI", "");
//   resp.replace("OK", "");

//   // zostaje tylko cyfrowy IMSI
//   return resp;
// }

  // Dodatkowe funkcje pomocnicze
bool SALGSMv1::isModuleAlive(void) {
  String response = this->sendAT("AT", 2000);
  return response.indexOf("OK") != -1;
}

int SALGSMv1::parseHTTPAction(String response) {
  // Szukaj pattern: +HTTPACTION: 0,200,1234
  int start = response.indexOf("+HTTPACTION:");
  if (start == -1) return -1;
  
  int comma1 = response.indexOf(',', start);
  int comma2 = response.indexOf(',', comma1 + 1);
  
  if (comma1 == -1 || comma2 == -1) return -1;
  
  return response.substring(comma1 + 1, comma2).toInt();
}

int SALGSMv1::getHTTPContentLength() {
  // Możesz dodać parsowanie długości z odpowiedzi AT+HTTPACTION
  // Tymczasowo zwróć domyślną wartość
  return 100; // lub odczytaj z odpowiedzi modułu
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