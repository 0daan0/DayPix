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
void resetToDefault(){
  storeString(B_16BIT_EEPROM_ADDR, String(b_16Bit));
  storeString(B_FAILOVER_EEPROM_ADDR, String(b_failover));
  storeString(B_SILENT_EEPROM_ADDR, String(b_silent));
  storeString(B_REVERSE_ARRAY_EEPROM_ADDR, String(b_reverseArray));

  storeString(B_DHCP_EEPROM_ADDR, String(b_dhcp));
  storeString(IP_ADDR_EEPROM_ADDR, IP_ADDR);
  storeString(IP_SUBNET_EEPROM_ADDR, IP_SUBNET);
  storeString(IP_GATEWAY_EEPROM_ADDR, IP_GATEWAY);
  storeString(IP_DNS_EEPROM_ADDR, IP_DNS);
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