#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESPping.h>
#include <WiFiClientSecure.h>
#include <base64.h>

// =============================================
//  SECTION: Variables
// =============================================

const char* ssid = "YOUR_WIFI_SSID"; // Change Me
const char* pass = "YOUR_WIFI_PASSWORD"; // Change Me

// Basic Auth
const char* www_user = "YOUR_HTTP_USERNAME"; // Change Me
const char* www_pass = "YOUR_HTTP_PASSWORD"; // Change Me
 
ESP8266WebServer server(80);

const char* serverIP = "192.168.100.10"; // Change Me
const char* homePCIP = "192.168.100.3"; // Change Me

const char* webpage PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Wemos Control Panel</title>
<style>
body {
  margin:0; padding:0;
  font-family: 'Helvetica Neue', Helvetica, Arial, sans-serif;
  background: #0f1720;
  color: #f1f5f9;
  display:flex; justify-content:center; align-items:center;
  min-height:100vh; flex-direction:column;
}

.container {
  display:flex; flex-direction:column; align-items:center; gap:20px;
  width: 100%; max-width: 500px; padding: 20px; box-sizing: border-box;
}

.card {
  background: rgba(15, 23, 32, 0.95);
  border-radius: 16px;
  padding: 20px;
  width: 100%;
  box-shadow: 0 10px 30px rgba(0,0,0,0.6);
}

.card h2 {
  margin-top:0; margin-bottom:16px; font-size:22px;
  text-align:center; color:#f1f5f9;
}

.status-list {
  display:flex; justify-content:space-around; gap:20px;
}

.status-item {
  display:flex; flex-direction:column; align-items:center;
  font-size:16px; color:#f1f5f9;
}

.dot {
  width:20px; height:20px;
  border-radius:50%;
  margin-top:6px;
  background:red;
  transition: background 0.3s ease;
}

.buttons {
  display:flex; justify-content:space-around; gap:20px; flex-wrap:wrap;
}

button {
  background: linear-gradient(135deg,#3b82f6,#2563eb);
  border:none; border-radius:12px;
  padding:14px 20px; color:#fff;
  font-weight:bold; font-size:16px;
  cursor:pointer;
  transition: all 0.2s ease;
  flex:1 1 45%;
  text-align:center;
}

button:hover {
  background: linear-gradient(135deg,#2563eb,#1d4ed8);
  transform: translateY(-2px) scale(1.02);
  box-shadow:0 8px 20px rgba(0,0,0,0.4);
}

button:active {
  transform: translateY(1px) scale(0.98);
  box-shadow:0 4px 12px rgba(0,0,0,0.3);
}
</style>
</head>
<body>
<div class="container">
  <div class="card">
    <h2>DEVICE STATUS</h2>
    <div class="status-list">
      <div class="status-item">
        SERVER
        <div id="dotServer" class="dot"></div>
      </div>
      <div class="status-item">
        HOME-PC
        <div id="dotHome" class="dot"></div>
      </div>
    </div>
  </div>
  <div class="card buttons">
    <button id="btn1" onclick="sendCommand(1)">WoL SERVER</button>
    <button id="btn3" onclick="sendCommand(3)">WoL HOME-PC</button>
  </div>
</div>

<script>
async function updateStatus() {
  try {
    const res = await fetch('/status');
    const data = await res.json();
    document.getElementById('dotServer').style.background = data.server ? 'limegreen':'red';
    document.getElementById('dotHome').style.background = data.home ? 'limegreen':'red';
    document.getElementById('btn1').style.display = 'inline-block';
    document.getElementById('btn3').style.display = 'inline-block';
  } catch(e) { console.log(e); }
}

async function sendCommand(id) {
  try {
    const res = await fetch('/run?id='+id);
    if(res.ok) alert('Done');
    else alert('Error');
  } catch(e) { alert('Error: '+e); }
}

setInterval(updateStatus, 3000);
window.onload = updateStatus;
</script>
</body>
</html>
)rawliteral";


void addCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

unsigned long lastExecMillis = 0;

void handleRoot() {
  addCORS();
  if (!server.authenticate(www_user, www_pass)) { server.requestAuthentication(); return; }
  server.send(200, "text/html", FPSTR(webpage));
}

// Rute for statuses
void handleStatus() {
  addCORS();
  if (!server.authenticate(www_user, www_pass)) { server.requestAuthentication(); return; }
  IPAddress ipServer, ipHome;
  ipServer.fromString(serverIP);
  ipHome.fromString(homePCIP);
  bool aliveServer = Ping.ping(ipServer,3);
  bool aliveHome = Ping.ping(ipHome,3);
  String res = String("{\"server\":")+(aliveServer?"true":"false")+
               ",\"home\":"+(aliveHome?"true":"false")+"}";
  server.send(200,"application/json",res);
}

void handleRun() {
  addCORS();
  if (!server.authenticate(www_user, www_pass)) { server.requestAuthentication(); return; }
  if (!server.hasArg("id")) { server.send(400,"application/json","{\"error\":\"no id\"}"); return; }
  unsigned long now = millis();
  if(now - lastExecMillis < 5000) { server.send(429,"application/json","{\"error\":\"too_many_requests\"}"); return; }
  lastExecMillis = now;

  String id = server.arg("id");
  WiFiClient client;
// =============================================
//  SECTION: Remote execution host (e.g. RouterOS)
// =============================================
  const char* host = "192.168.101.1"; // Change Me
  if(!client.connect(host,80)) { server.send(500,"application/json","{\"error\":\"connect failed\"}"); return; }

// =============================================
//  SECTION: Remote execution credentials
// =============================================

  String credentials = base64::encode(
  String("ROUTER_USER:ROUTER_PASSWORD")  // Change Me
);
  String payload = String("{\".id\":\"*") + id + "\"}";
  String request =
    String("POST /rest/system/script/run HTTP/1.1\r\n") +
    "Host: "+String(host)+"\r\n"+
    "Authorization: Basic "+credentials+"\r\n"+
    "Content-Type: application/json\r\n"+
    "Content-Length: "+payload.length()+"\r\n\r\n"+
    payload;

  client.print(request);
  unsigned long timeout = millis();
  while(client.connected() && millis()-timeout<3000) {
    if(client.available()) {
      String line = client.readStringUntil('\n');
      if(line.startsWith("HTTP/1.1 200")) { server.send(200,"application/json","{\"executed\":true}"); client.stop(); return; }
    }
  }
  server.send(500,"application/json","{\"error\":\"no response\"}");
  client.stop();
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  Serial.printf("Connecting to %s\n", ssid);
  while(WiFi.status()!=WL_CONNECTED) { delay(400); Serial.print("."); }
  Serial.println();
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/run", handleRun);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
