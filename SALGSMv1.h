#ifndef SALGSMV1_H
#define SALGSMV1_H

#include <Arduino.h>

class SALGSMv1 {
public:
  SALGSMv1(Stream &serial, Stream &debugSerial, const char *apn, bool debug);

  void sendAT(const char *cmd,
              char *out,
              size_t outSize,
              unsigned long timeout = 2000);
  void clear(char *buf, size_t size);
  bool init();
  void IMSI();
  bool con_to_internet();
  bool http_get_(const char *cmd);
  const char *extractID(const char *resp);
  void reset_();
  bool isModuleAlive();
  void setDEBUG(bool state);

  char my_IMSI[20] = {0};
  char my_APN[100] = {0};
  char IP[20] = {0};

private:
  bool DEBUG = false;
  bool STATUS = false;
  Stream *my_serial = nullptr;
  Stream *debug_serial = nullptr;
};

#endif
