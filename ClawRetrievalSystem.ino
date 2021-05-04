// Libraries
#include <NewPing.h>      // include the NewPing library for this program
#include <Servo.h>


// Constants // 
// States
#define CALIBRATE 0
#define GROUNDED 1
#define SENSE 2
#define GRABBED 3
#define RELEASE 4

// Symbolic constants //
#define TRUE 1
#define FALSE 0
// Modes
#define SONAR 0
#define JOYSTICK 1
// Sonar
#define OPEN 0 // open position of servo
#define CLOSED 180 // close position of servo 
#define HEIGHT 25 // - legit height
#define GRAB_DELAY 1000 // length to wait after grabbing an object before attempting to release the object
#define RELEASE_DELAY 1000 // length to wait after releasing an object before attempting to grab another object
#define GRAB_THRESHOLD 9 // distance on the sonar sensor to close the claw
#define RELEASE_THRESHOLD 10 // distance on the sonar sensor to open the claw
#define DISTANCES_SIZE 10 // number of distances to measure to prevent outliers
// Joystick
#define JOYSTICK_DELAY 1000
// Blink
#define BLINK_DELAY 500

// Components //
// Sonar
#define VCC_PIN 12 // 13 - Original        // sonar vcc pin attached to pin 13
#define TRIGGER_PIN 11 // 12 - Original    // sonar trigger pin will be attached to Arduino pin 12
#define ECHO_PIN 10 // 11 - Original       // sonar echo pint will be attached to Arduino pin 11
#define GROUND_PIN 9 // 10 - Original      // sonar ground pin attached to pin 10
#define MAX_DISTANCE 200  // fmaximum distance set to 200 cm
// Joystick
#define GROUND_JOY_PIN A3   // joystick ground pin will connect to Arduino analog pin A3
#define VOUT_JOY_PIN A2     //  joystick +5 V pin will connect to Arduino analog pin A2
#define XJOY_PIN A1         // X axis reading from joystick will go into analog pin A1
// Servo
#define SERVO_PIN 8

// Variables //
int mode = SONAR;
int state = CALIBRATE;
unsigned long target = 0;
int distances[DISTANCES_SIZE] = {0};
int calibrateCount = 0;

// Sonar
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // initialize NewPing

// Servo
Servo myservo;  // create servo object to control a servo

// Blink
int blinkLED = FALSE;
int blinkCount = 0;
unsigned long blinkTarget = 0;

// Functions //
// printArray
void printArray(int* data, int arraySize) {
  Serial.print("[");
  for (int index = 0; index < arraySize - 1; index ++) {
    Serial.print(data[index]);
    Serial.print(", ");
  }
  Serial.print(data[arraySize-1]);
  Serial.println("]");
}

// Push into distances array
int push(int value) {
  int average = value;
  for (int index = 1; index < DISTANCES_SIZE; index ++) {
    average += distances[index];
    distances[index - 1] = distances[index];
  }
  distances[DISTANCES_SIZE - 1] = value;
  // printArray(distances, DISTANCES_SIZE);
  return average / DISTANCES_SIZE;
}

// Sonar sensor
void sonarSensor() {
  unsigned long currentTime = millis();
  int distance = push(sonar.ping_cm());
  /*
  Serial.print("State - ");
  Serial.print(state);
  Serial.print(" | Distance - ");
  Serial.print(distance);
  Serial.print(" | Current Time - ");
  Serial.print(currentTime);
  Serial.print(" | Target - ");
  Serial.println(target);
  */
  if (currentTime >= target) {
    switch (state) {
      case CALIBRATE: 
        calibrateCount++;
        if (calibrateCount >= DISTANCES_SIZE) {
          Serial.println("Calibration complete");
          if (distance >= HEIGHT) {
            state = SENSE; 
          } else {
            state = GROUNDED;
          }
          for (int index = 0; index < 6; index ++) {  
            if (index % 2 == 0) {
              digitalWrite(LED_BUILTIN, HIGH);
            } else {
              digitalWrite(LED_BUILTIN, LOW);
            }
            delay(BLINK_DELAY / 5);
          }
        }
        break;
      case GROUNDED:
        blinkLED = FALSE;
        if (distance >= HEIGHT) {
          Serial.println("Sensing ...");
          state = SENSE;
        }
        break;
      case SENSE:
        blinkLED = TRUE;
        if (distance <= GRAB_THRESHOLD) {
          Serial.println("Grabbing object");
          blinkLED = FALSE;
          digitalWrite(LED_BUILTIN, LOW);
          state = GRABBED;
          delay(500);
          myservo.write(CLOSED);
          target = currentTime + GRAB_DELAY;
        }
        break;
      case GRABBED: 
        blinkLED = FALSE;
        if (distance >= HEIGHT) {
          Serial.println("Object lifted");
          digitalWrite(LED_BUILTIN, HIGH);
          state = RELEASE;
        }
        break;
      case RELEASE: 
        blinkLED = FALSE;
        if (distance <= RELEASE_THRESHOLD) {
          Serial.println("Releasing object");
          digitalWrite(LED_BUILTIN, LOW);
          state = GROUNDED;
          myservo.write(OPEN);
          target = currentTime + RELEASE_DELAY;
        }
        break;
    }
  }
}

// Joystick
void joystick() {
  unsigned long currentTime = millis();
  if (currentTime >= target) {
    int joystickXVal = analogRead(XJOY_PIN) ;           //read joystick input on pin A1.  Will return a value between 0 and 1023.
    if (joystickXVal > 1000) {
      Serial.println("Closing claw");
      digitalWrite(LED_BUILTIN, LOW);
      myservo.write(CLOSED);
      target = currentTime + JOYSTICK_DELAY;
    } else if (joystickXVal < 100) {
      Serial.println("Opening claw");
      digitalWrite(LED_BUILTIN, HIGH);
      myservo.write(OPEN);
      target = currentTime + JOYSTICK_DELAY;
    }
    // int servoVal = map(joystickXVal, 0, 1023, 0, 180) ; //changes the value to a raneg of 0 to 180.   See "map" function for further details.
    // myservo.write(servoVal); 
  }
}

// Blink
void blink() {
  unsigned long currentTime = millis();
  if (currentTime >= blinkTarget) {
    blinkCount ++;
    if (blinkCount % 2 == 0) {
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      digitalWrite(LED_BUILTIN, LOW);
    }
    blinkTarget += BLINK_DELAY;
  }
}
// Setup
void setup() {
  Serial.begin(9600);            // set data transmission rate to communicate with computer
  Serial.print("Initializing mode - ");
  // LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  switch(mode) {
    case SONAR:
      Serial.println("Sonar");
      // Sonar
      pinMode(ECHO_PIN, INPUT) ;  
      pinMode(TRIGGER_PIN, OUTPUT) ;
      pinMode(GROUND_PIN, OUTPUT);    // tell pin 10 it is going to be an output
      pinMode(VCC_PIN, OUTPUT);       // tell pin 13 it is going to be an output
      digitalWrite(GROUND_PIN,LOW);   // tell pin 10 to output LOW (OV, or ground)
      digitalWrite(VCC_PIN, HIGH) ;   // tell pin 13 to output HIGH (+5V)
      break;
    case JOYSTICK:
      Serial.println("Joystick");
      // Joystick 
      pinMode(VOUT_JOY_PIN, OUTPUT);      //pin A3 shall be used as output
      pinMode(GROUND_JOY_PIN, OUTPUT) ;   //pin A2 shall be used as output
      digitalWrite(VOUT_JOY_PIN, HIGH) ;  //set pin A3 to high (+5)
      digitalWrite(GROUND_JOY_PIN,LOW) ;  //set pin Ad to low (ground)
      break;
  }

  // Servo
  myservo.attach(SERVO_PIN);  // attaches the servo on pin 9 to the servo object
  myservo.write(OPEN);

  Serial.println("Claw Retrival System setup complete.");
}

// Loop
void loop() {
  switch(mode) {
    case SONAR: 
      sonarSensor();
      break;
    case JOYSTICK:
      joystick();
      break;
  }
  if (blinkLED) {
    blink();
  }
}
