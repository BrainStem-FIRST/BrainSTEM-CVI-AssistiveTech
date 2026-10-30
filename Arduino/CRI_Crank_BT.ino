/*
  CHILD'S REACH INITIATIVE
  Interface Type: Big Crank

  Arduino MKR WiFi 1010 is used as Bluetooth Peripheral with madeup UUIDs for Service and Characteristics.
  needleAngleChar: Sends an integer [-90..+90] as angle of the needle

  This interface is used to confirm the selection on BrainSTEM CVI app.
  
  Copyright (c) 2024 Özge Ada Uzman, BrainSTEM Learning
*/

#include <SPI.h>
#include <ArduinoBLE.h>

BLEService CRIService("feba623b-2ed9-4b05-89e3-9b5949864147"); // creating the service 180A
BLEUnsignedCharCharacteristic needleAngleChar("545a56a6-dc36-4569-a4cc-12370e11fcaa", BLERead | BLENotify); // creating the Analog Value characteristic
//BLEByteCharacteristic buttonChar("e2937984-0f46-4118-8139-d846637cd81b", BLERead | BLENotify);

const int crankPIN = A1;  // The pushbutton's data wire is connected to digital pin #2
const int ledPIN = LED_BUILTIN; // for visual confirmation
long previousMillis = 0;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);

  pinMode(crankPIN, INPUT);      // set the pushbutton pin mode
  pinMode(ledPIN, OUTPUT);

  //initialize ArduinoBLE library
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy failed!");
    while (1);
  }

  BLE.setLocalName("Big Crank"); //Setting a name that will appear when scanning for Bluetooth® devices
  BLE.setAdvertisedService(CRIService);

  CRIService.addCharacteristic(needleAngleChar);

  BLE.addService(CRIService);  // adding the service

  needleAngleChar.writeValue((int8_t)getNeedleAngle());

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

        needleAngleChar.writeValue((int8_t)getNeedleAngle());
      }
    }
    
    digitalWrite(LED_BUILTIN, LOW); // when the central disconnects, turn off the LED
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }

}


int getNeedleAngle() {
  // prepare the serverValue based on the duration the button pressed
  int crankState = analogRead(A1); // Read sensor value
  int angle = map(crankState, 0, 1023, -90, 90); // Map sensor value (0-1023) to needle angle (-90-90)

  //Serial.print("Crank State: ");
  //Serial.println(crankState);

  //Serial.print("Needle Direction: ");
  //Serial.println(direction);

  return angle;
  
}