#ifndef SALGSMv1_H
#define SALGSMv1_H

#include <Arduino.h>

/*
  URUCHOMIENIE MODULU sygnalizowane nast komendami
  +CPIN: READY
  Call Ready
  SMS Ready
*/

class SALGSMv1 {
public:
  SALGSMv1(Stream& s, String APN, bool debug) : my_serial(s) {this->my_APN = APN; this->DEBUG = debug;}
  String init(void);
  String IMSI(void);
  String networks(void);
  String bands(void);
  String set_band(String band);
  String reset(void);
  bool con_to_internet(void);
  String get_ip(void);
  bool waitForNetwork(void);
  String http_get_(String url, int wait = 1000);
  void httpGetGoogle(void);
  void sendHTTPPOST(const char* host, int port, const char* path, const char* data);
  void location_area_code(void);
  String STATUS = "OK"; //"AT_ERROR", 

private:
  bool DEBUG = false;
  Stream& my_serial;
  String my_APN = "";
  String BUFF = "";
  uint16_t my_lengthToRead = 0;

  //location_area_code
  long lacDec = 0;
  long cellDec = 0;
  String network_operator = "";

  String sendAT(String cmd, int wait = 3000);
  String clean(String s);
  String removeATPrefix(String text);
  String extractHttpData(String raw);
};

#endif