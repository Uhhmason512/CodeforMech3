#include <ezButton.h>

#include <Servo.h>

Servo myservo;

ezButton limitSwitch(48);  // create ezButton object that attach to pin 7;

const int stepPin = 12; 
const int dirPin = 53; 
const int enPin = 52;


void setup() {
  Serial.begin(9600);
  myservo.attach(49);  // attaches the servo on pin 49 to the servo object
  myservo.write(115);
  limitSwitch.setDebounceTime(50); // set debounce time to 50 milliseconds

  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  pinMode(enPin,OUTPUT);
  digitalWrite(enPin,LOW);

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
      digitalWrite(dirPin,LOW);
      break;
    }
    delay(100);
  }

}

void loop() {
  myservo.write(0); // position over Vessel
  Serial.println("Servo position 0");
  delay(1000);

  for (int y = 0; y < 3; y++) { // Motor moves down
      for(int x = 0; x < 800; x++) { //Start a loop for an eighth of a revolution
        digitalWrite(stepPin,HIGH); 
        delayMicroseconds(500);
        digitalWrite(stepPin,LOW); 
        delayMicroseconds(500);
      }
      delay(100);
    }

  delay(5000);
  digitalWrite(dirPin,HIGH);
  for (int y = 0; y < 3; y++) { // Motor moves up
    for(int x = 0; x < 800; x++) { //Start a loop for an eighth of a revolution
      digitalWrite(stepPin,HIGH); 
      delayMicroseconds(500);
      digitalWrite(stepPin,LOW); 
      delayMicroseconds(500);
    }
    delay(500);
  }

  digitalWrite(dirPin,LOW);
  myservo.write(170); // Prime servo position for cleaning
  Serial.println("Servo position 170");
  delay(1000);

  for (int y = 0; y < 7; y++) { // Motor moves down
    for(int x = 0; x < 800; x++) { //Start a loop for an eighth of a revolution
      digitalWrite(stepPin,HIGH); 
      delayMicroseconds(500);
      digitalWrite(stepPin,LOW); 
      delayMicroseconds(500);
    }
    delay(100);
    Serial.println(y);
  }
  Serial.println("At the bottom");

  for (int y=0; y<6; y++) { // Cleaning loop
      myservo.write(180);              // tell servo to go to position in variable 'pos'
      delay(300); 
      myservo.write(150);              // tell servo to go to position in variable 'pos'
      delay(300);      
    }
  digitalWrite(dirPin,HIGH);
  for (int y = 0; y < 2; y++) { // Motor moves up
    for(int x = 0; x < 800; x++) { //Start a loop for an eighth of a revolution
      digitalWrite(stepPin,HIGH); 
      delayMicroseconds(500);
      digitalWrite(stepPin,LOW); 
      delayMicroseconds(500);
    }
    delay(500);
  }

  myservo.write(115); // Set position for storage

  digitalWrite(dirPin,LOW);
  delay(500);
  for (int y = 0; y < 2; y++) { // Motor moves down
    for(int x = 0; x < 800; x++) { //Start a loop for an eighth of a revolution
      digitalWrite(stepPin,HIGH); 
      delayMicroseconds(500);
      digitalWrite(stepPin,LOW);
      delayMicroseconds(500);
    }
    delay(100);
  }

  while (true){
    Serial.println("All clean");
    delay(10000);
  }

}
