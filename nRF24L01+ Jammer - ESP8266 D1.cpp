/*
 * ============================================================================
 * nRF24L01+ Jammer - ESP8266 D1 (AZDelivery) - WITH LED INDICATION
 * ============================================================================
 * Pin Connections:
 *   CE  -> D2 (GPIO4)
 *   CSN -> D8 (GPIO15)
 *   SCK -> D5 (GPIO14)
 *   MOSI -> D7 (GPIO13)
 *   MISO -> D6 (GPIO12)
 * 
 * LED: Built-in LED on GPIO2 (D4) – active LOW
 *   - 3 blinks at startup → ready
 *   - Solid ON → jamming active
 *   - OFF → jamming stopped
 *   - Fast blink → nRF24 not detected
 * 
 * Features:
 *   - Web interface at http://192.168.4.1 (AP: NRF_Jammer, pw: 12345678)
 *   - Real-time status and elapsed timer
 *   - Channel presets: Full 2.4GHz (0-125) or Wi-Fi only (1-11)
 *   - Adjustable burst packets (1-20)
 *   - Power level: LOW, MEDIUM (HIGH), MAX
 *   - Auto-stop timer
 *   - Non-blocking jamming with Ticker
 * 
 * WARNING: Educational use only. Unauthorized jamming is illegal.
 * ============================================================================
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// ========== Pin Definitions ==========
#define CE_PIN   4   // D2 (GPIO4)
#define CSN_PIN 15   // D8 (GPIO15)
#define LED_PIN  2   // D4 (built-in LED, active LOW)

// ========== Wi-Fi Access Point ==========
const char* ap_ssid = "NRF_Jammer";
const char* ap_password = "12345678";
ESP8266WebServer server(80);

// ========== nRF24L01+ Setup ==========
RF24 radio(CE_PIN, CSN_PIN);

// ========== Jamming Parameters ==========
bool jammingActive = false;
unsigned long jamStartTime = 0;

// Channel presets
const uint8_t fullChannels[] = {
  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,
  20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,
  40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,
  60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,
  80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,
  100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,
  120,121,122,123,124,125
};
const uint8_t wifiChannels[] = {1,2,3,4,5,6,7,8,9,10,11};
const int numFullChannels = sizeof(fullChannels) / sizeof(fullChannels[0]);
const int numWifiChannels = sizeof(wifiChannels) / sizeof(wifiChannels[0]);

// Current settings
const uint8_t* currentChannelList = fullChannels;
int currentNumChannels = numFullChannels;
int currentChannelIndex = 0;
int burstPackets = 8;            // packets per channel (1-20)
rf24_pa_dbm_e powerLevel = RF24_PA_MAX;   // HIGH power

// Payload (16 bytes for speed)
uint8_t payload[16];

// Ticker for non-blocking jamming
Ticker jammerTicker;
unsigned long lastJammerRun = 0;
const unsigned long JAMMER_INTERVAL_MS = 20;  // 20ms between channel hops

// ========== LED Helper Functions ==========
void setLED(bool on) {
  // Built-in LED on D1: LOW = ON, HIGH = OFF
  digitalWrite(LED_PIN, on ? LOW : HIGH);
}

void blinkLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    setLED(true);
    delay(delayMs);
    setLED(false);
    delay(delayMs);
  }
}

// ========== HTML Web Interface (PROGMEM) ==========
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=yes">
  <title>NRF24 Jammer Control</title>
  <style>
    * { box-sizing: border-box; }
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
      color: #eee;
      margin: 0;
      padding: 20px;
      min-height: 100vh;
    }
    .container {
      max-width: 600px;
      margin: 0 auto;
      background: rgba(255,255,255,0.1);
      backdrop-filter: blur(10px);
      border-radius: 20px;
      padding: 20px;
      box-shadow: 0 8px 32px rgba(0,0,0,0.3);
    }
    h1 {
      text-align: center;
      margin-top: 0;
      font-size: 1.8rem;
      color: #ffd700;
    }
    .status-card {
      background: #0f0f1a;
      border-radius: 15px;
      padding: 15px;
      margin-bottom: 20px;
      text-align: center;
      border-left: 5px solid #ff4444;
    }
    .status {
      font-size: 1.4rem;
      font-weight: bold;
      margin: 10px 0;
    }
    .status.active { color: #ff4444; text-shadow: 0 0 5px #ff4444; }
    .status.inactive { color: #44ff44; }
    .timer {
      font-size: 2rem;
      font-family: monospace;
      letter-spacing: 2px;
      background: #00000088;
      display: inline-block;
      padding: 8px 20px;
      border-radius: 40px;
    }
    .button-group {
      display: flex;
      gap: 15px;
      justify-content: center;
      margin-bottom: 25px;
    }
    button {
      padding: 12px 24px;
      font-size: 1.2rem;
      font-weight: bold;
      border: none;
      border-radius: 50px;
      cursor: pointer;
      transition: transform 0.1s, opacity 0.2s;
    }
    button:active { transform: scale(0.97); }
    .btn-start { background: #ff4444; color: white; box-shadow: 0 0 10px #ff4444; }
    .btn-stop { background: #444444; color: white; }
    .btn-start:hover { opacity: 0.9; }
    .btn-stop:hover { background: #666; }
    .settings {
      background: #0f0f1a;
      border-radius: 15px;
      padding: 15px;
      margin-top: 20px;
    }
    .setting-row {
      margin-bottom: 15px;
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      gap: 10px;
    }
    .setting-row label {
      width: 130px;
      font-weight: bold;
    }
    select, input {
      flex: 1;
      padding: 8px 12px;
      border-radius: 8px;
      border: none;
      background: #2a2a3a;
      color: white;
      font-size: 1rem;
    }
    input[type="range"] {
      flex: 2;
    }
    .value-display {
      min-width: 40px;
      text-align: center;
      background: #2a2a3a;
      padding: 4px 8px;
      border-radius: 8px;
    }
    .footer {
      text-align: center;
      font-size: 0.7rem;
      margin-top: 20px;
      opacity: 0.6;
    }
    @media (max-width: 480px) {
      .container { padding: 15px; }
      .setting-row label { width: 100px; font-size: 0.9rem; }
      button { padding: 10px 20px; font-size: 1rem; }
    }
  </style>
</head>
<body>
<div class="container">
  <h1>📡 NRF24 Jammer</h1>
  <div class="status-card">
    <div>🔌 STATUS</div>
    <div id="statusText" class="status inactive">● STOPPED</div>
    <div>⏱️ ELAPSED</div>
    <div id="timerDisplay" class="timer">00:00:00</div>
  </div>
  <div class="button-group">
    <button id="startBtn" class="btn-start" onclick="setJammer(true)">▶ START</button>
    <button id="stopBtn" class="btn-stop" onclick="setJammer(false)">■ STOP</button>
  </div>
  <div class="settings">
    <h3>⚙️ CUSTOMIZATION</h3>
    <div class="setting-row">
      <label>📶 Channel Preset:</label>
      <select id="channelPreset" onchange="updateSettings()">
        <option value="full">Full 2.4GHz (0-125)</option>
        <option value="wifi">Wi-Fi only (1-11)</option>
      </select>
    </div>
    <div class="setting-row">
      <label>📦 Burst Packets:</label>
      <input type="range" id="burst" min="1" max="20" value="8" oninput="updateBurstValue(this.value); updateSettings()">
      <span id="burstValue" class="value-display">8</span>
    </div>
    <div class="setting-row">
      <label>⚡ Power Level:</label>
      <select id="powerLevel" onchange="updateSettings()">
        <option value="low">LOW (-12dBm)</option>
        <option value="medium">MEDIUM (-6dBm)</option>
        <option value="high" selected>HIGH (0dBm)</option>
      </select>
    </div>
    <div class="setting-row">
      <label>⏱️ Auto Stop (min):</label>
      <input type="number" id="autoStop" min="0" max="60" value="0" step="1" placeholder="0 = off" onchange="updateSettings()">
      <span style="font-size:0.8rem;">minutes</span>
    </div>
  </div>
  <div class="footer">
    ⚠️ Educational use only. Unauthorized jamming is illegal.
  </div>
</div>
<script>
  let elapsedInterval = null;
  let autoStopTimeout = null;

  function updateStatus(jamming) {
    const statusDiv = document.getElementById('statusText');
    if (jamming) {
      statusDiv.innerHTML = '⚠️ ACTIVE (JAMMING)';
      statusDiv.className = 'status active';
      document.getElementById('startBtn').disabled = true;
      document.getElementById('stopBtn').disabled = false;
      startElapsedTimer();
    } else {
      statusDiv.innerHTML = '● STOPPED';
      statusDiv.className = 'status inactive';
      document.getElementById('startBtn').disabled = false;
      document.getElementById('stopBtn').disabled = true;
      stopElapsedTimer();
      document.getElementById('timerDisplay').innerHTML = '00:00:00';
    }
  }

  function startElapsedTimer() {
    if (elapsedInterval) clearInterval(elapsedInterval);
    let startTime = Date.now();
    elapsedInterval = setInterval(() => {
      const elapsed = Math.floor((Date.now() - startTime) / 1000);
      const hours = String(Math.floor(elapsed / 3600)).padStart(2,'0');
      const minutes = String(Math.floor((elapsed % 3600) / 60)).padStart(2,'0');
      const seconds = String(elapsed % 60).padStart(2,'0');
      document.getElementById('timerDisplay').innerHTML = `${hours}:${minutes}:${seconds}`;
    }, 1000);
  }

  function stopElapsedTimer() {
    if (elapsedInterval) {
      clearInterval(elapsedInterval);
      elapsedInterval = null;
    }
  }

  function updateBurstValue(val) {
    document.getElementById('burstValue').innerText = val;
  }

  async function setJammer(active) {
    const preset = document.getElementById('channelPreset').value;
    const burst = document.getElementById('burst').value;
    const power = document.getElementById('powerLevel').value;
    const autoStopMin = parseInt(document.getElementById('autoStop').value) || 0;
    const url = `/jam?active=${active}&preset=${preset}&burst=${burst}&power=${power}&autoStop=${autoStopMin}`;
    try {
      const res = await fetch(url);
      const data = await res.json();
      updateStatus(data.jamming);
      if (data.jamming && autoStopMin > 0) {
        if (autoStopTimeout) clearTimeout(autoStopTimeout);
        autoStopTimeout = setTimeout(() => setJammer(false), autoStopMin * 60 * 1000);
      }
    } catch(e) { console.error(e); }
  }

  async function updateSettings() {
    const statusDiv = document.getElementById('statusText');
    const isActive = statusDiv.innerHTML.includes('ACTIVE');
    if (isActive) {
      await setJammer(true);
    }
  }

  async function fetchStatus() {
    try {
      const res = await fetch('/status');
      const data = await res.json();
      updateStatus(data.jamming);
    } catch(e) { console.error(e); }
  }
  fetchStatus();
  setInterval(fetchStatus, 2000);
</script>
</body>
</html>
)rawliteral";

// ========== Helper: Apply nRF24 settings ==========
void applyRadioSettings() {
  radio.setPALevel(powerLevel);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(0);
  radio.setPayloadSize(sizeof(payload));
  radio.setAutoAck(false);
  radio.setRetries(0, 0);
  radio.setCRCLength(RF24_CRC_DISABLED);
  radio.stopListening();
}

// ========== Non-blocking jamming routine (called by Ticker) ==========
void runJammer() {
  if (!jammingActive) return;
  
  unsigned long now = millis();
  if (now - lastJammerRun < JAMMER_INTERVAL_MS) return;
  lastJammerRun = now;
  
  radio.setChannel(currentChannelList[currentChannelIndex]);
  for (int i = 0; i < burstPackets; i++) {
    radio.write(&payload, sizeof(payload));
  }
  
  currentChannelIndex++;
  if (currentChannelIndex >= currentNumChannels) currentChannelIndex = 0;
}

// ========== Web Handlers ==========
void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleJam() {
  bool newState = false;
  if (server.hasArg("active")) {
    newState = (server.arg("active") == "true");
    
    if (server.hasArg("preset")) {
      String preset = server.arg("preset");
      if (preset == "wifi") {
        currentChannelList = wifiChannels;
        currentNumChannels = numWifiChannels;
      } else {
        currentChannelList = fullChannels;
        currentNumChannels = numFullChannels;
      }
      currentChannelIndex = 0;
    }
    if (server.hasArg("burst")) {
      burstPackets = server.arg("burst").toInt();
      if (burstPackets < 1) burstPackets = 1;
      if (burstPackets > 20) burstPackets = 20;
    }
    if (server.hasArg("power")) {
      String pwr = server.arg("power");
      if (pwr == "low") {
        powerLevel = RF24_PA_LOW;
      } else if (pwr == "medium") {
        powerLevel = RF24_PA_HIGH;
      } else {
        powerLevel = RF24_PA_MAX;
      }
      applyRadioSettings();
    }
  }
  
  if (newState && !jammingActive) {
    jammingActive = true;
    jamStartTime = millis();
    setLED(true);           // <-- LED ON when jamming starts
    Serial.println("Jamming STARTED - LED ON");
  } else if (!newState && jammingActive) {
    jammingActive = false;
    setLED(false);          // <-- LED OFF when jamming stops
    Serial.println("Jamming STOPPED - LED OFF");
  }
  
  String json = "{\"jamming\":" + String(jammingActive ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

void handleStatus() {
  String json = "{\"jamming\":" + String(jammingActive ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

// ========== Setup ==========
void setup() {
  Serial.begin(115200);
  Serial.println("\n\nNRF24 Jammer v3.0 - ESP8266 D1 with LED");

  // Initialize LED (start with OFF)
  pinMode(LED_PIN, OUTPUT);
  setLED(false);          // LED off initially
  
  // SPI init
  SPI.begin();
  SPI.setFrequency(4000000);
  
  // Check nRF24
  if (!radio.begin()) {
    Serial.println("ERROR: nRF24 not detected! Check wiring.");
    // Fast blink forever to indicate error
    while (1) {
      setLED(true);
      delay(200);
      setLED(false);
      delay(200);
    }
  }
  Serial.println("nRF24 detected.");
  
  // Random payload
  for (int i = 0; i < sizeof(payload); i++) {
    payload[i] = random(0, 256);
  }
  
  applyRadioSettings();
  Serial.println("Radio configured: PA_MAX, 250kbps, CRC off.");
  
  // Start Wi-Fi AP
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("Access Point IP: ");
  Serial.println(WiFi.softAPIP());
  
  // Web server
  server.on("/", handleRoot);
  server.on("/jam", handleJam);
  server.on("/status", handleStatus);
  server.begin();
  Serial.println("HTTP server started.");
  Serial.println("Connect to Wi-Fi: " + String(ap_ssid) + " (pw: " + ap_password + ")");
  Serial.println("Then open http://" + WiFi.softAPIP().toString());
  
  // Blink LED 3 times to indicate ready
  blinkLED(3, 200);
  
  // Start Ticker
  jammerTicker.attach_ms(JAMMER_INTERVAL_MS, runJammer);
}

void loop() {
  server.handleClient();
  yield();
}