#include "SALGSMv1.h"

SALGSMv1 GSM_dev(Serial1,"sensor.net", false); 

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);

  Serial.println(GSM_dev.init());

  GSM_dev.IMSI();
  //GSM_dev.set_band("ALL_BAND");

  GSM_dev.con_to_internet();

  //GSM_dev.get_ip();
  GSM_dev.location_area_code();
}

void loop() {
  // nic
  if(GSM_dev.STATUS!="OK") {
    Serial.println("RESET GSM");
    GSM_dev.reset();
    GSM_dev.con_to_internet();
  }

  Serial.println(GSM_dev.http_get_("http://dlb.sytes.net/api.php?name=demo&command=TIME",2000));

  if (Serial1.available()) Serial.write(Serial1.read());
  if (Serial.available()) Serial1.write(Serial.read());
}