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

//#include <SPIFFS.h>
ledDriver ledDriverInstance; 
AsyncWebServer server(80);

void setupWebServer() {
    // Set up endpoints for web server
  server.on("/diagnostic", HTTP_GET, handleDiagnostic);
  server.on("/8bitTest", HTTP_POST, handle8BitTest);
  server.on("/16bitTest", HTTP_POST, handle16BitTest);
  server.on("/BlankLEDSTest", HTTP_POST, handle16BitTest);
  server.on("/data/daypix.jpg", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(SPIFFS, "/daypix.jpg", "image/jpeg");});
  server.on("/getSignalStrength", HTTP_GET, handleSignalStrength);
  server.on("/identify", HTTP_POST, handleIdentify);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/ledcontrol", HTTP_POST, handleLedControl);
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", "<h1>Updating...</h1>");
  }, handleUpdate);
  server.on("/reboot", HTTP_POST, handleReboot);


server.on("/ledcontrol", HTTP_GET, [](AsyncWebServerRequest *request){
    // HTML for the LED control page
    String html = "<html><head><title>LED Control</title></head><body>";
    html += "<h1>LED Control</h1>";
    html += "<form id='ledForm'>";
    html += "Red: <input type='range' id='redSlider' name='red' min='0' max='255'><br>";
    html += "Green: <input type='range' id='greenSlider' name='green' min='0' max='255'><br>";
    html += "Blue: <input type='range' id='blueSlider' name='blue' min='0' max='255'><br>";
    html += "</form></body>";
    // JavaScript for handling slider changes and sending Fetch API requests
    html += "<script>";
    html += "const redSlider = document.getElementById('redSlider');";
    html += "const greenSlider = document.getElementById('greenSlider');";
    html += "const blueSlider = document.getElementById('blueSlider');";
    html += "const ledForm = document.getElementById('ledForm');";
    html += "redSlider.addEventListener('input', updateLed);";
    html += "greenSlider.addEventListener('input', updateLed);";
    html += "blueSlider.addEventListener('input', updateLed);";
    html += "function updateLed() {";
    html += "const red = redSlider.value;";
    html += "const green = greenSlider.value;";
    html += "const blue = blueSlider.value;";
    html += "fetch('/ledcontrol', {";
    html += "method: 'POST',";
    html += "headers: {";
    html += "'Content-Type': 'x-www-form-urlencoded'";
    html += "},";
    html += "body: 'red=' + encodeURIComponent(red) + '&green=' + encodeURIComponent(green) + '&blue=' + encodeURIComponent(blue)";
    html += "})";
    html += ".then(response => {";
    html += "if (response.ok) {";
    html += "console.log('LEDs updated successfully');";
    html += "} else {";
    html += "console.error('Failed to update LEDs');";
    html += "}";
    html += "})";
    html += ".catch(error => {";
    html += "console.error('Error:', error);";
    html += "});";
    html += "}";
    html += "</script>";
    html += "</html>";
    request->send(200, "text/html", html);
});




  server.begin();
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

void handleLedControl(AsyncWebServerRequest* request) {
 
           // Check if there is a request body
    // Check if the request has a body
  if (request->hasParam("plain", true)) {
    String body = request->getParam("plain", true)->value();

    // Parse the body to extract red, green, and blue values
    int red, green, blue;
    sscanf(body.c_str(), "red=%d&green=%d&blue=%d", &red, &green, &blue);
    led.setLEDColor(red, green, blue);
    // Here you can use the RGB values to control your LEDs or perform any desired action
    // For example, you could use these values to adjust the brightness of RGB LEDs
    Serial.printf("Received RGB values - Red: %d, Green: %d, Blue: %d\n", red, green, blue);

    // Respond with a success message
    request->send(200, "text/plain", "LEDs updated successfully");
  } else {
    // If the request body is missing, respond with an error message
    request->send(400, "text/plain", "Missing request body");
  }
}

void handleRoot(AsyncWebServerRequest* request) 
{
  loginUser(request);
  
// JavaScript code to periodically fetch and update WiFi signal strength
String script = R"====(
  <script>
    function updateSignalStrength() {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var signalStrength = parseInt(this.responseText);
          document.getElementById("signalStrength").innerHTML = "WiFi Signal Strength: " + signalStrength + " dBm";
          
          // Update signal strength meter
          updateSignalStrengthMeter(signalStrength);
        }
      };
      xhttp.open("GET", "/getSignalStrength", true);
      xhttp.send();
    }

    function updateSignalStrengthMeter(signalStrength) {
      var signalMeter = document.getElementById("signalMeter");
      var maxSignal = -30;  // Maximum signal strength
      var minSignal = -90; // Minimum signal strength
      var bars;

      // Calculate number of bars based on signal strength
      bars = Math.min(6, Math.max(1, Math.ceil((signalStrength - minSignal) / ((maxSignal - minSignal) / 6))));

      // Change color based on signal strength
      var color;
      if (bars == 1) {
        color = 'red';
      } else if (bars <= 3) {
        color = 'orange';
      } else if (bars <= 4) {
        color = 'yellow';
      } else {
        color = 'green';
      }

      // Update signal meter
      signalMeter.innerHTML = "";
      for (var i = 0; i < bars; i++) {
        var bar = document.createElement("div");
        bar.style.border = "1px solid black"; // Add outline
        bar.style.backgroundColor = color;
        bar.style.width = "calc(100% / 6 - 2px)"; // Set max width to 400 pixels
        bar.style.height = "5px";
        bar.style.display = "inline-block";
        signalMeter.appendChild(bar);
      }

      // Add labels below the signal meter
      var labels = ['Weak', 'Good', 'Best'];
      var labelContainer = document.getElementById("signalLabels");
      labelContainer.innerHTML = "";
      for (var i = 0; i < 3; i++) {
        var label = document.createElement("div");
        label.innerHTML = labels[i];
        label.style.width = "calc(100% / 3 - 2px)";
        label.style.textAlign = "center";
        label.style.display = "inline-block";
        labelContainer.appendChild(label);
      }
    }

    // Update signal strength every 5 seconds
    setInterval(updateSignalStrength, 5000);

    // Initial update
    updateSignalStrength();
  </script>
)====";


// Main html page of device. 
  // CSS style
String html = "<html><head><style>";
html += "body { font-family: Arial, sans-serif; background-color: #343541; color: #fff; }"; // Dark background color
html += "h1 { text-align: center; border: 3px solid; max-width: 400px; margin: 0 auto; background-image: linear-gradient(to right, violet, indigo, blue, green, yellow, orange, red); padding: 10px; color: #EFEFE0; text-shadow: -1px -1px 0 #000, 1px -1px 0 #000, -1px 1px 0 #000, 1px 1px 0 #000; }";
html += "h3 { text-align: left; margin-bottom: 5px; }";
html += "form { max-width: 400px; margin: 0 auto; }";
html += "label { display: block; margin-bottom: 5px; }";
html += "input[type='text'], input[type='password'], input[type='file'], select { width: 100%; padding: 8px; margin-bottom: 10px; box-sizing: border-box; background-color: #343541; color: #EFEFE0; border: 1px solid #EFEFE0; }"; // Background color: #222, Border color: #fff
html += "input[type='submit'], input[type='button'] { width: 100%; padding: 10px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer; }";
html += "input[type='submit']:hover, input[type='button']:hover { background-color: #45a049; }";
html += "select { width: 100%; padding: 8px; margin-bottom: 10px; box-sizing: border-box; background-color: #343541; color: #EFEFE0; border: 1px solid #EFEFE0; }"; // Background color: #222, Border color: #fff
html += "p { text-align: center; font-size: 10px; margin-top: 5px; }"; // Updated style for the paragraph
html += "#signalStrength { margin-bottom: 10px; }";
html += "</style></head><body>";


  // Title header 1
  html += "<h1>" + String(H_PRFX) + " Config</h1>";
  // Main form for wifi config input
  html += "<form action='/save' method='post'>";
  html += "<h3> Device/Network Settings </h3>";
  html += "Device Name    : <input type='text' name='devicename' value='" + DEV_NAME + "'><br>";
  html += "WiFi SSID    : <input type='text' name='ssid' value='" + String(getStoredString(SSID_EEPROM_ADDR)) + "'><br>";
  html += "WiFi Password: <input type='password' name='password' value='" + String(getStoredString(PASS_EEPROM_ADDR)) + "'><br>";
  if (led.ethCap){
  html += "Ethernet to Wifi Failover   : <input type='checkbox' name='b_failover' " + String(b_failover ? "checked" : "") + " value='" + String(b_failover) + "'><br>";
  html += "<p>Failover mode will fallback to wifi when Ethernet is disconnected</p>";
  }
  html += "</select><br>";
  html += "<div id='signalStrength'></div>";
  html += "<div id='signalMeter' style='max-width: 400px; border: 1px solid black;'></div>";  // Set max width to 400 pixels with a black border
  // Main form for DMX/Artnet config input
  html += "<h3> ArtNet/DMX Settings </h3>";
  //html += "Art-Net Universe Start: <input type='text' name='universe_start' value='" + String(getStoredString(UNIVERSE_START_EEPROM_ADDR)) + "'><br>";
  html += "Art-Net Universe Start: <select name='universe_start'>";
  for (int i = 0; i <= 15; ++i) {
    html += "<option value='" + String(i) + "' " + (getStoredString(UNIVERSE_START_EEPROM_ADDR).toInt() == i ? "selected" : "") + ">" + String(i) + "</option>";
  }
  html += "</select>";
  //html += "Art-Net Universe End: <input type='text' name='universe_end' value='" + String(getStoredString(UNIVERSE_END_EEPROM_ADDR)) + "'><br>";
  html += "Art-Net Universe End: <select name='universe_end'>";
  for (int i = 0; i <= 15; ++i) {
    html += "<option value='" + String(i) + "' " + (getStoredString(UNIVERSE_END_EEPROM_ADDR).toInt() == i ? "selected" : "") + ">" + String(i) + "</option>";
  }
  html += "</select>";
  //html += "Art-Net Universe: <select name='universe'>";
  //for (int i = 0; i <= 15; ++i) {
  //  html += "<option value='" + String(i) + "' " + (getStoredString(UNIVERSE_EEPROM_ADDR).toInt() == i ? "selected" : "") + ">" + String(i) + "</option>";
  //}
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
      // 16-bit mode: DMX address range is startAddr to (startAddr + (nrOfLeds * 6))
      endAddr = startAddr + (getStoredString(NRLEDS_EEPROM_ADDR).toInt() * 6 );
  } else {
      // 8-bit mode: DMX address range is startAddr to (startAddr + (nrOfLeds * 3))
      endAddr = startAddr + (getStoredString(NRLEDS_EEPROM_ADDR).toInt() * 3 );
  }

  html += "<br>";
  html += "Current Address Range: " + String(startAddr+1) + " - " + String(endAddr) + "<br>";


  html += "</select><br>";
  // Main form for LED config input
  html += "<h3> Pixel Settings </h3>";
  //html += "Nr of LEDS      : <input type='text' name='NrofLEDS' value='" + String(getStoredString(NRLEDS_EEPROM_ADDR)) + "'><br>";
  html += "Nr of Pixels   : <select name='NrofLEDS'>";
  for (int i = 1; i <= 170; ++i) {
      if (b_16Bit && i > 85) {
      break;
    }
    html += "<option value='" + String(i) + "' " + (getStoredString(NRLEDS_EEPROM_ADDR).toInt() == i ? "selected" : "") + ">" + String(i) + "</option>";
  }
  html += "</select><br>";
  html += "16-bit Mode   : <input type='checkbox' name='b_16Bit' " + String(b_16Bit ? "checked" : "") + " value='" + String(b_16Bit) + "'><br>";
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
  // add reboot button
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
  // script for identify button
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
  // Include the JavaScript code
  html += script;
  html += "</body></html>";
  request->send(200, "text/html", html);
}

void handleReboot(AsyncWebServerRequest* request) {
  request->send(200, "text/plain", "Rebooting...");
  delay(1000);
  ESP.restart();
}

void handleDiagnostic(AsyncWebServerRequest* request) {
  loginUser(request);

  // Gather diagnostic information
  String diagnosticInfo = "<html><head><style>";
  //diagnosticInfo += "System Diagnostics:\n";
  diagnosticInfo += "body { font-family: Arial, sans-serif; }";
  diagnosticInfo += "h1 { text-align: center; border: 3px solid; max-width: 400px; margin: 0 auto; background-image: linear-gradient(to right, violet, indigo, blue, green, yellow, orange, red); padding: 10px; color: #fff; text-shadow: -1px -1px 0 #000, 1px -1px 0 #000, -1px 1px 0 #000, 1px 1px 0 #000; }";
  diagnosticInfo += "h3 { text-align: left; margin-bottom: 5px; }";
  diagnosticInfo += "form { max-width: 400px; margin: 0 auto; }";
  diagnosticInfo += "label { display: block; margin-bottom: 5px; }";
  diagnosticInfo += "input[type='text'], input[type='password'], input[type='file'], select { width: 100%; padding: 8px; margin-bottom: 10px; box-sizing: border-box; }";
  diagnosticInfo += "input[type='submit'], input[type='button'] { width: 100%; padding: 10px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer; }";
  diagnosticInfo += "input[type='submit']:hover, input[type='button']:hover { background-color: #45a049; }";
  diagnosticInfo += "p { text-align: center; font-size: 10px; margin-top: 20px; }"; // Updated style for the paragraph
  diagnosticInfo += "#signalStrength { margin-bottom: 10px; }";
  diagnosticInfo += "</style></head><body>";
  diagnosticInfo += "Free Heap: " + String(ESP.getFreeHeap()) + " bytes\n";
  diagnosticInfo += "Received universe: "+ String(recvUniverse)+ "\n";
  // Add more diagnostic information as needed
  // Add buttons for 8-bit and 16-bit tests using AJAX
  diagnosticInfo += "<form id='identifyForm' method='post'>";
  diagnosticInfo += "<input type='button' value='16bitTest' onclick='runTest(\"16bit\")' style='margin-top: 10px;'>";
  diagnosticInfo += "</form>";
  // Add buttons for 8-bit and 8-bit tests using AJAX
  diagnosticInfo += "<form id='identifyForm' method='post'>";
  diagnosticInfo += "<input type='button' value='8bitTest' onclick='runTest(\"8bit\")' style='margin-top: 10px;'>";
  diagnosticInfo += "</form>";
  // Add buttons for 8-bit and blank tests using AJAX
  diagnosticInfo += "<form id='identifyForm' method='post'>";
  diagnosticInfo += "<input type='button' value='blankLEDS' onclick='runTest(\"blankLEDS\")' style='margin-top: 10px;'>";
  diagnosticInfo += "</form>";
  // Include JavaScript for AJAX that executes test type
  diagnosticInfo += "<script>";
  diagnosticInfo += "function runTest(testType) {";
  diagnosticInfo += "var xhttp = new XMLHttpRequest();";
  diagnosticInfo += "xhttp.open('POST', '/' + testType + 'Test', true);";
  diagnosticInfo += "xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');";
  diagnosticInfo += "xhttp.send();";
  diagnosticInfo += "}";
  diagnosticInfo += "</script>";

  // Send the diagnostic information as HTML with embedded JavaScript
  request->send(200, "text/html", diagnosticInfo);
    // Stop capturing Serial output


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

void handleBlankLEDSTest(AsyncWebServerRequest* request) {
  loginUser(request);
  diag = true;
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
  String b_16Bit = request->hasArg("b_16Bit") ? "1" : "0";
  b_16Bit = b_16Bit.toInt();
  String b_failover = request->hasArg("b_failover") ? "1" : "0";
  b_failover = b_failover.toInt();
    String b_silent = request->hasArg("b_silent") ? "1" : "0";
  b_silent = b_silent.toInt();
     String b_reverseArray = request->hasArg("b_reverseArray") ? "1" : "0";
  b_reverseArray = b_reverseArray.toInt();
  String devicename = request->arg("devicename");
  String universe_start = request->arg("universe_start");
  String universe_end = request->arg("universe_end");
  // Print values from memory
  // convert DMX addres for readability
  //int realDMX = dmx.toInt()-1;
  //String DmxAddr = String(realDMX);

  storeString(SSID_EEPROM_ADDR, ssid);
  storeString(PASS_EEPROM_ADDR, password);
  storeString(UNIVERSE_EEPROM_ADDR, universe);
  storeString(DMX_ADDR_EEPROM_ADDR, DmxAddr);
  storeString(NRLEDS_EEPROM_ADDR, NrofLEDS);
  storeString(B_16BIT_EEPROM_ADDR, b_16Bit);
  storeString(B_FAILOVER_EEPROM_ADDR, b_failover);
  storeString(B_SILENT_EEPROM_ADDR, b_silent);
  storeString(B_REVERSE_ARRAY_EEPROM_ADDR, b_reverseArray);
  storeString(DEV_NAME_EEPROM_ADDR, devicename);
  storeString(UNIVERSE_START_EEPROM_ADDR, universe_start);
  storeString(UNIVERSE_END_EEPROM_ADDR, universe_end);
  storeString(B_REVERSE_ARRAY_EEPROM_ADDR, b_reverseArray);

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
void startAccessPoint() {
  // Set AP mode true, this is needed for LED indication 
  b_APmode = true;
  startDNSServerTask();
  Serial.println("Starting in Access Point mode...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(HOST_NAME.c_str(), "setupdaypix");
  Serial.println("Access Point started");

  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Connect to the " + HOST_NAME + " WiFi network and configure your settings.");
  // Setup webserver
  setupWebServer();

   // Add the following lines to configure the captive portal
    server.addHandler(new AsyncCallbackJsonWebHandler("/config", [](AsyncWebServerRequest *request, JsonVariant &json) {
    // Your existing configuration handling code
  }));
  // Add the following lines to handle captive portal requests
  server.onNotFound([](AsyncWebServerRequest *request) {
    if (!request->host().equalsIgnoreCase(WiFi.softAPIP().toString())) {
      AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
      response->addHeader("Location", "http://" + WiFi.softAPIP().toString());
      request->send(response);
    }
  });
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
  }

  led.blankLEDS(179);
  Serial.print("Number of connected clients: ");
  Serial.println(numStations);
}


