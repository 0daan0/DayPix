#include <WiFi.h>
#include <ESPAsyncWebSrv.h>
#include <EEPROM.h>
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
#include <Preferences.h>
#include <freertos/semphr.h>

#ifdef ETH_CAP
#include <ArtnetETH.h>
ArtnetReceiver artnet;
#else

#include <ArtnetWiFi.h>
ArtnetWiFiReceiver artnet;
#endif

// Global variables to store data and size
uint8_t* bdata1[512];
uint8_t* bdata2[512];
uint8_t* bdata3[512];
uint8_t* bdata4[512];
uint16_t bsize1 = 0;
uint16_t bsize2 = 0;
uint16_t bsize3 = 0;
uint16_t bsize4 = 0;
volatile bool u1rec = false; 
volatile bool u2rec = false; 
volatile bool u3rec = false; 
volatile bool u4rec = false; 
SemaphoreHandle_t xMutex;

ledDriver led;
RGBEffects effect;
#ifdef RST_BTN
 int rLED = IO5;
 int sLED = IO17;
#endif

void callback(const uint8_t* data, const uint16_t size) {    
    if (recvUniverse.indexOf(String(artnet.universe())) == -1) {
        // The recvUniverse does not contain artnet.universe(), so add it
        recvUniverse += "U" + String(artnet.universe()) + "U";
    }  
    led.writePixelBuffer(data, size, NrOfLeds, DmxAddr, 0, true);
}

void callback1(const uint8_t* data, const uint16_t size) {
    if (recvUniverse.indexOf("U" + String(artnet.universe()) + "U") == -1) {
        recvUniverse += "U" + String(artnet.universe()) + "U";   
    }
    xSemaphoreTake(xMutex, portMAX_DELAY);
    memcpy(bdata1, data, size);
    bsize1 = size;
    u1rec = true;
    
    xSemaphoreGive(xMutex);
    checkAndWritePixelBuffer();
}

void callback2(const uint8_t* data, const uint16_t size) {
    if (recvUniverse.indexOf("U" + String(artnet.universe()) + "U") == -1) {
        recvUniverse += "U" + String(artnet.universe()) + "U";
    }
    xSemaphoreTake(xMutex, portMAX_DELAY); 
    memcpy(bdata2, data, size);
    bsize2 = size;
    u2rec = true;
    
    xSemaphoreGive(xMutex);
    checkAndWritePixelBuffer();
}

void callback3(const uint8_t* data, const uint16_t size) {
  if (recvUniverse.indexOf("U" + String(artnet.universe()) + "U") == -1) {
        recvUniverse += "U" + String(artnet.universe()) + "U";
    }
    xSemaphoreTake(xMutex, portMAX_DELAY); 
    memcpy(bdata3, data, size);
    bsize3 = size;
    u3rec = true;
    
    xSemaphoreGive(xMutex);
    checkAndWritePixelBuffer();
}

void callback4(const uint8_t* data, const uint16_t size) {
  if (recvUniverse.indexOf("U" + String(artnet.universe()) + "U") == -1) {
        recvUniverse += "U" + String(artnet.universe()) + "U";
    }
    xSemaphoreTake(xMutex, portMAX_DELAY); 
    memcpy(bdata4, data, size);
    bsize4 = size;
    u4rec = true;
    
    xSemaphoreGive(xMutex);
    checkAndWritePixelBuffer();
}

void checkAndWritePixelBuffer() {
    xSemaphoreTake(xMutex, portMAX_DELAY);
    
    if (u1rec && u2rec && u3rec && u4rec) {
        // Combine data from both callbacks by attaching them back-to-front
        uint8_t combinedData[bsize1 + bsize2 + bsize3 + bsize4];
        memcpy(combinedData, bdata1, bsize1);
        memcpy(combinedData + bsize1, bdata2, bsize2);
        memcpy(combinedData + bsize1 + bsize2, bdata3, bsize3);
        memcpy(combinedData + bsize1 + bsize2 + bsize3, bdata4, bsize4);

        led.writePixelBuffer(combinedData, bsize1 + bsize2 + bsize3 + bsize4, NrOfLeds * 4, DmxAddr, 0, true);
        // Reset flags
        u1rec = false;
        u2rec = false;
        u3rec = false;
        u4rec = false;
    }
    
    xSemaphoreGive(xMutex);
}


// Task to get Artnet data frame and write to pixel buffer
void artnetTask(void* parameter) {
  while (1) {

    vTaskDelay(10 / portTICK_PERIOD_MS);  // Adjust delay as needed
    while (!diag){
        artnet.parse();
    }
  }
}
/*
void artnetTask2(void* parameter) {
  while (1) {
    led.writePixelBufferPort2(bdata2, bsize2, NrOfLeds, DmxAddr, 0, true);
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
*/

#ifdef ETH_CAP
void conGuardTask(void* parameter) {
  while(1){

    while (WiFi.status() != WL_CONNECTED){
      if (led.ethCap){
    
        if(!ETH.linkUp() && !b_APmode){
          Serial.println("Not Connected to wifi and ethernet .. will reboot");
          ESP.restart();
        }
      }
       vTaskDelay(5 / portTICK_PERIOD_MS);  // Adjust delay as needed
    }
    while(WiFi.status() == WL_CONNECTED){
      if (led.ethCap){
        if (ETH.linkUp()){
          ESP.restart();
        }
      }
      vTaskDelay(5 / portTICK_PERIOD_MS);  // Adjust delay as needed
    }  
  }
}
#endif

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
  xMutex = xSemaphoreCreateMutex();
  // WHY THIS DELAY?
  delay(500);

  Serial.println("Starting setup...");
  
  // init eeprom
  EEPROM.begin(512);
  initializeEEPROM();
  setupHw();

#ifdef RST_BTN
// Reset to default at startup button press
const int debounceDelay = 50; // debounce delay in milliseconds
const int stablePressDuration = 200; // minimum duration the button should be pressed to consider it a valid press in milliseconds
//const int longPressDuration = 10000; // duration the button needs to be pressed in the second stage (10 seconds)
unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
int lastButtonState = HIGH; // the previous reading from the input pin
bool buttonPressed = false;
bool longPressDetected = false;

pinMode(rLED, OUTPUT);
pinMode(sLED, OUTPUT);
digitalWrite(rLED, LOW);
digitalWrite(sLED, HIGH);
pinMode(led.reset_Button, INPUT_PULLUP);
delay(100);

unsigned long startTime = millis();

// Initial debounce check for 2 seconds during startup
while (millis() - startTime < 500) {
  int reading = digitalRead(led.reset_Button);
  
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && (millis() - lastDebounceTime) > stablePressDuration) {
      buttonPressed = true;
      break;
    }
  }
  
  lastButtonState = reading;
  delay(10);
}

if (buttonPressed) {
  Serial.println("Initial button press detected. Hold for 10 seconds to reset to default.");
  digitalWrite(rLED, HIGH); // Indicate that we are in the second stage
  lastDebounceTime = millis(); // Reset the debounce timer for the second stage
  unsigned long pressStartTime = millis();
  int i = 13;
  while (i > 0) {
    int reading = digitalRead(led.reset_Button);
    
    if (reading == HIGH) {
      // Button released, exit the second stage
      Serial.println("Button released before 10 seconds. Continuing setup.");
      digitalWrite(rLED, LOW);
      buttonPressed = false;
      break;
    }
    led.blink(i,0);
    i--;
    delay(500);
  }

  if (buttonPressed) {
    longPressDetected = true;
  }
}

if (longPressDetected) {
  digitalWrite(rLED, HIGH);
  resetToDefault();
  Serial.println("Reset to default DONE!");
  for (int i = 20; i >= 0; i--) {
    digitalWrite(sLED, HIGH);
    led.blink(1,20);
    delay(10);
    digitalWrite(sLED, LOW);
    delay(10);
  }
} 
#endif

  // init ethernet of device is capable
  #ifdef ETH_CAP
   ETH.begin();
  #endif
// #region Retrieve stored configuration
  
  // ArtNet settings
  String storedUniverse = getStoredString(UNIVERSE_EEPROM_ADDR);
  int storedUniverseStart = getStoredString(UNIVERSE_START_EEPROM_ADDR).toInt();
  int storedUniverseEnd = getStoredString(UNIVERSE_END_EEPROM_ADDR).toInt();
  String storedDmxAddr = getStoredString(DMX_ADDR_EEPROM_ADDR);
  String storedReverseArray = getStoredString(B_REVERSE_ARRAY_EEPROM_ADDR);
  // LED settings
  String storedNrofLEDS = getStoredString(NRLEDS_EEPROM_ADDR);
  String storedB16Bit = getStoredString(B_16BIT_EEPROM_ADDR);  // Add this line
  // Device settings
  String storedBfailover = getStoredString(B_FAILOVER_EEPROM_ADDR);
  String storedBsilent = getStoredString(B_SILENT_EEPROM_ADDR);
  String storedDeviceName = getStoredString(DEV_NAME_EEPROM_ADDR);
  // Wifi settings
  String storedSSID = getStoredString(SSID_EEPROM_ADDR);
  String storedPassword = getStoredString(PASS_EEPROM_ADDR);
  // DHCP config 
  String storedDHCP = getStoredString(B_DHCP_EEPROM_ADDR);
  String storedIP = getStoredString(IP_ADDR_EEPROM_ADDR);
  String storedSubnet = getStoredString(IP_SUBNET_EEPROM_ADDR);
  String storedGateway = getStoredString(IP_GATEWAY_EEPROM_ADDR);
  String storedDNS = getStoredString(IP_DNS_EEPROM_ADDR);
  float storedGamma = getStoredFloat(GAMMA_VALUE_EEPROM_ADDR);
  if (storedGamma >= 0.01) {
      GAMMA_CORRECTION = storedGamma;
  }
  // Set configuration parameters
  universe1 = storedUniverse.toInt();
  NrOfLeds = storedNrofLEDS.toInt();
  DmxAddr = storedDmxAddr.toInt();
  b_16Bit = storedB16Bit.toInt(); 
  b_failover = storedBfailover.toInt(); // Add this line
  b_silent = storedBsilent.toInt();
  b_dhcp = storedDHCP.toInt();
  b_reverseArray = storedReverseArray.toInt();
  IP_ADDR = storedIP;
  IP_SUBNET = storedSubnet;
  IP_GATEWAY = storedGateway;
  IP_DNS = storedDNS;
  //b_dhcp = storedDHCP;
// #endregion

  // If DHCP is disabled, try to apply static ip config
  if (b_dhcp == 0){
    bool bValid = false;
      Serial.println("Configure static IP...");
      // Convert IP address from string to IPAddress object
      IPAddress ip;
      IPAddress subnet;
      IPAddress gateway;
      IPAddress dns;
      if (ip.fromString(IP_ADDR)) { // check if IP is correct format
          Serial.println("IP Address: "+ IP_ADDR);
          if (subnet.fromString(IP_SUBNET)) { // check if subnet mask is correct format
              Serial.println("Subnet Mask: " + IP_SUBNET);
              if (gateway.fromString(IP_GATEWAY)) { // check if gateway is correct format
                  Serial.println("Gateway: "+ IP_GATEWAY);
                  bValid = true; // we have valid config, safe to enable static IP
              } else {
                  Serial.println("Invalid Gateway: " + IP_GATEWAY);
              }               
          } else {
              Serial.println("Invalid Subnet Mask: " + IP_SUBNET);
          }
      } else {
          Serial.println("Invalid IP address: " + IP_ADDR);
      }     
      // Optional DNS address settings
      if (dns.fromString(IP_DNS)) {
          Serial.println("DNS Server: " + IP_DNS);
      } else {
          Serial.println("Invalid DNS Server: " + IP_DNS);
      }
      // Set static IP on Ethernet if capable and Wifi 
      if (bValid){
      #ifdef ETH_CAP
        if(!ETH.config(ip, gateway, subnet, dns)){
            Serial.println("Static IP Failed to configure");
        };
      #endif
        if (!WiFi.config(ip, gateway, subnet, dns)) {
           Serial.println("Static IP Failed to configure");
        }
      }
  }
  
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
  HOST_NAME = H_PRFX + "-" + DEV_NAME;;
  ArduinoOTA.setHostname(HOST_NAME.c_str());
  WiFi.setHostname(HOST_NAME.c_str());
  delay(100);
// #region Print config
  Serial.println("------------------------DEVICE-CONFIG-----------------------------");
  Serial.println("DEV_NAME: " + DEV_NAME);
  Serial.println("SSID: " + storedSSID);
  Serial.println("Password: " + storedPassword);
  Serial.println("nrOfLEDS: " + storedNrofLEDS);
  Serial.println("DMX Address: " + storedDmxAddr);
  Serial.println("16BitMode: " + String(b_16Bit > 0 ? "True" : "False") +" : "+ String(b_16Bit));
  Serial.println("Reverse DMX: " + String(b_reverseArray > 0 ? "True" : "False")+" : "+ String(b_reverseArray));
  Serial.println("storedUniverseStart: " + String(storedUniverseStart));
  Serial.println("storedUniverseEnd: " + String(storedUniverseEnd));
  Serial.println("Silent: " + String(b_silent > 0 ? "True" : "False")+" : "+ String(b_silent));
  Serial.println("EthCap: " + String(led.ethCap));
  Serial.println("DHCP: " + String(b_dhcp > 0 ? "True" : "False")+" : "+ String(b_dhcp));
  Serial.println("Static IP: " + IP_ADDR);
  Serial.println("Static Subnet: " + IP_SUBNET);
  Serial.println("Static Gateway: " + IP_GATEWAY);
  Serial.println("Static DNS: " + IP_DNS);
  Serial.println("FallbackWifi: " + String(b_failover > 0 ? "True" : "False"));
  Serial.print("WiFi Hostname: ");
  Serial.println(WiFi.getHostname());
  Serial.print("mDNS Name: ");
  Serial.println(MDNS.hostname(0));
  Serial.println("------------------------------END--------------------------------");
// #endregion 
  // Initialize LED and effect
  effect.initialize();

  led.initialize(NrOfLeds, DmxAddr);
// #region Networking and webserver setup
  Serial.println("Nework setup start");
  // if ethernet capable device start ethernet
  bool bEth = false;
  #ifdef ETH_CAP
    Serial.println("Ethernet capable.. checking connection");
    if(ETH.linkUp()) {
      Serial.println("Ethernet connected");
      Serial.println(ETH.localIP());
      CURR_IP = String(ETH.localIP());
      bEth = true;
    }
  #endif
    if (!bEth) {
      Serial.println("Ethernet not connected starting wifi");
      // Start Access Point if no stored WiFi credentials
      if (storedSSID.isEmpty() || storedPassword.isEmpty()) {
        startAccessPoint(true);
      } else {
        startAccessPoint(false);
        // Connect to WiFi
        WiFi.begin(storedSSID.c_str(), storedPassword.c_str());
        // Attempt to connect with a timeout
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < wifi_attempts) {
          if (!b_silent > 0)
          {
            led.blink(1,1000);
          }
          delay(1000);
          Serial.println("Connecting to WiFi...");
          attempts++;
          if (led.ethCap){
  #ifdef ETH_CAP
          if(ETH.linkUp()){
            Serial.println("ETH connected, rebooting...");
            ESP.restart();
          }
  #endif
        }
      }
      // If not connected, start Access Point and setup
      if (WiFi.status() != WL_CONNECTED) {
        WiFi.disconnect(false);
        startAccessPoint(true);
      } else {
        Serial.println("Connected to WiFi");
        Serial.println(WiFi.localIP());
        CURR_IP = String(WiFi.localIP());
        Serial.print("WiFi Hostname: ");
        Serial.println(WiFi.getHostname());
      }
    }
  }
  // if connected start connection guard task to ensure we are alwasy able to connect.
  #ifdef ETH_CAP
    if (b_failover > 0)
    {
        xTaskCreatePinnedToCore(conGuardTask, "ConGuardTask", 2048, NULL, 3, NULL, 1);
    }
  #endif
  MDNS.begin(HOST_NAME.c_str());
  MDNS.disableArduino();
  // add Http service to mdns
  MDNS.addService("http", "tcp", 80);
  // Setup webserver if we are not in AP mode
  if (!b_APmode){
    setupWebServer();
  }
  //
  // display led effect when connected
  if (!b_silent > 0)
  {
    led.ledTest();
  }
  led.blankLEDS(170);
  delay(1);
  Serial.println("Nework setup done");
// #endregion
// #region ArtNet init
  // Create FreeRTOS task for ArtNet updates on core 1
  Serial.println("Starting ArtNet");
  xTaskCreatePinnedToCore(artnetTask, "ArtnetTask", 8192, NULL, 1, NULL, 1);
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
  if (storedUniverseStart == storedUniverseEnd){
    // Subscribe callback to the first universe
    artnet.subscribe(storedUniverseStart, callback);
    Serial.println("We only have one universe");
    Serial.println("Add universe listener for " + String(storedUniverseStart));
  }

  // Subscribe callback to the second universe if needed
  if (storedUniverseStart < storedUniverseEnd) {
      artnet.subscribe(storedUniverseStart, callback1);
      Serial.println("Add universe listener for " + String(storedUniverseStart));
  }

  // Subscribe callback2 to the third universe
  if (storedUniverseStart + 1 <= storedUniverseEnd) {
      artnet.subscribe(storedUniverseStart + 1, callback2);
      Serial.println("Add universe listener for " + String(storedUniverseStart + 1) + " for callback2");
  }
   // Subscribe callback3 to the third universe
  if (storedUniverseStart + 2 <= storedUniverseEnd) {
      artnet.subscribe(storedUniverseStart + 2, callback3);
      Serial.println("Add universe listener for " + String(storedUniverseStart + 1) + " for callback3");
  }
   // Subscribe callback4 to the third universe
  if (storedUniverseStart + 3 <= storedUniverseEnd) {
      artnet.subscribe(storedUniverseStart + 3, callback4);
      Serial.println("Add universe listener for " + String(storedUniverseStart + 1) + " for callback4");
  }

  // Subscribe callback2 to the fourth universe
  //if (storedUniverseStart + 2 <= storedUniverseEnd) {
  //    artnet.subscribe(storedUniverseStart + 2, callback3);
  //    Serial.println("Add universe listener for " + String(storedUniverseStart + 2) + " for callback2");
  //}
// #endregion

  // Begin OTA update
  ArduinoOTA.begin();

  xTaskCreatePinnedToCore(otaTask, "OtaTask", 8192, NULL, 2, NULL, 1);
  Serial.println("Setup complete");
  applyLEDEffectFromFile(String("boot.fx"));
  
#ifdef RST_BTN
 digitalWrite(sLED, LOW);
#endif
}



// Main loop does nothing
void loop() {
  // while (!diag){
  //    artnet.parse();
  // set onboard leds
  // if (u1rec == 1 && u2rec ==1 )
  // {
  //   u1rec =0;
  //   u2rec =0;
  // }
#ifdef RST_BTN
  digitalWrite(rLED, HIGH);
#endif
  delay(200);
#ifdef RST_BTN
  digitalWrite(rLED, LOW);

  pinMode(led.reset_Button, INPUT_PULLUP);
   
   if (digitalRead(led.reset_Button) == LOW)
   {
      Serial.println("Reset pressed.. rebooting");         
      digitalWrite(rLED, HIGH);
      digitalWrite(sLED, LOW);
      ESP.restart();
   }
   delay(100);

  
#endif
}
