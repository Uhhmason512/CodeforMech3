#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "orp_iso_surveyor.h"

float Surveyor_ORP_Isolated::read_voltage() {
   uint32_t pulse_len = 0;
   for (int i = 0; i < volt_avg_len*3; ++i) {
       pulse_len += pulseIn(this->pin, HIGH, 400);
   }
   float avg = (float)pulse_len / (float)(volt_avg_len*3.0);
   if(avg < 0.1){
       if(digitalRead(this->pin) == HIGH){
           avg = 80;
       }else{
           avg = 0;
       }
   }
   return avg*35;
}