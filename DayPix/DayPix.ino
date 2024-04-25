#include <WiFi.h>
#include <ESPAsyncWebSrv.h>
#include <EEPROM.h>
//#include <ArtnetWiFi.h>
#include <ArtnetETH.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "rgb_effects.h"
#include "LedDriver.h"
#include <ArduinoOTA.h>
#include "Config.h"
#include "WebServerFunctions.h"
#include <ESPmDNS.h>
#include <esp_eth.h>
#include "HwFunctions.h"

// Definitions
// define for wifi or eth boards
//#define artnet ArtnetWiFiReceiver
//#define artnet ArtnetReceiver
//ArtnetReceiver artnet;
ArtnetReceiver artnet;

ledDriver led;
RGBEffects effect;

// DMX array data
const uint8_t* bdata;
uint16_t bsize;

const uint8_t* bdata2;
uint16_t bsize2;

int cb1calls=0;
int cb2calls=0;
// Artnet Callback
void callback(const uint8_t* data, const uint16_t size) {
    
   //bdata = data;
   //bsize = size;
  if (recvUniverse.indexOf(String(artnet.universe())) == -1) {
      // The recvUniverse does not contain artnet.universe(), so add it
      recvUniverse += "U" + String(artnet.universe()) + "U";
    }

  led.writePixelBuffer(data, size, NrOfLeds, DmxAddr, 0);
  //led.showBuffer();

  //Serial.println(artnet.universe());
  //Serial.println(artnet.universe15bit());
}
void callback1(const uint8_t* data, const uint16_t size) {
    
   //bdata = data;
   //bsize = size;
  //if (recvUniverse.indexOf(String(artnet.universe())) == -1) {
      // The recvUniverse does not contain artnet.universe(), so add it
  //    recvUniverse += "U" + String(artnet.universe()) + "U";
 // }
 if (recvUniverse.indexOf(String(artnet.universe())) == -1) {
      // The recvUniverse does not contain artnet.universe(), so add it
      recvUniverse += "U" + String(artnet.universe()) + "U";
  }
  led.writePixelBuffer(data, size, NrOfLeds, DmxAddr, 0);


  //Serial.println(artnet.universe());
  //Serial.println(artnet.universe15bit());
}
void callback2(const uint8_t* data, const uint16_t size) {
  //bdata2 = data;
  //bsize2 = size;
  //fillArray(data, size);
  if (recvUniverse.indexOf(String(artnet.universe())) == -1) {
      // The recvUniverse does not contain artnet.universe(), so add it
      recvUniverse += "U" + String(artnet.universe()) + "U";
  }
  led.writePixelBufferPort2(data, size, NrOfLeds, DmxAddr, 0);
  //led.showBufferP2();
}
void callback3(const uint8_t* data, const uint16_t size) {
  //bdata2 = data;
  //bsize2 = size;
  //fillArray(data, size);
  if (recvUniverse.indexOf(String(artnet.universe())) == -1) {
      // The recvUniverse does not contain artnet.universe(), so add it
      recvUniverse += "U" + String(artnet.universe()) + "U";
  }
  led.writePixelBufferPort2(data, size, NrOfLeds, DmxAddr, 0);

}

// Task to get Artnet data frame and write to pixel buffer
void artnetTask(void* parameter) {
  while (1) {

    vTaskDelay(10 / portTICK_PERIOD_MS);  // Adjust delay as needed

    if (identify) {
      delay(2000);
      identify = false;
    }
    while (diag) {
      //do nothing since we are in diag mode
      delay(500);
    }
  }
}

void artnetTask2(void* parameter) {
  while (1) {
    led.writePixelBufferPort2(bdata2, bsize2, NrOfLeds, DmxAddr, 0);
    vTaskDelay(1 / portTICK_PERIOD_MS);  // Adjust delay as needed

    if (identify) {
      delay(2000);
      identify = false;
    }
    while (diag) {
      //do nothing since we are in diag mode
      delay(500);
    }
  }
}


void conGuardTask(void* parameter) {
  while(1){

    while (WiFi.status() != WL_CONNECTED){
      if (!ETH.linkUp()){
        ESP.restart();
      }
       vTaskDelay(5 / portTICK_PERIOD_MS);  // Adjust delay as needed
    }
    while(WiFi.status() == WL_CONNECTED){
      if (ETH.linkUp()){
        ESP.restart();
      }
      vTaskDelay(5 / portTICK_PERIOD_MS);  // Adjust delay as needed
    }
   
    /*if (ETH.linkUp() && WiFi.isConnected()) {
      Serial.println("Eth and WIFI Connected");
      delay(500);
      ESP.restart();
    }*/
    
  }
}
void otaTask(void* parameter) {
  while(1){
     ArduinoOTA.handle();
     vTaskDelay(5 / portTICK_PERIOD_MS);  // Adjust delay as needed
  }
}

// Setup of the runtime
void setup() {
  // init serial for debugging
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting setup...");
  // init eeprom
  EEPROM.begin(512);
  initializeEEPROM();
  ETH.begin();
  // debug listing all files on filesystem
  //listFiles(SPIFFS, "/");

  // Retrieve stored configuration
  String storedSSID = getStoredString(SSID_EEPROM_ADDR);
  String storedPassword = getStoredString(PASS_EEPROM_ADDR);
  String storedUniverse = getStoredString(UNIVERSE_EEPROM_ADDR);
  String storedDmxAddr = getStoredString(DMX_ADDR_EEPROM_ADDR);
  String storedNrofLEDS = getStoredString(NRLEDS_EEPROM_ADDR);
  String storedB16Bit = getStoredString(B_16BIT_EEPROM_ADDR);  // Add this line
  String storedBfailover = getStoredString(B_FAILOVER_EEPROM_ADDR);
  String storedBsilent = getStoredString(B_SILENT_EEPROM_ADDR);
  String storedDeviceName = getStoredString(DEV_NAME_EEPROM_ADDR);
  int storedUniverseStart = getStoredString(UNIVERSE_START_EEPROM_ADDR).toInt();
  int storedUniverseEnd = getStoredString(UNIVERSE_END_EEPROM_ADDR).toInt();
  // Set configuration parameters
  universe1 = storedUniverse.toInt();
  NrOfLeds = storedNrofLEDS.toInt();
  DmxAddr = storedDmxAddr.toInt();
  b_16Bit = storedB16Bit.toInt(); 
  b_failover = storedBfailover.toInt(); // Add this line
  b_silent = storedBsilent.toInt();


  // Set device name based on the last two digits of the MAC address if not been set by user
  if (storedDeviceName == "" || storedDeviceName == "null") {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    String lastTwoDigits = String(mac[4], HEX) + String(mac[5], HEX);
    // Make unique hostname based on last digets of mac address
    lastTwoDigits.toUpperCase();
    DEV_NAME = lastTwoDigits;
  } else {
    DEV_NAME = storedDeviceName;
  }
  HOST_NAME = H_PRFX + "-" + DEV_NAME;
  ArduinoOTA.setHostname(HOST_NAME.c_str());
  WiFi.setHostname(HOST_NAME.c_str());

  // Print stored values for debugging
  Serial.println("DEV_NAME: " + DEV_NAME);
  Serial.println("SSID: " + storedSSID);
  Serial.println("Password: " + storedPassword);
  Serial.println("nrOfLEDS: " + storedNrofLEDS);
  Serial.println("DMX Address: " + storedDmxAddr);
  Serial.println("16BitMode: " + b_16Bit);
  Serial.println("storedUniverseStart: " + storedUniverseStart);
  Serial.println("storedUniverseEnd: " + storedUniverseEnd);

  Serial.print("WiFi Hostname: ");
  Serial.println(WiFi.getHostname());

  // Get and print the mDNS name (service name)
  Serial.print("mDNS Name: ");
  Serial.println(MDNS.hostname(0));

  // Initialize LED and effect
  effect.initialize();
  led.initialize(NrOfLeds, DmxAddr);
  Serial.println("Nework setup start");
  if (ETH.linkUp()) {
    Serial.println("Ethernet connected");
    Serial.println(ETH.localIP());
  } else {
    Serial.println("Ethernet not connected starting wifi");
    // Start Access Point if no stored WiFi credentials
    if (storedSSID.isEmpty() || storedPassword.isEmpty()) {
      startAccessPoint();
    } else {
      // Connect to WiFi
      WiFi.begin(storedSSID.c_str(), storedPassword.c_str());


      // Attempt to connect with a timeout
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < WIFI_ATTEMPT) {
        if (b_silent == 0)
        {
          led.blink(attempts);
        }
        delay(1000);
        Serial.println("Connecting to WiFi...");
        attempts++;
        if (ETH.linkUp())
        {
          Serial.println("ETH connected, rebooting...");
          ESP.restart();
        }
      }

      // If not connected, start Access Point and setup
      if (WiFi.status() != WL_CONNECTED) {
        startAccessPoint();
      } else {
        Serial.println("Connected to WiFi");
        Serial.println(WiFi.localIP());
        Serial.print("WiFi Hostname: ");
        Serial.println(WiFi.getHostname());
      }
    }
  }
  
  // if connected start connection guard task to ensure we are alwasy able to connect.
  if (b_failover > 0)
  {
      xTaskCreatePinnedToCore(conGuardTask, "ConGuardTask", 2048, NULL, 3, NULL, 1);
  }
  // If connected, start normal mode
  Serial.println("Nework setup done");
  MDNS.begin(HOST_NAME.c_str());
  MDNS.disableArduino();
  // add Http service to mdns
  MDNS.addService("http", "tcp", 80);
  // Setup webserver
  setupWebServer();
  //b_APmode = false;

  // Get and print the mDNS name (service name)
  Serial.print("mDNS Name: ");
  Serial.println(MDNS.hostname(0));
  // display led effect when connected
  if (b_silent == 0)
  {
    led.ledTest();
  }
  delay(1);

  // Create FreeRTOS task for ArtNet updates on core 1
  Serial.println("Starting ArtNet");
  //xTaskCreatePinnedToCore(artnetTask, "ArtnetTask", 8192, NULL, 1, NULL, 1);
 // xTaskCreatePinnedToCore(artnetTask2, "ArtnetTask2", 8192, NULL, 1, NULL, 1);
  // Set Artnet discovery name
  artnet.shortname(HOST_NAME);
  artnet.longname(HOST_NAME + " ArtNet node");
  artnet.begin();
  //artnet.forward()

  // remove all possible subscribtions of universes;
  for (int i = 0; i < 15; i++) {
    artnet.unsubscribe(i);
  }
    // Subscribe callback to the first universe
  artnet.subscribe(storedUniverseStart, callback);
  Serial.println("Add universe listener for " + String(storedUniverseStart));

  // Subscribe callback to the second universe
  if (storedUniverseStart + 1 <= storedUniverseEnd) {
      artnet.subscribe(storedUniverseStart + 1, callback1);
      Serial.println("Add universe listener for " + String(storedUniverseStart + 1));
  }

  // Subscribe callback2 to the third universe
  if (storedUniverseStart + 2 <= storedUniverseEnd) {
      artnet.subscribe(storedUniverseStart + 2, callback2);
      Serial.println("Add universe listener for " + String(storedUniverseStart + 2) + " for callback2");
  }

  // Subscribe callback2 to the fourth universe
  if (storedUniverseStart + 3 <= storedUniverseEnd) {
      artnet.subscribe(storedUniverseStart + 3, callback3);
      Serial.println("Add universe listener for " + String(storedUniverseStart + 3) + " for callback2");
  }
  // Begin OTA update

  ArduinoOTA.begin();

  xTaskCreatePinnedToCore(otaTask, "OtaTask", 8192, NULL, 2, NULL, 1);
  Serial.println("Setup complete");
  
}



// Main loop does nothing
void loop() {
     artnet.parse();
}
