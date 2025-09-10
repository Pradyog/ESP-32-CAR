#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

// Access Point credentials (ESP32 creates its own WiFi network)
const char* ap_ssid = "ESP32_Car_Control";     // Name of WiFi network created by ESP32
const char* ap_password = "12345678";          // Password (minimum 8 characters)

// Server settings
WebServer server(80);
DNSServer dnsServer;

// Motor Driver Pin Definitions (EN pins removed)
#define IN1 21   // Input 1 for Motor A - Connected to D21
#define IN2 19   // Input 2 for Motor A - Connected to D19
#define IN3 23   // Input 3 for Motor B - Connected to D23
#define IN4 18   // Input 4 for Motor B - Connected to D18

// Built-in LED for status indication
#define LED_PIN 2

// Variables for improved command processing
char currentCommand = 'S';
unsigned long lastCommandTime = 0;
const unsigned long COMMAND_TIMEOUT = 1000; // Stop after 1 second of no command

void setup() {
  Serial.begin(115200);
  
  // Set motor pins as outputs (EN pins removed)
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  // Initialize motors (stopped)
  stopMotors();
  
  // Blink LED to show startup
  for(int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
  
  // Configure ESP32 as Access Point
  Serial.println("Setting up Access Point...");
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  
  IPAddress myIP = WiFi.softAPIP();
  Serial.println();
  Serial.println("==================================");
  Serial.println("🚗 ESP32 Car Server Started!");
  Serial.println("==================================");
  Serial.print("📶 WiFi Network: ");
  Serial.println(ap_ssid);
  Serial.print("🔐 Password: ");
  Serial.println(ap_password);
  Serial.print("🌐 Server IP: ");
  Serial.println(myIP);
  Serial.println("==================================");
  Serial.println("📱 How to connect:");
  Serial.println("1. Connect to WiFi: " + String(ap_ssid));
  Serial.println("2. Open browser: http://" + myIP.toString());
  Serial.println("   or simply: http://192.168.4.1");
  Serial.println("==================================");
  
  // Setup DNS server for captive portal (optional)
  dnsServer.start(53, "*", myIP);
  
  // Define web server routes (speed control removed)
  server.on("/", handleRoot);
  server.on("/forward", handleForward);
  server.on("/backward", handleBackward);
  server.on("/left", handleLeft);
  server.on("/right", handleRight);
  server.on("/stop", handleStop);
  server.on("/status", handleStatus);
  server.on("/info", handleInfo);
  
  // Handle unknown requests (captive portal)
  server.onNotFound(handleRoot);
  
  // Start server
  server.begin();
  Serial.println("✅ Web server started successfully!");
  
  // Turn on LED to indicate ready
  digitalWrite(LED_PIN, HIGH);
}

void loop() {
  // Handle DNS requests (for captive portal)
  dnsServer.processNextRequest();
  
  // Handle client requests
  server.handleClient();
  
  // Auto-stop if no command received for a while (safety feature)
  if (millis() - lastCommandTime > COMMAND_TIMEOUT && currentCommand != 'S') {
    stopMotors();
    currentCommand = 'S';
    Serial.println("🛑 Auto-stopped (timeout)");
  }
  
  delay(10);
}

// Enhanced web page HTML with speed control removed
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>ESP32 Car Control</title>
    <style>
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }
        body { 
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            min-height: 100vh;
            padding: 10px;
            overflow-x: hidden;
        }
        .container {
            max-width: 400px;
            margin: 0 auto;
            padding: 20px;
        }
        h1 { 
            text-align: center;
            margin-bottom: 30px;
            font-size: 24px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }
        .control-panel {
            background: rgba(255,255,255,0.15);
            padding: 20px;
            border-radius: 20px;
            margin-bottom: 20px;
            backdrop-filter: blur(10px);
            box-shadow: 0 8px 32px rgba(31, 38, 135, 0.37);
            border: 1px solid rgba(255, 255, 255, 0.18);
        }
        .panel-title {
            text-align: center;
            margin-bottom: 20px;
            font-size: 18px;
            color: #FFD93D;
        }
        .direction-grid {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            grid-template-rows: 1fr 1fr 1fr;
            gap: 10px;
            margin: 20px auto;
            max-width: 250px;
        }
        .control-btn {
            background: linear-gradient(145deg, #667eea, #764ba2);
            border: none;
            color: white;
            padding: 15px;
            text-align: center;
            font-size: 16px;
            cursor: pointer;
            border-radius: 15px;
            box-shadow: 0 4px 15px rgba(0,0,0,0.2);
            transition: all 0.2s ease;
            user-select: none;
            -webkit-user-select: none;
            -webkit-touch-callout: none;
        }
        .control-btn:active {
            transform: scale(0.95);
            box-shadow: 0 2px 8px rgba(0,0,0,0.3);
        }
        .forward { grid-column: 2; grid-row: 1; }
        .left { grid-column: 1; grid-row: 2; }
        .stop-btn { 
            grid-column: 2; 
            grid-row: 2;
            background: linear-gradient(145deg, #ff4757, #ff3838);
            font-weight: bold;
        }
        .right { grid-column: 3; grid-row: 2; }
        .backward { grid-column: 2; grid-row: 3; }
        .status {
            background: rgba(255,255,255,0.2);
            padding: 15px;
            border-radius: 15px;
            text-align: center;
            font-size: 16px;
            border: 1px solid rgba(255,255,255,0.3);
        }
        .status-label {
            opacity: 0.8;
            margin-bottom: 5px;
        }
        #currentStatus {
            font-weight: bold;
            color: #FFD93D;
            font-size: 18px;
        }
        .info-btn {
            position: fixed;
            top: 20px;
            right: 20px;
            background: rgba(255,255,255,0.2);
            border: none;
            color: white;
            width: 40px;
            height: 40px;
            border-radius: 50%;
            cursor: pointer;
            font-size: 20px;
        }
        @media (max-width: 480px) {
            .container {
                padding: 10px;
            }
            h1 {
                font-size: 20px;
                margin-bottom: 20px;
            }
            .control-btn {
                padding: 12px;
                font-size: 14px;
            }
        }
    </style>
</head>
<body>
    <button class="info-btn" onclick="showInfo()">ℹ️</button>
    
    <div class="container">
        <h1>🚗 ESP32 Car Control</h1>
        
        <div class="control-panel">
            <div class="panel-title">Direction Control (Full Speed)</div>
            <div class="direction-grid">
                <button class="control-btn forward" 
                    onmousedown="sendCommand('forward')" 
                    onmouseup="sendCommand('stop')" 
                    ontouchstart="sendCommand('forward')" 
                    ontouchend="sendCommand('stop')">⬆️</button>
                <button class="control-btn left" 
                    onmousedown="sendCommand('left')" 
                    onmouseup="sendCommand('stop')" 
                    ontouchstart="sendCommand('left')" 
                    ontouchend="sendCommand('stop')">⬅️</button>
                <button class="control-btn stop-btn" onclick="sendCommand('stop')">STOP</button>
                <button class="control-btn right" 
                    onmousedown="sendCommand('right')" 
                    onmouseup="sendCommand('stop')" 
                    ontouchstart="sendCommand('right')" 
                    ontouchend="sendCommand('stop')">➡️</button>
                <button class="control-btn backward" 
                    onmousedown="sendCommand('backward')" 
                    onmouseup="sendCommand('stop')" 
                    ontouchstart="sendCommand('backward')" 
                    ontouchend="sendCommand('stop')">⬇️</button>
            </div>
        </div>
        
        <div class="status">
            <div class="status-label">Current Status:</div>
            <div id="currentStatus">Stopped</div>
        </div>
    </div>
    
    <script>
        function sendCommand(command) {
            fetch('/' + command, {method: 'POST'})
                .then(response => response.text())
                .then(data => {
                    document.getElementById('currentStatus').innerText = data;
                })
                .catch(error => console.error('Error:', error));
        }
        
        function showInfo() {
            window.open('/info', '_blank');
        }
        
        // Update status every 3 seconds
        setInterval(function() {
            fetch('/status')
                .then(response => response.text())
                .then(data => {
                    document.getElementById('currentStatus').innerText = data;
                });
        }, 3000);
        
        // Prevent context menu and text selection
        document.addEventListener('contextmenu', function(e) {
            e.preventDefault();
        });
        
        document.addEventListener('selectstart', function(e) {
            e.preventDefault();
        });
        
        // Prevent zoom on double tap
        let lastTouchEnd = 0;
        document.addEventListener('touchend', function (event) {
            let now = (new Date()).getTime();
            if (now - lastTouchEnd <= 300) {
                event.preventDefault();
            }
            lastTouchEnd = now;
        }, false);
    </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

// Info page (updated to remove speed control references)
void handleInfo() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ESP32 Car - Connection Info</title>
    <style>
        body { 
            font-family: Arial; 
            padding: 20px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            min-height: 100vh;
        }
        .info-box {
            background: rgba(255,255,255,0.15);
            padding: 20px;
            border-radius: 15px;
            margin: 10px 0;
            backdrop-filter: blur(10px);
        }
        h1 { text-align: center; color: #FFD93D; }
        h2 { color: #FFD93D; margin-top: 20px; }
        .highlight { color: #FFD93D; font-weight: bold; }
        ul { padding-left: 20px; }
        li { margin: 5px 0; }
    </style>
</head>
<body>
    <h1>📡 ESP32 Car - Connection Info</h1>
    
    <div class="info-box">
        <h2>🔗 Connection Details</h2>
        <p><strong>WiFi Network:</strong> <span class="highlight">ESP32_Car_Control</span></p>
        <p><strong>Password:</strong> <span class="highlight">12345678</span></p>
        <p><strong>Server IP:</strong> <span class="highlight">192.168.4.1</span></p>
        <p><strong>Server URL:</strong> <span class="highlight">http://192.168.4.1</span></p>
    </div>
    
    <div class="info-box">
        <h2>📱 How to Connect</h2>
        <ol>
            <li>Go to your device's <strong>WiFi Settings</strong></li>
            <li>Look for network: <span class="highlight">"ESP32_Car_Control"</span></li>
            <li>Connect using password: <span class="highlight">"12345678"</span></li>
            <li>Open any web browser</li>
            <li>Go to: <span class="highlight">http://192.168.4.1</span></li>
        </ol>
    </div>
    
    <div class="info-box">
        <h2>🎮 Controls</h2>
        <ul>
            <li><strong>Direction Buttons:</strong> Hold to move, release to stop</li>
            <li><strong>STOP Button:</strong> Emergency stop</li>
            <li><strong>Full Speed:</strong> Motors run at maximum speed</li>
        </ul>
    </div>
    
    <div class="info-box">
        <h2>✨ Features</h2>
        <ul>
            <li>🚫 <strong>No Internet Required</strong> - Works offline</li>
            <li>📱 <strong>Multi-device Support</strong> - Multiple phones can control</li>
            <li>🛡️ <strong>Auto-stop Safety</strong> - Stops if connection lost</li>
            <li>⚡ <strong>Real-time Control</strong> - Instant response</li>
            <li>🎨 <strong>Mobile Optimized</strong> - Touch-friendly interface</li>
            <li>🔥 <strong>Full Speed Operation</strong> - Maximum power mode</li>
        </ul>
    </div>
    
    <div style="text-align: center; margin: 30px 0;">
        <button onclick="window.close()" style="
            background: linear-gradient(45deg, #FF6B6B, #FF8E53);
            border: none;
            color: white;
            padding: 15px 30px;
            border-radius: 10px;
            cursor: pointer;
            font-size: 16px;
        ">Close</button>
    </div>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

// Command handlers
void handleForward() {
  moveForward();
  currentCommand = 'F';
  lastCommandTime = millis();
  server.send(200, "text/plain", "Moving Forward");
  Serial.println("🔼 Forward");
}

void handleBackward() {
  moveBackward();
  currentCommand = 'B';
  lastCommandTime = millis();
  server.send(200, "text/plain", "Moving Backward");
  Serial.println("🔽 Backward");
}

void handleLeft() {
  turnLeft();
  currentCommand = 'L';
  lastCommandTime = millis();
  server.send(200, "text/plain", "Turning Left");
  Serial.println("◀️ Left");
}

void handleRight() {
  turnRight();
  currentCommand = 'R';
  lastCommandTime = millis();
  server.send(200, "text/plain", "Turning Right");
  Serial.println("▶️ Right");
}

void handleStop() {
  stopMotors();
  currentCommand = 'S';
  lastCommandTime = millis();
  server.send(200, "text/plain", "Stopped");
  Serial.println("🛑 Stop");
}

void handleStatus() {
  String status = "";
  switch(currentCommand) {
    case 'F': status = "Moving Forward"; break;
    case 'B': status = "Moving Backward"; break;
    case 'L': status = "Turning Left"; break;
    case 'R': status = "Turning Right"; break;
    case 'S': status = "Stopped"; break;
    default: status = "Unknown"; break;
  }
  server.send(200, "text/plain", status);
}

// Motor control functions (simplified without EN pins)
void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void moveBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void turnLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}
