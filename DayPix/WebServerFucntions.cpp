// WebServerFunctions.cpp

#include "WebServerFunctions.h"
#include "Config.h"
#include "LedDriver.h"  // Include this line
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include "rgb_effects.h"  // Include any other necessary headers
#include <DNSServer.h>
#include <AsyncJson.h>
#include "HwFunctions.h"
#include <SPIFFS.h>
#include <FS.h> 
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

//ledDriver ledDriverInstance; 
AsyncWebServer server(80);
RGBEffects effects;

// Function to determine the MIME type based on the file extension
String getContentType(String filename) {
    if (filename.endsWith(".htm") || filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) return "image/jpeg";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".gif")) return "image/gif";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".xml")) return "text/xml";
    else if (filename.endsWith(".pdf")) return "application/pdf";
    else if (filename.endsWith(".zip")) return "application/zip";
    else if (filename.endsWith(".gz")) return "application/x-gzip";
    return "text/plain";
}

void loginUser(AsyncWebServerRequest* request)
{
  if (!b_APmode)
  {
    if (!request->authenticate(www_username, www_password)) {
      return request->requestAuthentication();
    }
  }
}

void applyLEDEffectFromFile(const String& effectFile) {
    //char *filenameCopy = strdup(effectFile.c_str());
    Serial.println("trying to apply effect"+ effectFile);
    File file = SPIFFS.open("/" + effectFile, "r");
    if (!file) {
        Serial.println("Failed to open file for reading"+ effectFile);
        return;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);  // Adjust delay as needed
     // Read the file and apply the LED effects
        bool configRead = false; // Flag to track if configuration is read
        int nrLeds = 1; // Default value for nrLeds
        int fx_repeat = 1; // Default value for fx_repeat
        
        while (fx_repeat > 0) {
            // Reset file pointer to the beginning of the file for each repetition
            file.seek(0);
            
            while (file.available()) {
                String line = file.readStringUntil('\n');
                line.trim();
                if (line.length() == 0) continue; // Skip empty lines

                // If configuration is not yet read, parse the configuration
                if (!configRead && line.startsWith("nrLeds=") && line.indexOf(", fx_repeat=") != -1) {
                    sscanf(line.c_str(), "nrLeds=%d, fx_repeat=%d", &nrLeds, &fx_repeat);
                    configRead = true;
                    continue; // Skip processing configuration line
                }

                // Process LED effect lines
                int r, g, b, delayTime;
                sscanf(line.c_str(), "%d,%d,%d,%d", &r, &g, &b, &delayTime);

                led.setLEDColor(r, g, b, nrLeds);
                delay(delayTime);
                rstWdt();
            }
            fx_repeat--;
        }
        led.blankLEDS(170);
         file.close();

}

void applyLEDEffectFromFileTask(void *parameter) {
    char *filename = (char*)parameter;
    applyLEDEffectFromFile(String(filename));
    free(filename); // Free the allocated memory for the filename
    vTaskDelete(NULL); // Delete this task when done
}

void setupWebServer() {
  if (!SPIFFS.exists("/style.css")) {
    Serial.println("Style not found, will create new file");
    File style_css = SPIFFS.open("/style.css", FILE_WRITE, true);

     String css_Style = 
      "body { font-family: Arial, sans-serif; background-color: #343541; color: #fff; font-size: 18px; }"
      "h1 { text-align: center; border: 3px solid; max-width: 90%; margin: 0 auto; background-image: linear-gradient(to right, violet, indigo, blue, green, yellow, orange, red); padding: 2em; color: #EFEFE0; text-shadow: -1px -1px 0 #000, 1px -1px 0 #000, -1px 1px 0 #000, 1px 1px 0 #000; font-size: 46px; }"
      "h3 { text-align: left; margin-bottom: 1em; font-size: 2.5rem; }"
      "form { max-width: 90%; margin: 0 auto; text-align: left; }"
      "label { display: block; margin-bottom: 1.5em; font-size: 24px; }"
      "input[type='text'], input[type='password'], input[type='file'], select { width: 100%; padding: 1.5em; margin-bottom: 1.5em; box-sizing: border-box; background-color: #343541; color: #EFEFE0; border: 1px solid #EFEFE0; border-radius: 1em; font-size: 2rem; }"
      "input[type='submit'], input[type='button'] { width: 100%; padding: 2em; margin-top: 2em; background-color: #4CAF50; color: white; border: none; border-radius: 1em; cursor: pointer; font-size: 1.5rem; }"
      "input[type='submit']:hover, input[type='button']:hover { background-color: #45a049; }"
      "select { width: 100%; padding: 0.8em; margin-bottom: 1em; box-sizing: border-box; background-color: #343541; color: #EFEFE0; border: 1px solid #EFEFE0; font-size: 1.5rem; }"
      "p { text-align: center; font-size: 1.5rem; margin-top: 0.5em; }"
      "#signalStrength { margin-bottom: 1em; }"
      ".switch { position: relative; display: inline-block; width: 4em; height: 2.25em; }"
      ".switch input { display: none; }"
      ".slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; }"
      ".slider:before { position: absolute; content: ''; height: 1.625em; width: 1.625em; left: 0.25em; bottom: 0.25em; background-color: white; transition: .4s; }"
      "input:checked + .slider { background-color: #2196F3; }"
      "input:focus + .slider { box-shadow: 0 0 1px #2196F3; }"
      "input:checked + .slider:before { transform: translateX(1.625em); }"
      ".slider.round { border-radius: 2.125em; }"
      ".slider.round:before { border-radius: 50%; }"
      "input[type='checkbox'] { width: 2.5em; height: 2.5em; }";

    if (style_css.print(css_Style)) {
      Serial.println("Successfully wrote style to style.css");
    } else {
      Serial.println("ERROR: Writing to style.css failed");
    }
  }
  else {
       Serial.println("ERROR:mounting SPIFFS");
  }
    // Set up endpoints for web server
  server.on("/diagnostic", HTTP_GET, handleDiagnostic);
  server.on("/8bitTest", HTTP_POST, handle8BitTest);
  server.on("/rainbow", HTTP_POST, handleRainbow);
  server.on("/16bitTest", HTTP_POST, handle16BitTest);
  server.on("/blankLEDSTest", HTTP_POST, handleBlankLEDSTest);
  server.on("/file", HTTP_GET, [](AsyncWebServerRequest *request) {
      // Get the value of the filename parameter from the request URL
      if (request->hasParam("filename")) {
          String filename = request->getParam("filename")->value();

          // Check if the file exists in SPIFFS
          if (SPIFFS.exists("/" + filename)) {
              // If the file exists, send it to the client with the appropriate MIME type
              request->send(SPIFFS, "/" + filename, getContentType(filename));
          } else {
              // If the file does not exist, send a 404 Not Found response
              request->send(404, "text/plain", "File not found");
          }
      } else {
          // If the filename parameter is missing, send a 400 Bad Request response
          request->send(400, "text/plain", "Missing filename parameter");
      }
  });

 server.on("/fs", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Check if the user is logged in
   loginUser(request);
        // If not logged in, redirect to login page or send unauthorized response
  
  // Extract file name from request URL for deletion
    String filename;
    if (request->hasArg("delete")) {
        filename = request->arg("delete");
    }

    // Handle file deletion request
    if (!filename.isEmpty()) {
        if (SPIFFS.exists("/"+filename)) {
             Serial.println("Try to delete"+filename);
            if (SPIFFS.remove("/"+filename)) {
                request->send(200, "text/plain", "File deleted successfully");
            } else {
                request->send(500, "text/plain", "Failed to delete file");
            }
            return;
        } else {
            request->send(404, "text/plain", "File not found");
            return;
        }
    }

    // Check if the format button is clicked
    if (request->hasArg("format")) {
        // Format the file system
        if (SPIFFS.format()) {
            request->send(200, "text/plain", "File system formatted successfully");
        } else {
            request->send(500, "text/plain", "Failed to format file system");
        }
        return;
    }

    // Open SPIFFS directory
    String fileListing;

    File root = SPIFFS.open("/");
    if (!root) {
        Serial.println("Failed to open directory");
        request->send(500, "text/plain", "Failed to open directory");
        return;
    }
    // Calculate total, used, and free space in kilobytes
    size_t totalKB = SPIFFS.totalBytes() / 1024;
    size_t usedKB = SPIFFS.usedBytes() / 1024;
    size_t freeKB = totalKB - usedKB;

    // Generate file listing
    fileListing = "";
    File file = root.openNextFile();
    while (file) {
        fileListing += "<a href='/file?filename=" + String(file.name()) + "'>" + String(file.name()) + "</a>";
        fileListing += " <a href='/fs?delete=" + String(file.name()) + "'>Delete</a><br>";
        file = root.openNextFile();
    }
    root.close();
  String css = "<html><head><style>"
                    "body { font-family: Arial, sans-serif; background-color: #343541; color: #fff; padding: 10px; }"
                    "h1 { text-align: center; border: 3px solid; max-width: 90%; margin: 0 auto; background-image: linear-gradient(to right, violet, indigo, blue, green, yellow, orange, red); padding: 30px; color: #EFEFE0; text-shadow: -1px -1px 0 #000, 1px -1px 0 #000, -1px 1px 0 #000, 1px 1px 0 #000; font-size: 36px; }"
                    "form { max-width: 90%; margin: 0 auto; }"
                    "label { display: block; margin-bottom: 30px; font-size: 24px; }"
                    "input[type='range'] { width: 100%; height: 80px; padding: 30px; margin-bottom: 30px; box-sizing: border-box; background-color: #343541; color: #EFEFE0; border: 2px solid #EFEFE0; border-radius: 20px; -webkit-appearance: none; }"
                    "input[type='range']::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 60px; height: 60px; background: #4CAF50; border-radius: 50%; cursor: pointer; }"
                    "input[type='submit'], input[type='button'] { width: 100%; padding: 40px; margin-top: 30px; background-color: #4CAF50; color: white; border: none; border-radius: 20px; cursor: pointer; font-size: 28px; }"
                    "input[type='submit']:hover, input[type='button']:hover { background-color: #45a049; }"
                    "a {color: pink;}"
                    "a:hover {color: white}"
                    "</style></head><body>";

    // Generate response with file upload form, file listing, and space information in KB
    String response = "<h2>File Upload</h2>"
                      "<form method='POST' action='/upload' enctype='multipart/form-data'>"
                      "<input type='file' name='upload'>"
                      "<input type='submit' value='Upload'>"
                      "</form>"
                      "<h2>Existing Files</h2>"
                      "<pre>" + fileListing + "</pre>"
                      "<h2>Free Space</h2>"
                      "<p>Total KB: " + String(totalKB) + "</p>"
                      "<p>Used KB: " + String(usedKB) + "</p>"
                      "<p>Free KB: " + String(freeKB) + "</p>"
                      "<h2>Format File System</h2>"
                      "<form method='POST' action='/fs'>"
                      "<input type='hidden' name='format' value='true'>"
                      "<input type='submit' value='Format FS'>"
                      "</form>";

    // Send response to client
    request->send(200, "text/html", css+response);
    });

 // Handle file upload
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {},
  [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    static File uploadFile;
    if (index == 0) { // Start of upload
      Serial.printf("Upload Start: %s\n", filename.c_str());
      uploadFile = SPIFFS.open("/" + filename, FILE_WRITE);
      if (!uploadFile) {
        Serial.println("Failed to open file for writing");
        request->send(500, "text/plain", "Failed to open file for writing");
        return;
      }
    }
    if (uploadFile) {
      if (uploadFile.write(data, len) != len) {
        Serial.println("Write failed");
        uploadFile.close();
        request->send(500, "text/plain", "Write failed");
        return;
      }
    }
    if (final) { // End of upload
      if (uploadFile) {
        uploadFile.close();
        Serial.printf("Upload End: %s\n", filename.c_str());
        request->send(200, "text/plain", "File uploaded successfully");
      } else {
        Serial.println("Failed to close file");
        request->send(500, "text/plain", "Failed to close file");
      }
    }
  });


 // Serve static files
  server.serveStatic("/", SPIFFS, "/");

  // List .fx files endpoint
  server.on("/list-files", HTTP_GET, [](AsyncWebServerRequest *request){
    String fileList = "[";
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    bool firstFile = true;
    while (file) {
      if (String(file.name()).endsWith(".fx")) {
        if (!firstFile) fileList += ",";
        fileList += "\"" + String(file.name()) + "\"";
        firstFile = false;
      }
      file = root.openNextFile();
    }
    fileList += "]";
    request->send(200, "application/json", fileList);
  });

  // Read selected file content
  server.on("/read-file", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("file")) {
      String fileName = request->getParam("file")->value();
      File file = SPIFFS.open("/" + fileName);
      if (file) {
        String fileContent = file.readString();
        request->send(200, "text/plain", fileContent);
        file.close();
      } else {
        request->send(404, "text/plain", "File not found");
      }
    } else {
      request->send(400, "text/plain", "Bad request");
    }
  });

      // Route to handle file upload form and file upload POST request
 
  server.on("/getSignalStrength", HTTP_GET, handleSignalStrength);
  server.on("/identify", HTTP_POST, handleIdentify);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/ledcontrol", HTTP_POST, handleLedControl);
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", "<h1>Updating...</h1>");
  }, handleUpdate);
  server.on("/reboot", HTTP_POST, handleReboot);
  server.on("/setDiag", HTTP_POST, [](AsyncWebServerRequest* request) {
  handleSetDiag(request);

 
  });
server.on("/applyEffect", HTTP_POST, [](AsyncWebServerRequest *request){
   if (request->hasParam("effectFile", true)) {
        String effectFile = request->getParam("effectFile", true)->value();
        Serial.println("Selected effect file: " + effectFile);

       // Allocate memory for the filename and copy it
    char *filenameCopy = strdup(effectFile.c_str());

    // Create the task to run applyLEDEffectFromFile in a separate thread
    xTaskCreatePinnedToCore(
        applyLEDEffectFromFileTask,    // Task function
        "LED Effect Task",             // Name of the task
        2048,                          // Stack size in words
        (void*)filenameCopy,           // Parameter to pass to the task function
        3,                             // Priority of the task
        NULL,                          // Task handle
        1                              // Core to run the task on (0 or 1)
    );
      //xTaskCreatePinnedToCore(applyLEDEffectFromFile, "LED Effect Task", 2048, (void*)&filenameCopy, 3, NULL, 1);
      //xTaskCreate(applyLEDEffectFromFile,"LED Effect Task",8192,(void*)&filenameCopy,1,NULL);
    } else {
        request->send(400, "text/plain", "No effect file selected");
    }

    // Respond to the request
   
});



server.on("/ledcontrol", HTTP_GET, [](AsyncWebServerRequest *request){
    // HTML for the LED control page
    String html = "<html><head><style>"
                  "body { font-family: Arial, sans-serif; background-color: #343541; color: #fff; padding: 10px; background-image: url(/file?filename=bg.gif); background-size: cover; }" 
                  "h1 { text-align: center; border: 3px solid; max-width: 90%; margin: 0 auto; background-image: linear-gradient(to right, violet, indigo, blue, green, yellow, orange, red); padding: 30px; color: #EFEFE0; text-shadow: -1px -1px 0 #000, 1px -1px 0 #000, -1px 1px 0 #000, 1px 1px 0 #000; font-size: 36px; }"
                  "form { max-width: 90%; margin: 0 auto; }"
                  "label { display: block; margin-bottom: 30px; font-size: 24px; }"
                  "input[type='range'] { width: 100%; height: 80px; padding: 30px; margin-bottom: 30px; box-sizing: border-box; background-color: rgba(52, 53, 65, 0.7); color: #EFEFE0; border: 2px solid #EFEFE0; border-radius: 20px; -webkit-appearance: none; }"
                  "input[type='range']::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 60px; height: 60px; background: #4CAF50; border-radius: 50%; cursor: pointer; }"
                  "input[type='submit'], input[type='button'], select { width: 100%; padding: 40px; margin-top: 30px; background-color: rgba(76, 175, 80, 0.7); color: white; border: none; border-radius: 20px; cursor: pointer; font-size: 28px; opacity: 0.7; }"
                  "input[type='submit']:hover, input[type='button']:hover, select:hover { background-color: #45a049; opacity: 1; }"
                   "input[type='submit'], input[type='button'] { width: 100%; padding: 40px; margin-top: 30px; background-color: rgba(76, 175, 80, 0.7); color: white; border: none; border-radius: 20px; cursor: pointer; font-size: 28px; opacity: 0.7; }" // Added opacity to buttons
                  "input[type='submit']:hover, input[type='button']:hover { background-color: #45a049; opacity: 1; }" // Adjusted opacity on hover
                  "</style>"
                  "<script>"
                  "let bgToggle = false;"
                  "document.addEventListener('click', function () {"
                  "    bgToggle = !bgToggle;"
                  "    if (bgToggle) {"
                  "        document.body.style.backgroundImage = 'url(/file?filename=bg.gif)';"
                  "    } else {"
                  "        document.body.style.backgroundImage = 'url(/file?filename=bg2.gif)';"
                  "    }"
                  "});"
                  "window.addEventListener('beforeunload', function (event) {"
                  "    event.preventDefault();"
                  "    event.returnValue = '';"
                  "});"
                  "</script>"
                  "</head><body>";
                  "</style></head><body>";
    
    html += "<h1>DayPix Control</h1>";
    html += "<form id='ledForm'>";
    html += "<label for='redSlider' style='font-size: 2.5rem;'>Red:</label><br>";
    html += "<input type='range' id='redSlider' name='red' min='0' max='255' value='0'><br>";
    html += "<label for='greenSlider' style='font-size: 2.5rem;'>Green:</label><br>";
    html += "<input type='range' id='greenSlider' name='green' min='0' max='255' value='0'><br>";
    html += "<label for='blueSlider' style='font-size: 2.5rem;'>Blue:</label><br>";
    html += "<input type='range' id='blueSlider' name='blue' min='0' max='255' value='0'><br>";
    //html += "<input type='button' value='Rainbow Chase' onclick='runRainbowChase()' style='margin-top: 30px;'>";
    html += "</form>";
    

    // Dropdown for effect files
    html += "<form id='effectForm' method='post'>";
    html += "<label for='effectSelect' style='font-size: 2.5rem;'>Select Effect:</label><br>";
    html += "<select id='effectSelect' name='effectFile'>";
    
    // Open SPIFFS directory
    String fileListing;
    File root = SPIFFS.open("/");
    if (!root) {
        Serial.println("Failed to open directory");
        request->send(500, "text/plain", "Failed to open directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory() && String(file.name()).endsWith(".fx")) {
            Serial.println("Found .fx file: " + String(file.name()));
            fileListing += "<option value='" + String(file.name()) + "'>" + String(file.name()) + "</option>";
        }
        file = root.openNextFile();
    }

    root.close();
    
    html += fileListing;
    html += "</select>";
    html += "<input type='submit' value='Apply Effect'>";
    html += "</form>";

    html += "<form id='Config' method='post'>";
    html += "<input type='button' value='Config' onclick='window.location.href=\"/\"' style='margin-top: 30px;'>";
    html += "</form>";

    html += "<script>";
    html += "const redSlider = document.getElementById('redSlider');";
    html += "const greenSlider = document.getElementById('greenSlider');";
    html += "const blueSlider = document.getElementById('blueSlider');";
    
    html += "function updateLed() {";
    html += "  const red = redSlider.value;";
    html += "  const green = greenSlider.value;";
    html += "  const blue = blueSlider.value;";
    html += "  fetch('/ledcontrol', {";
    html += "    method: 'POST',";
    html += "    headers: {";
    html += "      'Content-Type': 'application/x-www-form-urlencoded'";
    html += "    },";
    html += "    body: 'plain=' + encodeURIComponent('red=' + red + '&green=' + green + '&blue=' + blue)";
    html += "  })";
    html += "  .then(response => {";
    html += "    if (response.ok) {";
    html += "      console.log('LEDs updated successfully');";
    html += "    } else {";
    html += "      console.error('Failed to update LEDs');";
    html += "    }";
    html += "  })";
    html += "  .catch(error => {";
    html += "    console.error('Error:', error);";
    html += "  });";
    html += "}";
    
    html += "function runRainbowChase() {";
    html += "  fetch('/rainbow', {";
    html += "    method: 'POST',";
    html += "    headers: {";
    html += "      'Content-Type': 'application/x-www-form-urlencoded'";
    html += "    }";
    html += "  })";
    html += "  .then(response => {";
    html += "    if (response.ok) {";
    html += "      console.log('Rainbow chase started successfully');";
    html += "    } else {";
    html += "      console.error('Failed to start rainbow chase');";
    html += "    }";
    html += "  })";
    html += "  .catch(error => {";
    html += "    console.error('Error:', error);";
    html += "  });";
    html += "}";

    html += "document.getElementById('effectForm').addEventListener('submit', function(event) {";
    html += "  event.preventDefault();"; // Prevent default form submission behavior
    html += "  applyEffect();"; // Call function to apply effect
    html += "});";

    html += "function applyEffect() {";
    html += "  const effectSelect = document.getElementById('effectSelect');";
    html += "  const selectedEffect = effectSelect.value;";
    html += "  fetch('/applyEffect', {";
    html += "    method: 'POST',";
    html += "    headers: {";
    html += "      'Content-Type': 'application/x-www-form-urlencoded'";
    html += "    },";
    html += "    body: 'effectFile=' + encodeURIComponent(selectedEffect)";
    html += "  })";
    html += "  .then(response => {";
    html += "    if (response.ok) {";
    html += "      console.log('Effect applied successfully');";
    html += "    } else {";
    html += "      console.error('Failed to apply effect');";
    html += "    }";
    html += "  })";
    html += "  .catch(error => {";
    html += "    console.error('Error:', error);";
    html += "  });";
    html += "}";
    
    html += "redSlider.addEventListener('input', updateLed);";
    html += "greenSlider.addEventListener('input', updateLed);";
    html += "blueSlider.addEventListener('input', updateLed);";
    html += "</script>";
    
    html += "</body></html>";
    request->send(200, "text/html", html);
});
  server.begin();
}




// Definition of handleFileUpload function
void handleFileUpload(AsyncWebServerRequest *request) {
   Serial.println("File:UPLOAD");
    AsyncWebServerResponse *response;
    if(request->hasParam("file", true, true)) {
        AsyncWebParameter* file = request->getParam("file", true, true);
        String filename = file->name(); // Use name() instead of filename()
        Serial.println("File:"+filename+ "Recived");
        File f = SPIFFS.open("/" + filename, FILE_WRITE, true);
        if(f) {
            f.write((const uint8_t*)file->value().c_str(), file->value().length());
            f.close();
            response = request->beginResponse(200, "text/plain", "File uploaded successfully");
        } else {
            response = request->beginResponse(500, "text/plain", "Failed to save file to filesystem");
        }
    } else {
        response = request->beginResponse(400, "text/plain", "No file uploaded");
    }
    request->send(response);
}
void handleSetDiag(AsyncWebServerRequest* request) {
  if (request->hasParam("diag", true)) {
    String diagValue = request->getParam("diag", true)->value();
    if (diagValue == "1") {
      setDiag(true);
    } else {
      setDiag(false);
    }
  }
  request->send(200, "text/plain", "Diagnostic flag updated");
}

void handleLedControl(AsyncWebServerRequest* request) {
 
  // Check if the request has a body
  if (request->hasArg("plain")) {
      // Read the body of the request
      String body = request->arg("plain");
      
      // Parse the body to extract red, green, and blue values
      int red = 0, green = 0, blue = 0;
      sscanf(body.c_str(), "red=%d&green=%d&blue=%d", &red, &green, &blue);
      
      // Here you can use the RGB values to control your LEDs or perform any desired action
      // For example, you could use these values to adjust the brightness of RGB LEDs
      //Serial.printf("Received RGB values - Red: %d, Green: %d, Blue: %d\n", red, green, blue);
      led.setLEDColor(red, green, blue);
      
      // Respond with a success message
      request->send(200, "text/plain", "LEDs updated successfully");
  } else {
      // If the request body is missing, respond with an error message
      request->send(400, "text/plain", "Missing request body");
  }
}

void handleRoot(AsyncWebServerRequest* request) {
  loginUser(request);

  String html = "<!DOCTYPE html><html><head>";
  html += "<link rel='stylesheet' type='text/css' href='/style.css'>";
  html += "<script>";
  html += "function toggleSwitch(element) {";
  html += "  element.checked = !element.checked;";
  html += "}";
  html += "</script>";
  html += "</head><body>";

  // Title header 1
  html += "<h1>" + String(H_PRFX) + " Config</h1>";

  // Main form for wifi config input
  html += "<form action='/save' method='post'>";
  html += "<h3> Device/Network Settings </h3>";
  html += "Device Name    : <input type='text' name='devicename' value='" + DEV_NAME + "'><br>";
  html += "WiFi SSID    : <input type='text' name='ssid' value='" + String(getStoredString(SSID_EEPROM_ADDR)) + "'><br>";
  html += "WiFi Password: <input type='password' name='password' value='" + String(getStoredString(PASS_EEPROM_ADDR)) + "'><br>";

  if (led.ethCap) {
    html += "Ethernet to Wifi Failover  : <input type='checkbox' name='b_failover' " + String(b_failover ? "checked" : "") + " value='" + String(b_failover) +"'><br>";
    html += "<p>Failover mode will fallback to wifi when Ethernet is disconnected</p>";    
  }

  // IP configuration section
  html += "<h3>IP Configuration</h3>";
  html += "Use DHCP: <input type='checkbox' id='dhcpCheckbox' name='b_dhcp' " + String(b_dhcp ? "checked" : "") + " onclick='toggleDHCP(this)'><br>";
  html += "<div id='manualConfig' style='display: " +  String(b_dhcp ? "none" : "block") + ";'>";
  html += "IP Address: <input type='text' name='ipAddress' value='" +String(getStoredString(IP_ADDR_EEPROM_ADDR)) + "'><br>";
  html += "Subnet Mask: <input type='text' name='subnetMask' value='" + String(getStoredString(IP_SUBNET_EEPROM_ADDR)) + "'><br>";
  html += "Gateway: <input type='text' name='gateway' value='" + String(getStoredString(IP_GATEWAY_EEPROM_ADDR)) + "'><br>";
  html += "DNS Server: <input type='text' name='dnsServer' value='" + String(getStoredString(IP_DNS_EEPROM_ADDR)) + "'><br>";
  html += "</div>";

  html += "<script>";
  html += "document.getElementById('dhcpCheckbox').addEventListener('change', function() {";
  html += "if (this.checked) {";
  html += "document.getElementById('manualConfig').style.display = 'none';";
  html += "} else {";
  html += "document.getElementById('manualConfig').style.display = 'block';";
  html += "}";
  html += "});";
  html += "</script>";

  // Main form for DMX/Artnet config input
  html += "<h3> ArtNet/DMX Settings </h3>";
  html += "Art-Net Universe Start: <select name='universe_start'>";
  for (int i = 0; i <= 15; ++i) {
    html += "<option value='" + String(i) + "' " + (getStoredString(UNIVERSE_START_EEPROM_ADDR).toInt() == i ? "selected" : "") + ">" + String(i) + "</option>";
  }
  html += "</select>";
  html += "Art-Net Universe End: <select name='universe_end'>";
  for (int i = 0; i <= 15; ++i) {
    html += "<option value='" + String(i) + "' " + (getStoredString(UNIVERSE_END_EEPROM_ADDR).toInt() == i ? "selected" : "") + ">" + String(i) + "</option>";
  }
  html += "</select><br>";

  html += "DMX Start Addr  : <select name='DmxAddr'>";
  for (int i = 0; i <= 512; ++i) {
    html += "<option value='" + String(i) + "' " + (getStoredString(DMX_ADDR_EEPROM_ADDR).toInt() == i ? "selected" : "") + ">" + String(i) + "</option>";
  }
  html += "</select>";

  // Calculate DMX address range
  int startAddr = getStoredString(DMX_ADDR_EEPROM_ADDR).toInt();
  int endAddr;
  if (b_16Bit) {
    endAddr = startAddr + (getStoredString(NRLEDS_EEPROM_ADDR).toInt() * 6 );
  } else {
    endAddr = startAddr + (getStoredString(NRLEDS_EEPROM_ADDR).toInt() * 3 );
  }

  html += "<br>";
  html += "Current Address Range: " + String(startAddr+1) + " - " + String(endAddr) + "<br>";

  // Main form for LED config input
  html += "<h3> Pixel Settings </h3>";
  html += "Number of Pixels per universe : <select name='NrofLEDS'>";
  for (int i = 1; i <= 170; ++i) {
    if (b_16Bit && i > 85) {
      break;
    }
    html += "<option value='" + String(i) + "' " + (getStoredString(NRLEDS_EEPROM_ADDR).toInt() == i ? "selected" : "") + ">" + String(i) + "</option>";
  }
  html += "<select><br>";

  html += "16-bit Mode   : <input type='checkbox' name='b_16Bit' " + String(b_16Bit ? "checked" : "") + " value='" + String(b_16Bit) +"'><br>";
  html += "<p>16Bit mode will consume double the DMX channels and limited to 85 LEDS per universe</p>";
  html += "Silent mode : <input type='checkbox' name='b_silent' " + String(b_silent ? "checked" : "") + " value='" + String(b_silent) + "'><br>";
  html += "<p>In silent mode no connection status is given on the LED outputs</p>";
  html += "Reverse DMX addresses : <input type='checkbox' name='b_reverseArray' " + String(b_reverseArray ? "checked" : "") + " value='" + String(b_reverseArray) + "'><br>";
  html += "<p>Reverse DMX addresses will address the pixels in reverse order</p>";

  html += "<p>Normal mode: Lowest address = first LED outwards from the controller</p>";
  html += "<p>Reverse mode: Highest address = first LED outwards from the controller</p>";
  html += "<p>NOTE: Default color space is BGR in reverse order this will be RGB</p>";

  // Add the Identify button with spacing
  html += "<form id='saveForm' action='/save' method='post'>";
  html += "<input type='submit' value='Save' onclick='saveClicked()' style='margin-bottom: 10px; margin-top: 10px;'>";
  html += "</form>";
  html += "<form id='identifyForm' method='post'>";
  html += "<input type='button' value='Identify' onclick='identifyClicked()' style='margin-top: 10px;'>";
  html += "</form>";

  // Add led control button
  html += "<form id='ledControl' method='post'>";
  html += "<input type='button' value='LEDControl' onclick='window.location.href=\"/ledcontrol\"' style='margin-top: 10px;'>";
  html += "</form>";

  // Add reboot button
  html += "<form id='rebootForm' method='post'>";
  html += "<input type='button' value='Reboot' onclick='rebootClicked()' style='margin-top: 10px;'>";
  html += "</form>";

  // Updated firmware version display
  html += "<form method='POST' action='/update' enctype='multipart/form-data' style='margin-top: 20px;'>";
  html += "Firmware Update: <input type='file' name='update'><br><br>";
  html += "<input type='submit' value='Update'>";
  html += "</form>";
  html += "<p>Firmware Version: " + String(FIRMWARE_VERSION) + "</p>";
  html += "<p>HW Version: " + String(ledDriverInstance.hwVersion) + "</p>";
  html += "<p>Daniel Guurink-Hoogerwerf</p>";
  html += "<a href='/diagnostic'>Diagnostics</a><br>";

  // Script for identify button
  html += "<script>";
  html += "function identifyClicked() {";
  html += "var xhttp = new XMLHttpRequest();";
  html += "xhttp.open('POST', '/identify', true);";
  html += "xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');";
  html += "xhttp.send();";
  html += "}";
  html += "function rebootClicked() {";
  html += "var xhttp = new XMLHttpRequest();";
  html += "xhttp.open('POST', '/reboot', true);";
  html += "xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');";
  html += "xhttp.send();";
  html += "}";
  html += "</script>";

  html += "</body></html>";

  request->send(200, "text/html", html);
}


void handleReboot(AsyncWebServerRequest* request) {
  loginUser(request);
  request->send(200, "text/plain", "Rebooting...");
  delay(1000);
  ESP.restart();
}
void handleDiagnostic(AsyncWebServerRequest* request) {
  loginUser(request);

  // Gather diagnostic information
  String diagnosticInfo = "<html><head><style>";
  diagnosticInfo += "body { font-family: Arial, sans-serif; background-color: #343541; color: #fff; }";
  diagnosticInfo += "h1 { text-align: center; border: 3px solid; max-width: 400px; margin: 0 auto; background-image: linear-gradient(to right, violet, indigo, blue, green, yellow, orange, red); padding: 10px; color: #EFEFE0; text-shadow: -1px -1px 0 #000, 1px -1px 0 #000, -1px 1px 0 #000, 1px 1px 0 #000; }";
  diagnosticInfo += "form { max-width: 400px; margin: 0 auto; }";
  diagnosticInfo += "label { display: block; margin-bottom: 5px; }";
  diagnosticInfo += "input[type='range'] { width: 100%; padding: 8px; margin-bottom: 10px; box-sizing: border-box; background-color: #343541; color: #EFEFE0; border: 1px solid #EFEFE0; }";
  diagnosticInfo += "input[type='submit'], input[type='button'] { width: 100%; padding: 10px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer; }";
  diagnosticInfo += "input[type='submit']:hover, input[type='button']:hover { background-color: #45a049; }";
  diagnosticInfo += "p { text-align: center; font-size: 10px; margin-top: 20px; }"; // Updated style for the paragraph
  diagnosticInfo += "#signalStrength { margin-bottom: 10px; }";
  diagnosticInfo += "</style></head><body>";

  // Add diagnostic content
  diagnosticInfo += "<h1>DayPix Diagnostics</h1>";
  diagnosticInfo += "<p>Free Heap: " + String(ESP.getFreeHeap()) + " bytes</p>";
  diagnosticInfo += "<p>Received universe: " + String(recvUniverse) + "</p>";

  diagnosticInfo += "</select><br>";
  diagnosticInfo += "<div id='signalStrength'></div>";
  diagnosticInfo += "<div id='signalMeter' style='max-width: 400px; border: 1px solid black;'></div>";  // Set max width to 400 pixels with a black border
  // Add checkbox for diagnostic flag
  diagnosticInfo += "<form id='diagForm' method='post'>";
  diagnosticInfo += "<label><input type='checkbox' id='diagCheckbox' name='diag' onchange='updateDiagFlag()'> Disable ArtNet</label>";
  diagnosticInfo += "</form>";

  // Add buttons for tests
  diagnosticInfo += "<form id='identifyForm' method='post'>";
  diagnosticInfo += "<input type='button' value='16bitTest' onclick='runTest(\"16bit\")' style='margin-top: 10px;'>";
  diagnosticInfo += "</form>";

  diagnosticInfo += "<form id='identifyForm' method='post'>";
  diagnosticInfo += "<input type='button' value='8bitTest' onclick='runTest(\"8bit\")' style='margin-top: 10px;'>";
  diagnosticInfo += "</form>";

  diagnosticInfo += "<form id='identifyForm' method='post'>";
  diagnosticInfo += "<input type='button' value='blankLEDSTest' onclick='runTest(\"blankLEDS\")' style='margin-top: 10px;'>";
  diagnosticInfo += "</form>";

  diagnosticInfo += "<form id='Config' method='post'>";
  diagnosticInfo += "<input type='button' value='Config' onclick='window.location.href=\"/\"' style='margin-top: 10px;'>";
  diagnosticInfo += "</form>";

  // Include JavaScript for AJAX
  diagnosticInfo += "<script>";
  diagnosticInfo += "function runTest(testType) {";
  diagnosticInfo += "var xhttp = new XMLHttpRequest();";
  diagnosticInfo += "xhttp.open('POST', '/' + testType + 'Test', true);";
  diagnosticInfo += "xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');";
  diagnosticInfo += "xhttp.send();";
  diagnosticInfo += "}";
  diagnosticInfo += "function updateDiagFlag() {";
  diagnosticInfo += "var checkbox = document.getElementById('diagCheckbox');";
  diagnosticInfo += "var diagValue = checkbox.checked;";
  diagnosticInfo += "var xhttp = new XMLHttpRequest();";
  diagnosticInfo += "xhttp.open('POST', '/setDiag', true);";
  diagnosticInfo += "xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');";
  diagnosticInfo += "xhttp.send('diag=' + (diagValue ? '1' : '0'));";
  diagnosticInfo += "}";
  diagnosticInfo += "</script>";
  // Include the JavaScript code for fetching signal strength
  diagnosticInfo += "<script>";
  diagnosticInfo += "function toggleSwitch(element) {";
  diagnosticInfo +=  "element.previousSibling.value = element.checked ? '1' : '0';";
  diagnosticInfo += "}";
  diagnosticInfo += "function updateSignalStrength() {";
  diagnosticInfo += "var xhttp = new XMLHttpRequest();";
  diagnosticInfo += "xhttp.onreadystatechange = function() {";
  diagnosticInfo += "if (this.readyState == 4 && this.status == 200) {";
  diagnosticInfo += "var signalStrength = parseInt(this.responseText);";
  diagnosticInfo += "document.getElementById('signalStrength').innerHTML = 'WiFi Signal Strength: ' + signalStrength + ' dBm';";
  diagnosticInfo += "updateSignalStrengthMeter(signalStrength);";
  diagnosticInfo += "}";
  diagnosticInfo += "};";
  diagnosticInfo += "xhttp.open('GET', '/getSignalStrength', true);";
  diagnosticInfo += "xhttp.send();";
  diagnosticInfo += "}";
  diagnosticInfo += "function updateSignalStrengthMeter(signalStrength) {";
  diagnosticInfo += "var signalMeter = document.getElementById('signalMeter');";
  diagnosticInfo += "var maxSignal = -30;";
  diagnosticInfo += "var minSignal = -90;";
  diagnosticInfo += "var bars;";
  diagnosticInfo += "bars = Math.min(6, Math.max(1, Math.ceil((signalStrength - minSignal) / ((maxSignal - minSignal) / 6))));";
  diagnosticInfo += "var color;";
  diagnosticInfo += "if (bars == 1) {";
  diagnosticInfo += "color = 'red';";
  diagnosticInfo += "} else if (bars <= 3) {";
  diagnosticInfo += "color = 'orange';";
  diagnosticInfo += "} else if (bars <= 4) {";
  diagnosticInfo += "color = 'yellow';";
  diagnosticInfo += "} else {";
  diagnosticInfo += "color = 'green';";
  diagnosticInfo += "}";
  diagnosticInfo += "signalMeter.innerHTML = '';";
  diagnosticInfo += "for (var i = 0; i < bars; i++) {";
  diagnosticInfo += "var bar = document.createElement('div');";
  diagnosticInfo += "bar.style.border = '1px solid black';";
  diagnosticInfo += "bar.style.backgroundColor = color;";
  diagnosticInfo += "bar.style.width = 'calc(100% / 6 - 2px)';";
  diagnosticInfo += "bar.style.height = '30px';"; // Adjusted height to be twice as high
  diagnosticInfo += "bar.style.display = 'inline-block';";
  diagnosticInfo += "signalMeter.appendChild(bar);";
  diagnosticInfo += "}";
  diagnosticInfo += "var labels = ['Weak', 'Good', 'Best'];";
  diagnosticInfo += "var labelContainer = document.getElementById('signalLabels');";
  diagnosticInfo += "labelContainer.innerHTML = '';";
  diagnosticInfo += "for (var i = 0; i < 3; i++) {";
  diagnosticInfo += "var label = document.createElement('div');";
  diagnosticInfo += "label.innerHTML = labels[i];";
  diagnosticInfo += "label.style.width = 'calc(100% / 6 - 4px)';";
  diagnosticInfo += "label.style.textAlign = 'center';";
  diagnosticInfo += "label.style.display = 'inline-block';";
  diagnosticInfo += "labelContainer.appendChild(label);";
  diagnosticInfo += "}";
  diagnosticInfo += "}";
  diagnosticInfo += "setInterval(updateSignalStrength, 5000);";
  diagnosticInfo += "updateSignalStrength();";
  diagnosticInfo += "</script>";
  // Send the diagnostic information as HTML
  request->send(200, "text/html", diagnosticInfo);
}


void handle8BitTest(AsyncWebServerRequest* request) {
  loginUser(request);

  Serial.println("Running 8-bit test...");
  //led.rgbEffect(reinterpret_cast<void*>(8));
  led.ledOn();
  //delay(5000);  // Adjust delay as needed
  diag = true;
  led.blankLEDS(170);
  led._8bTest();
  Serial.println("8-bit test completed");
  request->send(200, "text/plain", "8-bit test completed!");
}

void handleRainbow(AsyncWebServerRequest* request) {
  //effects.rainbowChase16bit(10);
  request->send(200, "text/plain", "rainbow chase started");
  effects.colorPulseBreathing(200,  20);
}

void handleBlankLEDSTest(AsyncWebServerRequest* request) {
  loginUser(request);
  Serial.println("Running blankLEDS test...");
  led.blankLEDS(170);
}

void handle16BitTest(AsyncWebServerRequest* request) {
  loginUser(request);
  Serial.println("Running 16-bit test...");
   diag = true;
  //led.rgbEffect(reinterpret_cast<void*>(16));
  led.ledOn();
  //delay(5000);  // Adjust delay as needed
  led.blankLEDS(170);
  led._16bTest();
  Serial.println("16-bit test completed");
  request->send(200, "text/plain", "16-bit test completed!");
}

void handleSignalStrength(AsyncWebServerRequest* request) {
  String signalStrength = String(WiFi.RSSI());
  request->send(200, "text/plain", signalStrength);
}

void handleSave(AsyncWebServerRequest* request) {
  loginUser(request);
  // Get values from webinterface
  String ssid = request->arg("ssid");
  String password = request->arg("password");
  String universe = request->arg("universe");
  String DmxAddr = request->arg("DmxAddr");
  String NrofLEDS = request->arg("NrofLEDS");
    
    String s_16Bit = "0";
    s_16Bit = request->arg("b_16Bit") == "0" ? "1" : "0";
    int b_16Bit_int = s_16Bit.toInt();
    
    String s_failover ="0";
    s_failover = request->arg("b_failover") == "0" ? "1" : "0";
    int b_failover_int = s_failover.toInt();
    
    String s_silent = "0";
    s_silent = request->arg("b_silent") == "0" ? "1" : "0";
    int b_silent_int = s_silent.toInt();
    
    String s_reverseArray = "0" ;
    s_reverseArray = request->arg("b_reverseArray") == "1" ? "1" : "0";
    int b_reverseArray_int = s_reverseArray.toInt();
    
    String s_dhcp ;
    s_dhcp = request->arg("b_dhcp") == "on" ? "1" : "0";
    int b_dhcp_int = s_dhcp.toInt();
    
  String devicename = request->arg("devicename");
  String universe_start = request->arg("universe_start");
  String universe_end = request->arg("universe_end");
  String ipaddress = request->arg("ipAddress");
  String subnet = request->arg("subnetMask");
  String gateway = request->arg("gateway");
  String dns = request->arg("dnsServer");

  // Print values from memory
  // convert DMX addres for readability
  //int realDMX = dmx.toInt()-1;
  //String DmxAddr = String(realDMX);
  writeConfigFile();
  storeString(SSID_EEPROM_ADDR, ssid);
  storeString(PASS_EEPROM_ADDR, password);
  storeString(UNIVERSE_EEPROM_ADDR, universe);
  storeString(DMX_ADDR_EEPROM_ADDR, DmxAddr);
  storeString(NRLEDS_EEPROM_ADDR, NrofLEDS);
  storeString(B_16BIT_EEPROM_ADDR, s_16Bit);
  storeString(B_FAILOVER_EEPROM_ADDR, s_failover);
  storeString(B_SILENT_EEPROM_ADDR, s_silent);
  storeString(B_REVERSE_ARRAY_EEPROM_ADDR, s_reverseArray);
  storeString(DEV_NAME_EEPROM_ADDR, devicename);
  storeString(UNIVERSE_START_EEPROM_ADDR, universe_start);
  storeString(UNIVERSE_END_EEPROM_ADDR, universe_end);
  storeString(B_REVERSE_ARRAY_EEPROM_ADDR, s_reverseArray);

  storeString(B_DHCP_EEPROM_ADDR, s_dhcp);
  storeString(IP_ADDR_EEPROM_ADDR, ipaddress);
  storeString(IP_SUBNET_EEPROM_ADDR, subnet);
  storeString(IP_GATEWAY_EEPROM_ADDR, gateway);
  storeString(IP_DNS_EEPROM_ADDR, dns);

  Serial.println("Received values from the web interface:");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.print("Universe: ");
  Serial.println(universe);
  Serial.print("DmxAddr: ");
  Serial.println(DmxAddr);
  Serial.print("NrofLEDS: ");
  Serial.println(NrofLEDS);
  Serial.print("Device Name: ");
  Serial.println(devicename);
  Serial.print("Art-Net Universe Start: ");
  Serial.println(universe_start);
  Serial.print("Art-Net Universe End: ");
  Serial.println(universe_end);
  // Store in memory

  String redirectScript = R"(
    <script>
      var count = 10;
      var countdown = setInterval(function() {
        document.getElementById("countdown").innerHTML = count;
        count--;
        if (count < 0) {
          clearInterval(countdown);
          window.history.back();
        }
      }, 1000);
    </script>
  )";

  String responseHtml = "<html><head><style>body { font-family: Arial, sans-serif; background-color: #343541; color: #fff; }</style></head><body><h1>Settings saved! Rebooting in <span id=\"countdown\">10</span> ..." + redirectScript + "</h1></body></html>";


  request->send(200, "text/html", responseHtml);

  // set the wifi pass and ssid
  WiFi.begin(ssid.c_str(), password.c_str());
  delay(1000);
  // blink led
  led.ledOn();
  // delay to let memory written 
  delay(2000);
  Serial.print("Restarting");
  // restart
  ESP.restart();
}

void handleIdentify(AsyncWebServerRequest* request) {
  loginUser(request);

  Serial.print("Identify");
  // check if not already identifying to avoid crashes
  if (!identify){
    identify = true;
    led.rgbEffect(reinterpret_cast<void*>(2));
    led.ledOn();
    delay(1000);
    led.blankLEDS(170);
    Serial.print("Identify complete");
    identify = false;
  }
}

void handleUpdate(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
  if (!index) {
    Serial.println("Update firmware...");
    led.blankLEDS(NrOfLeds);
    led.ledOn();
    // start with max available size
    if (Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Serial.println("Firmware update started");
    } else {
      Update.printError(Serial);
    }
  }
  // flashing firmware to ESP
  if (Update.write(data, len) != len) {
    Update.printError(Serial);
  }
  if (final) {
    if (Update.end(true)) {
      Serial.println("Firmware update successful");
      request->send(200, "text/plain", "Firmware update successful. Rebooting...");
      delay(1000);
      ESP.restart();
    } else {
      Update.printError(Serial);
    }
  }
}

String getStoredString(int address) {
  String result;
  for (int i = 0; i < 64; ++i) {
    char c = EEPROM.read(address + i);
    if (c == 0) break;
    result += c;
  }
  return result;
}

int getStoredInt(int address) {
  int result = 0;
  for (int i = 0; i < sizeof(int); ++i) {
    result |= (EEPROM.read(address + i) << (8 * i));
  }
  return result;
}

void storeString(int address, const String& value) {
  for (int i = 0; i < value.length(); ++i) {
    EEPROM.write(address + i, value[i]);
  }
  EEPROM.write(address + value.length(), '\0'); // Append termination character
  EEPROM.commit();

}

void storeByte(int address, uint8_t value) {
  EEPROM.write(address, value);
  EEPROM.commit();
  delay(100);
}

void dnsTask(void *pvParameters) {
    (void)pvParameters; // Unused parameter

    // Create a DNSServer object
    DNSServer *dnsServer = new DNSServer();

    // Start the DNS server
    dnsServer->start(53, "*", WiFi.softAPIP());

    // Main loop to process DNS requests
    while (true) {
        dnsServer->processNextRequest();
        vTaskDelay(10 / portTICK_PERIOD_MS); // Delay to allow other tasks to run
    }
}

void startDNSServerTask() {
    xTaskCreatePinnedToCore(
        dnsTask,           // Function to run on the task
        "DNSServerTask",   // Name of the task
        4096,              // Stack size
        NULL,              // Task parameters
        1,                 // Priority (1 is default)
        &dnsTaskHandle,    // Task handle
        0                  // Core to run the task on (0 or 1)
    );
}

// Starts wifi accesspoint and setup page
void startAccessPoint(bool waitForClient) {
  // Set AP mode true, this is needed for LED indication 
  b_APmode = true;
 
  Serial.println("Starting in Access Point mode...");

  WiFi.softAP(HOST_NAME.c_str(),  APpass);
  delay(100);
  WiFi.mode(WIFI_AP_STA);
 
  Serial.println("Access Point started");
  Serial.println(WiFi.softAPSSID());
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Connect to the " + HOST_NAME + " WiFi network and configure your settings.");
  // Setup webserver
  startDNSServerTask();
  setupWebServer();

   // Add the following lines to configure the captive portal
    //server.addHandler(new AsyncCallbackJsonWebHandler("/config", [](AsyncWebServerRequest *request, JsonVariant &json) {
    // Your existing configuration handling code
  //}));
  // Add the following lines to handle captive portal requests
  server.onNotFound([](AsyncWebServerRequest *request) {
    if (!request->host().equalsIgnoreCase(WiFi.softAPIP().toString())) {
      AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
      response->addHeader("Location", "http://" + WiFi.softAPIP().toString()+ "/ledcontrol");
      request->send(response);
    }
  });
  if (waitForClient){
    // Start the web server
    server.begin();
    int numStations = WiFi.softAPgetStationNum();
    while (numStations <= 0) {
      if (!b_silent){
        led.cFlash();}
      
      Serial.print("Number of connected clients: ");
      Serial.println(numStations);
      numStations = WiFi.softAPgetStationNum();
      delay(5000); // Adjust delay as needed
      led.blankLEDS(179);
      Serial.print("Number of connected clients: ");
      Serial.println(numStations);
    }
  }

 
}


