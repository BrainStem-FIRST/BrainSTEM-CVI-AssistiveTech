/*
  CHILD'S REACH INITIATIVE
  Interface Type: Big Button

  Uses a single REV Push Button
  
  Button Pressed value is sent to the center Bluetooth.
  This interface is used to confirm the selection on BrainSTEM CVI app.

  Alternative use of Big Button:
     Have two buttons. One sets the button state to -1 when pressed as Needle Direction, and the other setsit to +1 when pressed.
     For children with severe impairment on their hands, each button can be attached to left and right armrest of the wheelchair
     where the child can press with their elbows.
     See Big Hover interface for sending NeedleDirection to BrainSTEM CVI App.

  Arduino MKR WiFi 1010 is used.

  Copyright (c) 2024 Özge Ada Uzman, BrainSTEM Learning
*/

#include <SPI.h>
#include <ArduinoBLE.h>

BLEService CRIService("ce5c9979-2238-4470-a2f7-889c41a126d6"); // creating the service 180A
BLEUnsignedCharCharacteristic confirmChar("971dd13d-b312-4502-8429-cc19b7f0fc4c", BLERead | BLENotify); // creating the Analog Value characteristic

const int buttonPIN = 2;  // The pushbutton's data wire is connected to digital pin #2
const int ledPIN = LED_BUILTIN; // for visual confirmation
long previousMillis = 0;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);

  pinMode(buttonPIN, INPUT);      // set the pushbutton pin mode
  pinMode(ledPIN, OUTPUT);

  //initialize ArduinoBLE library
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy failed!");
    while (1);
  }

  BLE.setLocalName("Push Button"); //Setting a name that will appear when scanning for Bluetooth® devices
  BLE.setAdvertisedService(CRIService);

  CRIService.addCharacteristic(confirmChar);

  BLE.addService(CRIService);  // adding the service

  confirmChar.writeValue((int8_t)getButtonState());

  BLE.advertise(); //start advertising the service
  Serial.println(" Bluetooth® device active, waiting for connections...");
}

void loop() {
  
  BLEDevice central = BLE.central(); // wait for a Bluetooth® Low Energy central

  if (central) {  // if a central is connected to the peripheral
    Serial.print("Connected to central: ");
    
    Serial.println(central.address()); // print the central's BT address
    
    digitalWrite(LED_BUILTIN, HIGH); // turn on the LED to indicate the connection


    while (central.connected()) { // while the central is connected:
      
      long currentMillis = millis();
      
      if (currentMillis - previousMillis >= 200) { // repeat every 200 milliseconds
        previousMillis = currentMillis;

        confirmChar.writeValue((int8_t)getButtonState());
      }
    }
    
    digitalWrite(LED_BUILTIN, LOW); // when the central disconnects, turn off the LED
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }

}

// Sends buttonState to javaScript
int getButtonState() {
  // prepare the serverValue based on the duration the button pressed
  bool buttonState = !digitalRead(buttonPIN); // REV pushbutton returns FALSE when pressed
  Serial.print("Button State: ");
  Serial.println(buttonState);

  return buttonState; //test
  
}

