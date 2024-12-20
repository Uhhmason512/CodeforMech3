
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "orp_surveyor.h"
#include "EEPROM.h"  
    
Surveyor_ORP::Surveyor_ORP(uint8_t pin){
	this->pin = pin;
    this->EEPROM_offset = (pin) * EEPROM_SIZE_CONST;
    //to lay the calibration parameters out in EEPROM we map their locations to the analog pin numbers
    //we assume a maximum size of EEPROM_SIZE_CONST for every struct we're saving

}

bool Surveyor_ORP::begin(){
    #if defined(ESP8266) || defined(ESP32)
        EEPROM.begin(1024);
    #endif     
	if((EEPROM.read(this->EEPROM_offset) == magic_char)
    && (EEPROM.read(this->EEPROM_offset + sizeof(uint8_t)) == SURVEYOR_ORP)){
		EEPROM.get(this->EEPROM_offset,Orp);
		return true;
    }
	return false;
}

float Surveyor_ORP::read_voltage() {
  float voltage_mV = 0;
    for (int i = 0; i < volt_avg_len; ++i) {
	#if defined(ESP32)
	//ESP32 has significant nonlinearity in its ADC, we will attempt to compensate 
	//but you're on your own to some extent
	//this compensation is only for the ESP32
	//https://github.com/espressif/arduino-esp32/issues/92
		voltage_mV += analogRead(this->pin) / 4095.0 * 3300.0 + 130;
	#else
		voltage_mV += analogRead(this->pin) / 1024.0 * 5000.0;
    #endif 
    }
    voltage_mV /= volt_avg_len;
  return voltage_mV;
}

float Surveyor_ORP::read_orp(float voltage_mV) {
    return voltage_mV - 1500.0 - this->Orp.cal_offset; //midpoint
}

void Surveyor_ORP::cal(float value) {
    this->Orp.cal_offset = read_voltage() - (value + 1500);
    EEPROM.put(this->EEPROM_offset,Orp);
    
    #if defined(ESP8266) || defined(ESP32)
        EEPROM.commit(); 
    #endif
}

void Surveyor_ORP::cal_clear() {
    this->Orp.cal_offset = 0;
    EEPROM.put(this->EEPROM_offset,Orp);
    
    #if defined(ESP8266) || defined(ESP32)
        EEPROM.commit(); 
    #endif
}

float Surveyor_ORP::read_orp() {
  return(read_orp(read_voltage()));
}
