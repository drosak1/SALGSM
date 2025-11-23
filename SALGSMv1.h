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
  SALGSMv1(String APN, bool debug);
  void set_serial(Stream* s);
  String init(void);
  const char* IMSI(void);
  String networks(void);
  String bands(void);
  String CSQ(void);
  String set_band(String band);
  bool reset(void);
  bool con_to_internet(void);
  String get_ip(void);
  bool waitForNetwork(void);
  void http_get_(const char* cmd, int wait = 1000);
  void httpGetGoogle(void);
  void sendHTTPPOST(const char* host, int port, const char* path, const char* data);
  void location_area_code(void);
  void sendSMS(String number, String msg);
  void setDEBUG(bool state);
  void networkDiagnosis();
  bool debug();
  bool STATUS = true; //"AT_ERROR", 
  String sendAT(const char* cmd, int wait = 3000);
  const char* extractID(const char* resp);

private:
  bool DEBUG = false;
  Stream* my_serial;
  String my_APN = "";
  const char* BUFF = "";
  uint16_t my_lengthToRead = 0;
  char my_IMSI [20] = "";

  //location_area_code
  long lacDec = 0;
  long cellDec = 0;
  String network_operator = "";

  //const char*  removeATPrefix(const char*  text);
  String extractHttpData(String raw);
  //String extractIMSI(String resp);
  int getHTTPContentLength();
  int parseHTTPAction(String response);
  bool isModuleAlive();
};

#endif