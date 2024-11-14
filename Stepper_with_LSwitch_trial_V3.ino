#include <ezButton.h>

ezButton limitSwitch(7);  // create ezButton object that attach to pin 7;

const int stepPin = 5; 
const int dirPin = 2; 
const int enPin = 8;

void setup() {
  Serial.begin(9600);
  limitSwitch.setDebounceTime(50); // set debounce time to 50 milliseconds

  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  pinMode(enPin,OUTPUT);
  digitalWrite(enPin,LOW);
}

void loop() { // put your main code here, to run repeatedly:
  limitSwitch.loop(); // MUST call the loop() function first
  int limit_state = limitSwitch.getState();
  Serial.println(limit_state);
  digitalWrite(dirPin,HIGH); 
  while (true) { // Motor moves until it hits the Limit switch
    for(int x = 0; x < 100; x++) {
      digitalWrite(stepPin,HIGH); 
      delayMicroseconds(500);
      digitalWrite(stepPin,LOW); 
      delayMicroseconds(500);
    }
    limitSwitch.loop(); // MUST call the loop() function first
    int limit_state = limitSwitch.getState();
    Serial.print("In loop 1:");
    Serial.println(limit_state);
    if(limit_state == 1) { //Check is Lswitch is pressed
      Serial.println("The limit switch: UNTOUCHED -> TOUCHED");
      Serial.println("Out of loop 1");
      break;
    }
    delay(100);
  }
  delay(1000);
  Serial.println("Between loops");
  digitalWrite(dirPin,LOW);
  while (true) { // Motor moves until the limit switch is released
    for(int x = 0; x < 100; x++) { //Start a loop for an eighth of a revolution
      digitalWrite(stepPin,HIGH); 
      delayMicroseconds(500);
      digitalWrite(stepPin,LOW); 
      delayMicroseconds(500);
    }
    limitSwitch.loop(); // MUST call the loop() function first
    int limit_state = limitSwitch.getState();
    Serial.print("In loop 2:");
    Serial.println(limit_state);
    if(limit_state == 0) { //Check is Lswitch is released
      Serial.println("The limit switch: TOUCHED -> UNTOUCHED");
      Serial.println("Out of loop 2");
      break;
    }
    delay(100);
  }
}
