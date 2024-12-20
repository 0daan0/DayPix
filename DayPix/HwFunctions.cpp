#include <esp_task_wdt.h>
#include <SPIFFS.h>
#include <FS.h>
#include <EEPROM.h>
#include "config.h"
#include "WebServerFunctions.h"
//#include "Orbitron_font.h"


// reset watchdog timer
void rstWdt(){
   esp_task_wdt_reset();
}

void reset(){
   ESP.restart();
}
void logToFile(String message){
  File logFile = SPIFFS.open("/system.log", FILE_WRITE, true);
  if (!logFile) {
    Serial.println("Failed to open log file for writing");
    return;
  }
  logFile.println(message);
}
void writeConfigFile() {
  File configFile = SPIFFS.open("/config.cfg", FILE_WRITE, true);
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return;
  }

  // Write configuration data to the file
  configFile.println(String("SSID=") + getStoredString(SSID_EEPROM_ADDR));
  configFile.println(String("Password=") +  getStoredString(PASS_EEPROM_ADDR));
  configFile.println(String("DeviceName=") + getStoredString(DEV_NAME_EEPROM_ADDR));
  configFile.println(String("Universe=") + getStoredString(UNIVERSE_EEPROM_ADDR));
  configFile.println(String("DMXAddr=") + getStoredString(DMX_ADDR_EEPROM_ADDR));
  configFile.println(String("NrLeds=") + getStoredString(NRLEDS_EEPROM_ADDR));
  configFile.println(String("UniverseStart=") + getStoredString(UNIVERSE_START_EEPROM_ADDR));
  configFile.println(String("UniverseEnd=") + getStoredString(UNIVERSE_END_EEPROM_ADDR));
  configFile.println(String("16bit=") + getStoredString(B_16BIT_EEPROM_ADDR));
  configFile.println(String("Failover=") + getStoredString(B_FAILOVER_EEPROM_ADDR));
  configFile.println(String("Silent=") + getStoredString(B_SILENT_EEPROM_ADDR));
  configFile.println(String("Reverse=") + getStoredString(B_REVERSE_ARRAY_EEPROM_ADDR));
  configFile.println(String("DHCP=") + getStoredString(B_DHCP_EEPROM_ADDR));
  configFile.println(String("IP=") + getStoredString(IP_ADDR_EEPROM_ADDR));
  configFile.println(String("Subnet=") + getStoredString(IP_SUBNET_EEPROM_ADDR));
  configFile.println(String("Gateway=") + getStoredString(IP_GATEWAY_EEPROM_ADDR));
  configFile.println(String("DNS=") + getStoredString(IP_DNS_EEPROM_ADDR));
  configFile.println(String("FMversion=") + FIRMWARE_VERSION); 

  // TODO: extend with all config
  configFile.close();
  Serial.println("Config file written successfully");
}

void resetToDefault(){
  Serial.println("Backup config to filesystem before reset to default");
  writeConfigFile();

  storeString(SSID_EEPROM_ADDR, "");
  storeString(PASS_EEPROM_ADDR, "");
  storeString(UNIVERSE_EEPROM_ADDR, "0");
  storeString(DMX_ADDR_EEPROM_ADDR, "0");
  storeString(NRLEDS_EEPROM_ADDR, "13");
  storeString(DEV_NAME_EEPROM_ADDR, "");
  storeString(UNIVERSE_START_EEPROM_ADDR, "0");
  storeString(UNIVERSE_END_EEPROM_ADDR, "0");

  storeString(B_16BIT_EEPROM_ADDR, String(0));
  storeString(B_FAILOVER_EEPROM_ADDR, String(1));
  storeString(B_SILENT_EEPROM_ADDR, String(0));
  storeString(B_REVERSE_ARRAY_EEPROM_ADDR, String(0));

  storeString(B_DHCP_EEPROM_ADDR, String(0));
  storeString(IP_ADDR_EEPROM_ADDR, "2.0.0.1");
  storeString(IP_SUBNET_EEPROM_ADDR, "255.0.0.0");
  storeString(IP_GATEWAY_EEPROM_ADDR, "0.0.0.0");
  storeString(IP_DNS_EEPROM_ADDR, "0.0.0.0");


  // if (!SPIFFS.exists("/font.ttf")) {
  //   Serial.println("Font not found, will create new file");
  //   File file = SPIFFS.open("/font.ttf", FILE_WRITE, true);
  //   file.write(orbitron_variablefont_wght, orbitron_variablefont_wghtcnt);
  //   file.close();

  //   Serial.println("Font file written to filesystem");
  // }
  reset();
}
// Function to initialize EEPROM if not already initialized
void initializeEEPROM() {
  // Read the value at the initialization address
  byte marker = EEPROM.read(INIT_MARKER_ADDR);
  // Check if the read value matches the marker value
  if (marker != INIT_MARKER) {
    Serial.println("EEPROM: Flash not initialized, initializing flash...");
    // EEPROM is not initialized, write the marker value
     for (int i = 0; i < EEPROM_SIZE; i++) {
      EEPROM.write(i, 0x00); // Write 0xFF to each address in the EEPROM
      }
    EEPROM.write(INIT_MARKER_ADDR, INIT_MARKER);
    // Commit the changes to EEPROM
  EEPROM.commit(); 
  resetToDefault();
  }
}



void setupHw(){
// Start the filesystem
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }
}




void listFiles(fs::FS &fs, const char *dirname) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }

  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
  Serial.println();
}