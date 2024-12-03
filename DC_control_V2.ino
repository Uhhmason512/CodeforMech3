/*
  Arduino Starter Kit example
  Project 10 - Zoetrope

  This sketch is written to accompany Project 10 in the Arduino Starter Kit

  Parts required:
  - two 10 kilohm resistors
  - two momentary pushbuttons
  - one 10 kilohm potentiometer
  - motor
  - 9V battery
  - H-Bridge

  created 13 Sep 2012
  by Scott Fitzgerald
  Thanks to Federico Vanzati for improvements

  https://store.arduino.cc/genuino-starter-kit

  This example code is part of the public domain.
*/

#include <util/atomic.h> // For the ATOMIC_BLOCK macro
#include <Arduino.h>

#define ENCA 2 // pin 50 of the Arduino
#define ENCB 3 // pin 52 of the Arduino

const int controlPin1 = 42;                // connected to pin 7 on the H-bridge
const int controlPin2 = 4;                // connected to pin 2 on the H-bridge
const int enablePin = 9;                  // connected to pin 1 on the H-bridge
const int directionSwitchPin = 44;         // connected to the switch for direction
const int onOffSwitchStateSwitchPin = 45;  // connected to the switch for turning the motor on and off
const int potPin = A0;                    // connected to the potentiometer's output
volatile int posi = 0; // specify posi as volatile

// create some variables to hold values from your inputs
int onOffSwitchState = 0;              // current state of the on/off switch
int previousOnOffSwitchState = 0;      // previous position of the on/off switch
int directionSwitchState = 0;          // current state of the direction switch
int previousDirectionSwitchState = 0;  // previous state of the direction switch

int motorEnabled = 0;    // Turns the motor on/off
int motorSpeed = 0;      // speed of the motor
int motorDirection = 0;  // current direction of the motor

volatile int prev_pos;
unsigned long new_time;
unsigned long prev_time = 0;
unsigned long Delta_time;

float RPM;

void setup() {
  // initialize the inputs and outputs
  Serial.begin(9600);
  pinMode(directionSwitchPin, INPUT);
  pinMode(onOffSwitchStateSwitchPin, INPUT);
  pinMode(controlPin1, OUTPUT);
  pinMode(controlPin2, OUTPUT);
  pinMode(enablePin, OUTPUT);
  pinMode(ENCA,INPUT); // sets the Encoder_output_A pin as the input
  pinMode(ENCB,INPUT); // sets the Encoder_output_B pin as the input
  attachInterrupt(digitalPinToInterrupt(ENCA),readEncoder,RISING);

  // pull the enable pin LOW to start
  digitalWrite(enablePin, LOW);

}

void loop() {
  // read the value of the on/off switch
  onOffSwitchState = digitalRead(onOffSwitchStateSwitchPin);
  delay(10);


  // read the value of the direction switch
  directionSwitchState = digitalRead(directionSwitchPin);

  // read the value of the pot and divide by 4 to get a value that can be
  // used for PWM
  motorSpeed = analogRead(potPin) / 8;


  int true_pos = 0; 
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    true_pos = abs(posi % 100);
  }

  new_time = millis();

  

  if (new_time - prev_time >= 60000 && count < 60){

    Serial.print("Time difference: ");
    Serial.println(new_time - prev_time);
    
    Serial.print("posi at Delta_T (1s): ");
    Serial.println(posi);

    Serial.print("prev time: ");
    Serial.print(prev_time);

    prev_time = new_time;
    count++;

    Serial.print("  new time: ");
    Serial.println(new_time);

    float RPM = (posi * 60.0/(1000.0));

    Serial.print("RPM: ");
    Serial.println(RPM);

    posi = 0;

  }

 
  // if the on/off button changed state since the last loop()
  if (onOffSwitchState != previousOnOffSwitchState) {
    // change the value of motorEnabled if pressed
    if (onOffSwitchState == HIGH) {
      motorEnabled = !motorEnabled;
    }
  }

  // if the direction button changed state since the last loop()
  if (directionSwitchState != previousDirectionSwitchState) {
    // change the value of motorDirection if pressed
    if (directionSwitchState == HIGH) {
      motorDirection = !motorDirection;
    }
  }

  // change the direction the motor spins by talking to the control pins
  // on the H-Bridge
  if (motorDirection == 1) {
    digitalWrite(controlPin1, HIGH);
    digitalWrite(controlPin2, LOW);
  } else {
    digitalWrite(controlPin1, LOW);
    digitalWrite(controlPin2, HIGH);
  }

  // if the motor is supposed to be on
  if (motorEnabled == 1) {
    // PWM the enable pin to vary the speed
    analogWrite(enablePin, motorSpeed);
  } else {  // if the motor is not supposed to be on
    //turn the motor off
    analogWrite(enablePin, 0);
  }
  // save the current on/off switch state as the previous
  previousDirectionSwitchState = directionSwitchState;
  // save the current switch state as the previous
  previousOnOffSwitchState = onOffSwitchState;
}

void readEncoder(){
  int b = digitalRead(ENCB);  // Read the encoder signal

  // Check if the encoder signal is HIGH and debounce time has passed
  if (b > 0) {
    posi++;  // Increment position if the signal is HIGH
  }
}