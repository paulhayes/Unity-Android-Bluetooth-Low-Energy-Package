/*
  Arduino Nano 33 BLE
  Arduino Nano 33 BLE Sense ( https://docs.arduino.cc/tutorials/nano-33-ble-sense-rev2/cheat-sheet/  )
  Seeed Xaio Nrf52840 ( https://wiki.seeedstudio.com/XIAO_BLE/ )
  Seeed Xaio Nrf52840 Sense
  Adafruit Feather ( https://learn.adafruit.com/introducing-the-adafruit-nrf52840-feather/pinouts )
*/
#if defined(ARDUINO_Seeed_XIAO_nRF52840) || defined(ARDUINO_Seeed_XIAO_nRF52840_Sense) || defined(ARDUINO_NRF52840_FEATHER_SENSE)
#define USE_BLUEFRUIT
#endif

#if defined(ARDUINO_Seeed_XIAO_nRF52840_Sense) || defined(ARDUINO_SEEED_XIAO_NRF52840_SENSE)
#define DEVICE_NAME "XIAO nRF52 Sense"
#elif defined(ARDUINO_Seeed_XIAO_nRF52840) || defined(ARDUINO_SEEED_XIAO_NRF52840)
#define DEVICE_NAME "XIAO nRF52"
#elif defined(ARDUINO_ARDUINO_NANO33BLE)
#define DEVICE_NAME "Nano 33 BLE"
#define PIN_VBAT A0
#elif defined(ARDUINO_NRF52840_FEATHER_SENSE)
#define DEVICE_NAME "nRF Feather Sense"
#else
#define DEVICE_NAME "Generic Device"
#define PIN_VBAT A0
#endif

#ifdef USE_BLUEFRUIT
#include <bluefruit.h>
#else
#include <ArduinoBLE.h>
#endif

#ifdef ARDUINO_NANO_33_BLE_SENSE
#include <Arduino_HTS221.h>
#define USE_HTS221
#endif
#if defined(ARDUINO_Seeed_XIAO_nRF52840_Sense) || defined(ARDUINO_SEEED_XIAO_NRF52840_SENSE)
#include "LSM6DS3.h"
#include "Wire.h"
#define USE_LSM6DS3
LSM6DS3 imu(I2C_MODE, 0x6A);
#endif
 // Bluetooth® Low Energy Battery Service
BLEService batteryService("180F");
BLEService genericService("6d006143-881f-440e-a5a9-711d675c73ce");
// Bluetooth® Low Energy Battery Level Characteristic
#ifdef USE_BLUEFRUIT

BLECharacteristic batteryLevelChar("2A19",  // standard 16-bit characteristic UUID
    BLERead | BLENotify); // remote clients will be able to get notifications if this characteristic changes

BLECharacteristic temperatureChar("2A6E",  // standard 16-bit characteristic UUID
    BLERead | BLENotify); // remote clients will be able to get notifications if this characteristic changes

BLECharacteristic ledCharacteristic("19B10000-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

#else 
BLEUnsignedCharCharacteristic batteryLevelChar("2A19",  // standard 16-bit characteristic UUID
    BLERead | BLENotify); // remote clients will be able to get notifications if this characteristic changes

BLEIntCharacteristic temperatureChar("2A6E",  // standard 16-bit characteristic UUID
    BLERead | BLENotify); // remote clients will be able to get notifications if this characteristic changes

BLEBoolCharacteristic ledCharacteristic("19B10000-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
#endif

int oldBatteryLevel = 0;  // last battery level reading from analog input
long previousMillis = 0;  // last time the battery level was checked, in ms
int oldTemperature = 0;



void setup() {
  Serial.begin(9600);    // initialize serial communication
  //while (!Serial);

  pinMode(LED_BLUE, OUTPUT);


  #ifdef USE_HTS221
  if (!HTS.begin()) {
    Serial.println("Failed to initialize humidity temperature sensor!");
    while (1);
  }
  #endif

  #ifdef USE_LSM6DS3
  if (imu.begin() != 0) {
        Serial.println("Device error");
    } else {
        Serial.println("Device OK!");
    }
  #endif



#ifdef USE_BLUEFRUIT

  if (!Bluefruit.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }
  Bluefruit.setTxPower(8);    // Check bluefruit.h for supported values
  Bluefruit.setName(DEVICE_NAME);
  Bluefruit.Periph.setConnectCallback(blePeripheralConnectHandler);
  Bluefruit.Periph.setDisconnectCallback(blePeripheralDisconnectHandler);

  genericService.begin();
  
  batteryLevelChar.setFixedLen(1);
  batteryLevelChar.begin();

  temperatureChar.setFixedLen(2);
  temperatureChar.begin();
  
  ledCharacteristic.setFixedLen(1);
  ledCharacteristic.setWriteCallback(onLEDChanged);
  ledCharacteristic.begin();
  


  Bluefruit.Advertising.addService(genericService);
  Bluefruit.ScanResponse.addName();

#else
  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }
  /* Set a local name for the Bluetooth® Low Energy device
     This name will appear in advertising packets
     and can be used by remote devices to identify this Bluetooth® Low Energy device
     The name can be changed but maybe be truncated based on space left in advertisement packet
  */
  genericService.addCharacteristic(batteryLevelChar); // add the battery level characteristic
  genericService.addCharacteristic(temperatureChar);
  genericService.addCharacteristic(ledCharacteristic);
  
  BLE.setLocalName(DEVICE_NAME);
  BLE.addService(batteryService); // Add the battery service
  BLE.addService(genericService);
  BLE.setAdvertisedService(genericService);
  //BLE.setAdvertisedService(batteryService); // add the service UUID
  
  batteryLevelChar.writeValue(oldBatteryLevel); // set initial value for this characteristic
  temperatureChar.writeValue(oldTemperature);
  /* Start advertising Bluetooth® Low Energy.  It will start continuously transmitting Bluetooth® Low Energy
     advertising packets and will be visible to remote Bluetooth® Low Energy central devices
     until it receives a new connection */

  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  ledCharacteristic.setEventHandler(BLEWritten, onLEDChanged);

  // start advertising
  BLE.advertise();

  Serial.println("Bluetooth® device active, waiting for connections...");

#endif

}

void loop() {
#ifdef USE_BLUEFRUIT

#else
    BLE.poll();
#endif
    long currentMillis = millis();
    // if 200ms have passed, check the battery level:
    if (currentMillis - previousMillis >= 200) {
      previousMillis = currentMillis;
      updateBatteryLevel();
      updateTemperature();
    }
}

#ifdef USE_BLUEFRUIT
void blePeripheralConnectHandler(uint16_t conn_handle) {
  ledCharacteristic.write8((uint8_t)true);
#else
void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
  ledCharacteristic.writeValue(true);
#endif
    // turn on the LED to indicate the connection:
  digitalWrite(LED_BLUE, LOW);
}

#ifdef USE_BLUEFRUIT
void blePeripheralDisconnectHandler(uint16_t conn_handle, uint8_t reason) {
#else
void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
#endif
  digitalWrite(LED_BLUE, HIGH);
}

#ifdef USE_BLUEFRUIT
void onLEDChanged(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len){
  bool isOn = (bool)data[0];
#else
void onLEDChanged(BLEDevice central, BLECharacteristic characteristic){
  bool isOn = ledCharacteristic.value();
#endif
  Serial.print("LED value written: ");
  Serial.println(isOn);
  digitalWrite(LED_BLUE, isOn ? LOW : HIGH); 
}

void updateBatteryLevel() {
  /* Read the current voltage level on the A0 analog input pin.
     This is used here to simulate the charge level of a battery.
  */

  int battery = analogRead(PIN_VBAT);
  int batteryLevel = map(battery, 0, 1023, 0, 100);

  if (batteryLevel != oldBatteryLevel) {      // if the battery level has changed
    // Serial.print("Battery Level % is now: "); // print it
    // Serial.println(batteryLevel);
    #ifdef USE_BLUEFRUIT
    batteryLevelChar.write16(batteryLevel);
    #else
    batteryLevelChar.writeValue(batteryLevel);  // and update the battery level characteristic
    #endif
    oldBatteryLevel = batteryLevel;           // save the level for next comparison
  }
}

void updateTemperature(){
  
  int temperature = 0;
  #ifdef USE_HTS221
  int temperature = round(HTS.readTemperature(CELSIUS)*100);
  //float humidity    = HTS.readHumidity();
  #endif
  
  #ifdef USE_LSM6DS3
  temperature = round(imu.readTempC()*100);
  #endif

  // Serial.print("Temperature is now: "); 
  // Serial.print(temperature/100.0f,2);
  // Serial.println("°C");
  if(temperature!=oldTemperature){
    #ifdef USE_BLUEFRUIT
    temperatureChar.write16(temperature);
    #else
    temperatureChar.writeValue(temperature);
    #endif
    oldTemperature = temperature;
  }
}
