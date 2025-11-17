#include "SALGSMv1.h"

String SALGSMv1::init(void){
  this->reset();
  return "=== Inicjalizacja SALGSMv1 ===";
}

String SALGSMv1::IMSI(void){
  return this->sendAT("AT+CIMI",200);
}

String SALGSMv1::networks(void){
  return this->sendAT("AT+COPS=?",10000);
}

String SALGSMv1::bands(void){
  return this->sendAT("AT+CBAND?",1000);
}

//EGSM_MODE,ALL_BAND
String SALGSMv1::set_band(String band){
  return this->sendAT("AT+CBAND="+band);
}

String SALGSMv1::reset(void){
  STATUS = "OK"; 
  return this->sendAT("AT+CFUN=1,1",8000);
}

bool SALGSMv1::con_to_internet(void){
    this->sendAT("AT+CGATT=1");  //turn on internet
    this->sendAT("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
    this->sendAT("AT+SAPBR=3,1,\"APN\",\""+this->my_APN+"\"");
    this->sendAT("AT+SAPBR=1,1");
    this->sendAT("AT+SAPBR=2,1");
  return true;
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

String SALGSMv1::http_get_(String url, int wait){
  url += "&lacDec="+String(this->lacDec)+"&cellDec="+String(this->cellDec)+"&netop="+this->network_operator+"&end=END";
  this->sendAT("AT+HTTPINIT",wait);
  this->sendAT("AT+HTTPPARA=\"CID\",1",wait);
  this->sendAT("AT+HTTPPARA=\"URL\",\""+url+"\"",wait);

  this->sendAT("AT+HTTPACTION=0",(3*wait));

  //nalezy poskladac do kupy odpowiedzi z tych dwoch zapytan
  this->BUFF = "";
  this->sendAT("AT+HTTPREAD=0,33",(3*wait));
  if (this->my_lengthToRead > 33) this->sendAT("AT+HTTPREAD=33,66",(3*wait));
  if (this->my_lengthToRead > 66) this->sendAT("AT+HTTPREAD=66,99",(3*wait));

  this->sendAT("AT+HTTPTERM",wait);

  if(this->STATUS == "OK")
    return this->BUFF;
  else
    return this->STATUS;
}

String SALGSMv1::get_ip(void){
  return this->sendAT("AT+SAPBR=2,1");
}

String SALGSMv1::sendAT(String cmd, int wait) {
  String wynik = "";
  char pom;
  uint8_t n_pom=0;
  while((!(wynik.indexOf(removeATPrefix(cmd))>-1)) or (wynik.indexOf("ERROR")>-1) and (this->STATUS == "OK")){ //zabezpieczyc przed zapetleniem
    my_serial.println(cmd);

    if(DEBUG){
      Serial.print(">> ");
      Serial.print(cmd);
      Serial.print(" >>> ");
      Serial.print(removeATPrefix(cmd));
      Serial.print(" >>>> "); 
    }
    wynik = "";
    unsigned long t0 = millis();
    while (millis() - t0 < wait) {
      if (my_serial.available()) {
        pom = my_serial.read();
        //Serial.write(pom);
        delay(1); //??????
        wynik += pom; 
      }
    }
    n_pom++;
    wynik = clean(wynik);
    if(DEBUG) {
      Serial.print(n_pom);
      Serial.print("razy >> ");
      Serial.println(wynik);
    }
    if (n_pom>5) this->STATUS = "AT_ERROR";
  }
  

  if(wynik.indexOf(removeATPrefix("+HTTPACTION"))>-1){
    String raw = "+HTTPACTION: 0,200,41";
    int lastComma = wynik.lastIndexOf(',');   // szukamy ostatniego przecinka
    String lengthStr = wynik.substring(lastComma + 1); // wszystko po przecinku
    lengthStr.trim(); // usuwamy ewentualne spacje
    this->my_lengthToRead = lengthStr.toInt();
  }

  if(wynik.indexOf(removeATPrefix("+HTTPREAD"))>-1){
    //Serial.println(extractHttpData(wynik));
    this->BUFF += extractHttpData(wynik);
  }

  return wynik;
}

String SALGSMv1::removeATPrefix(String text) {
  // Usuwa pierwsze wystąpienia "AT" z tekstu
  text.remove(text.indexOf("AT"), 3); // usuwa 2 znaki: "A" i "T"

  if(text.indexOf("=")>-1){
    text.remove(text.indexOf("="), (text.length()-text.indexOf("=")));
  }
  return text;
}


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

void SALGSMv1::httpGetGoogle() {
  if (!waitForNetwork()) {
    Serial.println("Błąd: brak sieci – ponawiam próbę za 10s...");
    delay(10000);
    waitForNetwork();
  }

  sendAT("AT+CHTTPCREATE=\"google.com\"");
  delay(1000);
  sendAT("AT+CHTTPCON=0"); // połącz z sesją 0
  delay(1000);

  // Wysłanie zapytania HTTP GET
  sendAT("AT+CHTTPSEND=0,\"GET / HTTP/1.1\\r\\nHost: google.com\\r\\nConnection: close\\r\\n\\r\\n\"");
  delay(4000);

  // Odczyt odpowiedzi
  Serial.println("Odpowiedź HTTP:");
  sendAT("AT+CHTTPRECV=0,512");
  delay(1000);

  // Zamknięcie sesji
  sendAT("AT+CHTTPDISCON=0");
  sendAT("AT+CHTTPDESTROY=0");
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

String SALGSMv1::clean(String s) {
  if (s.startsWith("\xEF\xBB\xBF")) {
    return s.substring(3);
  }
  return s;
}

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

  my_serial.println("TEST SMS z modulu GSMSALv1");
  delay(200);

  my_serial.write(26); // CTRL+Z
  my_serial.write(26); // CTRL+Z


  // sendAT("AT");
  // sendAT("AT+CMGF=1");             // Tryb tekstowy
  // sendAT("AT+CSCS=\"GSM\"");       // Kodowanie

  // my_serial.print("AT+CMGS=\"");
  // my_serial.print(number);
  // my_serial.println("\"");
  // delay(500);

  // my_serial.print(msg);
  // delay(200);

  // my_serial.write(26); // CTRL+Z
  // delay(3000);


}