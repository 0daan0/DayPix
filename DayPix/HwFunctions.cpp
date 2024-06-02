#include <esp_task_wdt.h>
#include <SPIFFS.h>
#include <FS.h>
#include <EEPROM.h>
#include "config.h"
#include "WebServerFunctions.h"

// reset watchdog timer
void rstWdt(){
   esp_task_wdt_reset();
}

void writeConfigFile() {
  File configFile = SPIFFS.open("/config.cfg", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return;
  }

  // Write configuration data to the file
  configFile.println(String("SSID=") + getStoredString(SSID_EEPROM_ADDR));
  configFile.println(String("Password=") +  getStoredString(PASS_EEPROM_ADDR));
  configFile.println(String("DeviceName=") + getStoredString(DEV_NAME_EEPROM_ADDR));
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
  storeString(B_FAILOVER_EEPROM_ADDR, String(0));
  storeString(B_SILENT_EEPROM_ADDR, String(0));
  storeString(B_REVERSE_ARRAY_EEPROM_ADDR, String(0));

  storeString(B_DHCP_EEPROM_ADDR, String(1));
  storeString(IP_ADDR_EEPROM_ADDR, "192.168.0.10");
  storeString(IP_SUBNET_EEPROM_ADDR, "255.255.255.0");
  storeString(IP_GATEWAY_EEPROM_ADDR, "0.0.0.0");
  storeString(IP_DNS_EEPROM_ADDR, "0.0.0.0");
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


void reset(){
   ESP.restart();
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