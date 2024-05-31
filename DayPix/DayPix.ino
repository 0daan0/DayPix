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

#ifdef ETH_CAP
#include <ArtnetETH.h>
ArtnetReceiver artnet;
#else

#include <ArtnetWiFi.h>
ArtnetWiFiReceiver artnet;
#endif
// DMX array data
const uint8_t* bdata;
uint16_t bsize;

const uint8_t* bdata2;
uint16_t bsize2;


ledDriver led;
RGBEffects effect;

void callback(const uint8_t* data, const uint16_t size) {    
  if (recvUniverse.indexOf(String(artnet.universe())) == -1) {
      // The recvUniverse does not contain artnet.universe(), so add it
      recvUniverse += "U" + String(artnet.universe()) + "U";
    }
  led.writePixelBuffer(data, size, NrOfLeds, DmxAddr, 0);
}

void callback1(const uint8_t* data, const uint16_t size) {
 if (recvUniverse.indexOf(String(artnet.universe())) == -1) {
      recvUniverse += "U" + String(artnet.universe()) + "U";
  }
  led.writePixelBuffer(data, size, NrOfLeds, DmxAddr, 0);
}

void callback2(const uint8_t* data, const uint16_t size) {
  if (recvUniverse.indexOf(String(artnet.universe())) == -1) {
      recvUniverse += "U" + String(artnet.universe()) + "U";
  }
  led.writePixelBufferPort2(data, size, NrOfLeds, DmxAddr, 0);

}
void callback3(const uint8_t* data, const uint16_t size) {
  if (recvUniverse.indexOf(String(artnet.universe())) == -1) {
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


#ifdef ETH_CAP
void conGuardTask(void* parameter) {
  while(1){

    while (WiFi.status() != WL_CONNECTED){
      if (led.ethCap){
        Serial.println("Not Connected to wifi and ethernet .. will reboot");
        if(!ETH.linkUp()){
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
  
  // WHY THIS DELAY?
  delay(500);

  Serial.println("Starting setup...");

  #ifdef RST_BTN
  pinMode(IO5, OUTPUT);
  pinMode(IO17, OUTPUT);
  pinMode(led.reset_Button, INPUT_PULLUP);
  delay(100);
   if (digitalRead(led.reset_Button) == LOW)
   {
    Serial.println("Reset to default pressed");
    digitalWrite(IO5, HIGH);
    resetToDefault();
    delay(1000);
    for (int i = 15; i >= 0; i--) 
    {
    digitalWrite(IO17, HIGH);
    delay(50);
    digitalWrite(IO17, LOW);
    delay(50);
    }
    
   }
  #endif

  // init eeprom
  EEPROM.begin(512);
  initializeEEPROM();

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
  HOST_NAME = H_PRFX + "-" + DEV_NAME;
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
  if (led.ethCap) {
  #ifdef ETH_CAP
    Serial.println("Ethernet capable.. checking connection");
    if(ETH.linkUp()) {
      Serial.println("Ethernet connected");
      Serial.println(ETH.localIP());
    }
  #endif
  } else {
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
      while (WiFi.status() != WL_CONNECTED && attempts < WIFI_ATTEMPT) {
        if (!b_silent > 0)
        {
          led.blink(1);
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
        //startAccessPoint(true);
      } else {
        Serial.println("Connected to WiFi");
        Serial.println(WiFi.localIP());
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
  // Setup webserver
  setupWebServer();
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
// #endregion

  // Begin OTA update
  ArduinoOTA.begin();

  xTaskCreatePinnedToCore(otaTask, "OtaTask", 8192, NULL, 2, NULL, 1);
  Serial.println("Setup complete");
  
}



// Main loop does nothing
void loop() {
  while (!diag){
     artnet.parse();
  #ifdef RST_BTN
  pinMode(IO5, OUTPUT);
  pinMode(IO17, OUTPUT);
  digitalWrite(IO5, LOW);
  digitalWrite(IO17, HIGH);
   pinMode(led.reset_Button, INPUT_PULLUP);
   if (digitalRead(led.reset_Button) == HIGH)
   {
    Serial.println("Reset to default released");
   }
   else if (digitalRead(led.reset_Button) == LOW)
   {
      digitalWrite(IO5, HIGH);
      digitalWrite(IO17, LOW);
   }
  
  #endif
  }
}
