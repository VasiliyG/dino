/*
  Library for dino ruby gem.
*/

#include "Arduino.h"
#include "Dino.h"

Dino::Dino(){
  reset();
}

void Dino::parse(char c) {
  if (c == '!') index = 0;        // Reset request
  else if (c == '.') process();   // End request and process
  else request[index++] = c;      // Append to request
}

void Dino::process() {
  
  // Default response.
  response[0] = '\0';

  // Parse the request.
  strncpy(cmdStr, request, 2);      cmdStr[2] =  '\0';
  strncpy(pinStr, request + 2, 2);  pinStr[2] =  '\0';
  strncpy(valStr, request + 4, 3);  valStr[3] =  '\0';
  cmd = atoi(cmdStr);
  pin = atoi(pinStr);
  val = atoi(valStr);

  // Call the command.
  switch(cmd) {
    case 0:  setMode             ();  break;
    case 1:  dWrite              ();  break;
    case 2:  dRead               ();  break;
    case 3:  aWrite              ();  break;
    case 4:  aRead               ();  break;
    case 5:  addDigitalListener  ();  break;
    case 6:  addAnalogListener   ();  break;
    case 7:  removeListener      ();  break;
    case 90: reset               ();  break;
    case 98: setHeartRate        ();  break;
    case 99: toggleDebug         ();  break;
    default:                          break;
  }
  
  // Write the response.
  if (response[0] != '\0') writeResponse();
}


void Dino::setupWrite(void (*writeCallback)(char *str)) {
  _writeCallback = writeCallback;
}
void Dino::writeResponse() {
  _writeCallback(response);
}


void Dino::updateListeners() {
  if (timeSince(lastUpdate) > heartRate || timeSince(lastUpdate) < 0) {
    lastUpdate = micros();
    updateAnalogListeners();
    updateDigitalListeners();
  }
}

void Dino::updateDigitalListeners() {
  for (int i = 0; i < 22; i++) {
    if (digitalListeners[i]) {
      pin = i;
      dRead();
      if (rval != digitalListenerValues[i]) {
        digitalListenerValues[i] = rval;
        writeResponse();
      } 
    }
  }
}

void Dino::updateAnalogListeners() {
  for (int i = 0; i < 22; i++) {
    if (analogListeners[i]) {
      pin = i;
      aRead();
      writeResponse();
    }
  }
}

long Dino::timeSince(long event) {
 long time = micros() - event;
 return time;
}



// CMD = 00 // Pin Mode
void Dino::setMode() {
  if (val == 0)
    pinMode(pin, OUTPUT);
  else {
    removeListener();
    pinMode(pin, INPUT);
  }
}

// CMD = 01 // Digital Write
void Dino::dWrite() {
  if (val == 0)
    digitalWrite(pin, LOW);
  else
    digitalWrite(pin, HIGH);
}

// CMD = 02 // Digital Read
void Dino::dRead() { 
  rval = digitalRead(pin);
  sprintf(response, "%02d:%02d", pin, rval);
}

// CMD = 03 // Analog (PWM) Write
void Dino::aWrite() {
  analogWrite(pin,val);
}

// CMD = 04 // Analog Read
void Dino::aRead() {
  rval = analogRead(pin);
  sprintf(response, "%02d:%02d", pin, rval);
}


// CMD = 10
// Listen for a digital signal on any pin.
void Dino::addDigitalListener() {
  removeListener();
  digitalListeners[pin] = true;
  digitalListenerValues[pin] = 2;
}

// CMD = 11
// Listen for an analog signal on analog pins only.
void Dino::addAnalogListener() {
  removeListener();
  analogListeners[pin] = true;
}

// CMD = 12
// Remove analog and digital listeners from any pin.
void Dino::removeListener() {
  analogListeners[pin] = false;
  digitalListeners[pin] = false;
}

// CMD = 90
void Dino::reset() {
  debug = false;
  heartRate = 4940; // Default heartRate is ~5ms.
  for (int i = 0; i < 22; i++) digitalListeners[i] = false;
  for (int i = 0; i < 22; i++) digitalListenerValues[i] = 2;
  for (int i = 0; i < 22; i++)  analogListeners[i] = false;
  lastUpdate = micros();
  index = 0;
  sprintf(response, "ACK:%02d", A0);
}

// CMD = 98
// Set the heart rate in milliseconds.
void Dino::setHeartRate() {
  long rate = val;
  heartRate = (rate * 1000) - 60;
}

// CMD = 99
void Dino::toggleDebug() {
  if (val == 0) {
    debug = false;
    strcpy(response, "Debug 0");
  } else {
    debug = true;
    strcpy(response, "Debug 1");
  }
}
