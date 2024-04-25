#include <esp_task_wdt.h>
#include <SPIFFS.h>
#include <FS.h>
#include <EEPROM.h>
#include "config.h"

// reset watchdog timer
void rstWdt(){
   esp_task_wdt_reset();
}

// Function to initialize EEPROM if not already initialized
void initializeEEPROM() {
  // Read the value at the initialization address
  byte marker = EEPROM.read(INIT_MARKER_ADDR);
  // Check if the read value matches the marker value
  if (marker != INIT_MARKER) {
    // EEPROM is not initialized, write the marker value
    EEPROM.write(INIT_MARKER_ADDR, INIT_MARKER);
    // Commit the changes to EEPROM
    EEPROM.commit();
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