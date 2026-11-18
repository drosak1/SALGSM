#ifndef SALGSMV1_H
#define SALGSMV1_H

#include <Arduino.h>
#include <avr/wdt.h>

#define DEBUGG 1  // 1 = debug włączony, 0 = wyłączony

#if DEBUGG == 1
  #define DEBUG_PRINT(x)   do { if (DEBUG) debug_serial->print(x); } while(0)
  #define DEBUG_PRINTLN(x) do { if (DEBUG) debug_serial->println(x); } while(0)
#else
  #define DEBUG_PRINT(x) do {} while (0)
  #define DEBUG_PRINTLN(x) do {} while (0)
#endif

class SALGSMv1 {
public:
  SALGSMv1(Stream &serial, Stream &debugSerial, const char *apn, const char *apikey, const char *serverurl, bool debug, uint8_t GSMenable_pin_cpu, uint8_t GSMreset_pin_cpu);

  bool switch_on();
  bool switch_off();
  bool send(const char* key, const char* value);
  bool send(const char* key, double value);

  void reset_cpu_();
  bool isModuleAlive();
  void setDEBUG(bool state);

  char my_IMSI[20]        = {0};
  char my_APN[20]        = {0};
  char my_APIKEY[10]      = {0};
  char my_SERVERURL[20]   = {0};
  char IP[20]             = {0};

  uint8_t GSM_RESET; //  reset gdy stan niski
  uint8_t GSM_ENABLE; // aktywny stan wysoki

private:
  void IMSI();
  void sendAT(const char *cmd, char *out, size_t outSize, unsigned long timeout = 2000);
  void clear(char *buf, size_t size);
  bool con_to_internet();
  bool http_get_(const char *cmd);
  const char *extractID(const char *resp);

  bool DEBUG = false;
  bool STATUS = false;
  Stream *my_serial = nullptr;
  Stream *debug_serial = nullptr;
  //static constexpr char my_err_1_[] = "!!!!! STATUS = false !!!!";
};

#endif
