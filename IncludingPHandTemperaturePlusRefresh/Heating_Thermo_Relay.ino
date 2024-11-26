// pre setup - LCD
// include the library code:
#include <LiquidCrystal.h>
#include <Wire.h> //What?
// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// pre setup - thermocouple
const int tempPin = A0;
float voltage = 0;      // Variable to store the voltage reading
float temperatureF = 0; // Variable to store the calculated temperature in Fahrenheit
float temperatureC = 0; // Variable to store calculted Celcius

//SSR Relay
const int RELAY1 = 8;//set pin for relay output

void setup() {
  // setup - LCD
  lcd.begin(16, 2); // set up the LCD's number of columns and rows
  lcd.setCursor(0,1);
  lcd.write("Rumen Revoltion");
  delay(1000);

  // setup - thermocouple
  Serial.begin(9600); //Initialize serial communication for debugging

  // setup Relay
  pinMode(RELAY1, OUTPUT);
  digitalWrite(RELAY1, HIGH);
  
  }

void loop() {
  // read thermocouple
  int sensorValue = analogRead(tempPin); // Read the analog input (0-1023)
    voltage = sensorValue * (5.0 / 1023.0); // Convert to voltage (0-5V)

    // Convert voltage to temperature in Fahrenheit
    temperatureF = voltage / 0.01; // 0.01V per °F
    temperatureC = (55.56*voltage)+255.37-273.15; // deg C
    //an if statement for if Celcius or Farenheit setting selected fould be here. For now C
   // basic readout test, just print the current temp
  
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.print(" V, Temperature F: ");
    Serial.print(temperatureF);
    Serial.print(" °F, Temperature C: ");
    Serial.print(temperatureC);
    Serial.println(" °C");

delay(1000); // Wait for a second before the next reading 

   if (isnan(temperatureC)) {
     Serial.println("Something wrong with thermocouple!");
   } else {
     Serial.print("C: ");
     Serial.println(temperatureC);
   }

   // print on LCD
   lcd.clear();
   lcd.setCursor(0,1);
   lcd.write("Temp (C): ");
   lcd.print(temperatureC);


   if(temperatureC >= 36){
    digitalWrite(RELAY1, LOW);
    Serial.println("Heater Off");
   }
   else{
    digitalWrite(RELAY1, HIGH);
   }
   delay(1000);

   

}