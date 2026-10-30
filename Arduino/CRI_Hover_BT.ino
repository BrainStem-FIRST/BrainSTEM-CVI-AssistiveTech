/*
  CHILD'S REACH INITIATIVE
  Interface Type: Big Hover

  Uses a single REV Push Button and two REV 2M Distance Sensors.
  When the left sensor is triggered, the needle moves left.
  When the right sensor is triggered, the needle moves right.
  When the button is pressed, an image is selected.

  Direction and Button Pressed values are sent to the center Bluetooth 

  This interface is used to confirm the selection on BrainSTEM CVI app.

  Arduino MKR WiFi 1010 is used.

  Copyright (c) 2025 Özge Ada Uzman, BrainSTEM Without Borders
*/

#include <ArduinoBLE.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Define Bluetooth service and characteristics
BLEService CRIService("9709493d-88ac-40a6-bba8-bc6a36d80612"); // creating the service Big Hover
BLEByteCharacteristic needleDirectionChar("c1742a43-7b21-4b6e-b5ae-cfb8392dd74c", BLERead | BLENotify); // creating the Needle Direction characteristic
BLEByteCharacteristic buttonChar("c9a1aee9-7995-47b1-a843-d3095196b44f", BLERead | BLENotify);  // creating the Push Button characteristic

// Define the two distance sensors as left and right I2C devices
Adafruit_VL53L0X lox1 = Adafruit_VL53L0X(); // Create an object for the left sensor
Adafruit_VL53L0X lox2 = Adafruit_VL53L0X(); // Create an object for the right sensor

// Define the pressure sensor as Push Button
bool buttonPressed  = false; // This describes whether or not the button has been pressed
const int buttonPIN = 0;  // The pushbutton's data wire is connected to digital pin #3

// Set the builtin LED as Bluetooth connection indicator
const int ledPIN = LED_BUILTIN; // for visual confirmation

// Timer to set the loop cycle period
long previousMillis = 0;

// Multiplexer address
#define TCAADDR 0x70


void setup() {
  // Initialize I2C library
  Wire.begin();

  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  // while (!Serial); // wait for serial port to connect. DO NOT DO THIS IF NOT CONNECTED VIA CABLE
  
  // Scan the I2C channels on multiplexer for devices (for debug purposes)
  Serial.println("\nTCAScanner ready!");
  
  for (uint8_t t=0; t<8; t++) {
    Serial.print("TCA Port #"); 
    changeChannel(t); // change the I2C channel
    Serial.println(t);

    for (uint8_t addr = 0; addr<=127; addr++) {
      if (addr == TCAADDR) continue;

      Wire.beginTransmission(addr);
      if (!Wire.endTransmission()) {
        Serial.print("Found I2C 0x");  Serial.println(addr,HEX);
      }
    }
  }
  
  

  // Initialize the I2C sensors
  Serial.println("Initializing I2C Sensors...");

  // Left distance sensor is on Channel 1
  changeChannel(1);
  if (!lox1.begin()) {
    Serial.println("Failed to find left VL53L0X sensor!");
    //while (1);
  }
  else Serial.println("Channel 1 initialized.");

  // Right distance sensor is on Channel 7
  changeChannel(7);
  if (!lox2.begin()) {
    Serial.println("Failed to find right VL53L0X sensor!");
    //while (1);
  }
  else Serial.println("Channel 7 initialized.");
  
  // set the pusbutton pin mode
  pinMode(buttonPIN, INPUT);   
  pinMode(ledPIN, OUTPUT);

  //initialize ArduinoBLE library
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy failed!");
    while (1);
  }

  //Setting a name that will appear when scanning for Bluetooth® devices
  BLE.setLocalName("Big Hover"); 
  BLE.setAdvertisedService(CRIService);

  CRIService.addCharacteristic(needleDirectionChar);
  CRIService.addCharacteristic(buttonChar);

  BLE.addService(CRIService);  // adding the service

  // Broadcast initial values of Needle Direction and PushButton status
  needleDirectionChar.writeValue((int8_t)getNeedleDirection());
  buttonChar.writeValue((int8_t)getButtonState());

  BLE.advertise(); //start advertising the service
  Serial.println(" Bluetooth® device active, waiting for connections...");
}

// Select I2C BUS
void changeChannel(uint8_t bus){
  if (bus > 7) return; // only 8 channels on the multiplexer 0..7
  Wire.beginTransmission(TCAADDR);  // TCA9548A address
  Wire.write(1 << bus);          // send byte to select bus
  Wire.endTransmission();
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

        needleDirectionChar.writeValue((int8_t)getNeedleDirection());
        buttonChar.writeValue((int8_t)getButtonState());
      }
    }
    
    digitalWrite(LED_BUILTIN, LOW); // when the central disconnects, turn off the LED
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}


int getNeedleDirection() {
  // Change to Right Channel (Ch#1)
  changeChannel(1);

  // Perform a distance measurement
  VL53L0X_RangingMeasurementData_t measure1;
  lox1.rangingTest(&measure1, false); // Pass 'true' to print debug info to Serial Monitor

  Serial.print("Distance Left (mm): ");

  int distanceLeft = measure1.RangeMilliMeter;
  // Check if the measurement is valid
  if (measure1.RangeStatus != 4) {  // 4 means no valid measurement
    Serial.println(distanceLeft);
  } else {
    Serial.println("Out of range");
    distanceLeft = -1;  // indicate invalid reading
  }

  // Change to Left Channel (Ch#7)
  changeChannel(7);

  VL53L0X_RangingMeasurementData_t measure2;
  lox2.rangingTest(&measure2, false); // Pass 'true' to print debug info to Serial Monitor

  Serial.print("Distance Right (mm): ");

  int distanceRight = measure2.RangeMilliMeter;
  // Check if the measurement is valid
  if (measure2.RangeStatus != 4) {  // 4 means no valid measurement
    Serial.println(distanceRight);
  } else {
    Serial.println("Out of range");
    distanceRight = -1;  // indicate invalid reading 
  }

  // which way the needle should move?
  // 0: stop;  (+): towards right;  (-): towards left;
  int direction = 0;

  // Serial.print("distance Left: ");
  // Serial.println(distanceLeft);

  //Serial.print("distance Right: ");
  //Serial.println(distanceRight);
  
  if (distanceLeft <=150 && distanceRight <=150
    && distanceLeft > 0  && distanceRight > 0){ 
    // Both sensors are on. 
    direction = 0;  
    Serial.println("Needle stopped!");
  }
  else if (distanceRight <=130 && distanceRight > 0){
    direction = 1;
    Serial.println("Needle moves RIGHT");
  }     
  else if (distanceLeft <=130 && distanceLeft > 0){
    direction = -1;
    Serial.println("Needle moves LEFT");
  }
  else {
   // direction = 0;
   // Serial.println("Needle stopped!");
  }

  return direction;  
}

bool getButtonState() {
  bool buttonState = !digitalRead(buttonPIN); //REV pushbutton returns FALSE when pressed
  
  //Serial.print("Button State: ");
  //Serial.println(buttonState);

  if (buttonState == HIGH) {
    buttonPressed = true;
  }
  else {
    buttonPressed = false;
  }

  return buttonPressed;

}