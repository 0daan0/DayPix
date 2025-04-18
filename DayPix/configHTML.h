const char *config_html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>DayPix Config</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        h1, h3, h4 { color: #333; }
        input, select { margin: 5px; padding: 5px; }
        form { margin-bottom: 20px; }
    </style>
</head>
<body>
    <!-- Title -->
    <h1>DayPix Config</h1>

    <!-- Device/Network Settings -->
    <form action="/save" method="post">
        <h3>Device/Network Settings</h3>
        Device Name: <input type="deviceName" name="deviceName" value=""><br>
        WiFi SSID: <input type="ssid" name="ssid" value=""><br>
        WiFi Password: <input type="password" name="password" value=""><br>

        <input type="checkbox" id="failoverCheckbox" name="b_failover"> 
        <label for="failoverCheckbox">Ethernet to WiFi Failover</label><br>
        <h4>Failover mode will fallback to WiFi when Ethernet is disconnected.</h4>

        <!-- IP Configuration -->
        <h3>IP Configuration</h3>
        <input type="checkbox" id="dhcpCheckbox" name="b_dhcp"> Use DHCP<br>
        <div id="manualConfig" style="display: none;">
            IP Address: <input type="text" id="ipAddress" name="ipAddress" value=""><br>
            Subnet Mask: <input type="text" id="subnetMask" name="subnetMask" value=""><br>
            Gateway: <input type="text" id="gateway" name="gateway" value=""><br>
            DNS Server: <input type="text" id="dnsServer" name="dnsServer" value=""><br>
        </div>
    </form>

    <!-- Gamma Slider -->
    <label for="gammaSlider">Gamma:</label><br>
    <input type="range" id="gammaSlider" name="gamma" min="0.01" max="5" step="0.01" value="2.2">
    <span id="gammaValue">2.2</span><br>

    <!-- Firmware & Hardware Information -->
    <h3>Firmware and Hardware Information</h3>
    Firmware Version: <span id="firmwareVersion">Loading...</span><br>
    HW Version: <span id="hwVersion">Loading...</span><br>

    <!-- Buttons -->
    <form action="/save" method="post"><input type="submit" value="Save"></form>
    <form><input type="button" value="Identify" onclick="identifyClicked()"></form>
    <form><input type="button" value="Reboot" onclick="rebootClicked()"></form>

    <!-- Firmware Update -->
    <form method="POST" action="/update" enctype="multipart/form-data">
        Firmware Update: <input type="file" name="update"><br>
        <input type="submit" value="Update">
    </form>

<!-- JavaScript -->
<script>
    document.addEventListener('DOMContentLoaded', function() {
        // Fetch Device Name
        fetch('/getDeviceName')
            .then(response => response.text())
            .then(data => {
                document.querySelector('[name="deviceName"]').value = data;
            });

        // Fetch SSID
        fetch('/getSSID')
            .then(response => response.text())
            .then(data => {
                document.querySelector('[name="ssid"]').value = data;
            });

        // Fetch Gamma Value
        fetch('/getGamma')
            .then(response => response.text())
            .then(data => {
                document.getElementById('gammaValue').textContent = data;
            });

        // Fetch Firmware Version
        fetch('/getFirmwareVersion')
            .then(response => response.text())
            .then(data => {
                document.getElementById('firmwareVersion').textContent = data;
            });

        // Fetch HW Version
        fetch('/getHwVersion')
            .then(response => response.text())
            .then(data => {
                document.getElementById('hwVersion').textContent = data;
            });

        // DHCP checkbox event listener
        const dhcpCheckbox = document.getElementById('dhcpCheckbox');
        const manualConfig = document.getElementById('manualConfig');
        dhcpCheckbox.addEventListener('change', function() {
            manualConfig.style.display = this.checked ? 'none' : 'block';
        });

        // Fetch IP settings if DHCP is disabled
        fetch('/getIPSettings')
            .then(response => response.json())
            .then(data => {
                if (!data.dhcp) {
                    document.getElementById('ipAddress').value = data.ip;
                    document.getElementById('subnetMask').value = data.subnetMask;
                    document.getElementById('gateway').value = data.gateway;
                    document.getElementById('dnsServer').value = data.dnsServer;
                    dhcpCheckbox.checked = false;
                    manualConfig.style.display = 'block';
                } else {
                    dhcpCheckbox.checked = true;
                    manualConfig.style.display = 'none';
                }
            });
    });

    function identifyClicked() {
        fetch('/identify', { method: 'POST' });
    }

    function rebootClicked() {
        fetch('/reboot', { method: 'POST' });
    }
</script>

</body>
</html>
)rawliteral";
