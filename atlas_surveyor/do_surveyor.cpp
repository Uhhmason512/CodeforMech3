
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "do_surveyor.h"
#include "EEPROM.h"


Surveyor_DO::Surveyor_DO(uint8_t pin){
	this->pin = pin;
    this->EEPROM_offset = (pin) * EEPROM_SIZE_CONST;
    //to lay the calibration parameters out in EEPROM we map their locations to the analog pin numbers
    //we assume a maximum size of EEPROM_SIZE_CONST for every struct we're saving  
}

bool Surveyor_DO::begin(){
    #if defined(ESP8266) || defined(ESP32)
        EEPROM.begin(1024);
    #endif 
	if((EEPROM.read(this->EEPROM_offset) == magic_char)
    && (EEPROM.read(this->EEPROM_offset + sizeof(uint8_t)) == SURVEYOR_DO)){
		EEPROM.get(this->EEPROM_offset,Do);
		return true;
    }
	return false;
}

float Surveyor_DO::read_voltage() {
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

float Surveyor_DO::read_do_percentage(float voltage_mV) {
    return voltage_mV * 100.0 / this->Do.full_sat_voltage;
}

void Surveyor_DO::cal() {
    this->Do.full_sat_voltage = read_voltage();
    EEPROM.put(this->EEPROM_offset,Do);
    #if defined(ESP8266) || defined(ESP32)
        EEPROM.commit(); 
    #endif
}

void Surveyor_DO::cal_clear() {
    this->Do.full_sat_voltage = DEFAULT_SAT_VOLTAGE;
    EEPROM.put(this->EEPROM_offset,Do);
    #if defined(ESP8266) || defined(ESP32)
        EEPROM.commit(); 
    #endif
}

float Surveyor_DO::read_do_percentage() {
  return(read_do_percentage(read_voltage()));
}
