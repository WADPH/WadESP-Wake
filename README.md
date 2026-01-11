# WadESP-Wake

WadESP-Wake is a lightweight ESP8266-based Wake-on-LAN controller with a built-in web panel, designed to remotely power on PCs and servers, monitor their online status, and trigger startup actions from any browser within the network.

The project runs entirely on ESP8266 and provides:
- Web-based control panel
- HTTP Basic Authentication
- Online/offline status monitoring via ICMP ping
- Remote trigger execution (e.g. RouterOS scripts)
- Minimal dependencies and fast response

---

## Features

- Wake-on-LAN control via web UI  
- Device status monitoring (ping-based)  
- HTTP Basic Auth protection  
- CORS support  
- Rate limiting for command execution  
- Optimized for ESP8266 (low memory footprint)

---

## Hardware

- ESP8266 (Wemos D1 Mini, NodeMCU, etc.)
- Wi-Fi network with access to target devices

---

## Configuration (Important)

Before flashing the firmware, **you must configure sensitive variables** according to your environment.  
It is **strongly recommended** to move these values into a separate `secrets.h` file and exclude it from version control.

### 1. Wi-Fi credentials

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* pass = "YOUR_WIFI_PASSWORD";
```

### 2. Web panel authentication (Basic Auth)
```cpp
const char* www_user = "YOUR_HTTP_USERNAME";
const char* www_pass = "YOUR_HTTP_PASSWORD";
```

Used to protect access to:
- Web interface (/)

- Status endpoint (/status)

- Command execution endpoint (/run)

### 3. Target devices for status monitoring
```cpp
const char* serverIP = "192.168.0.10";
const char* homePCIP = "192.168.0.3";
```

IP addresses of devices that will be pinged to determine online/offline status.

### 4. Remote execution host (e.g. RouterOS)
```cpp
const char* host = "192.168.0.1";
```

IP address of the device that receives remote execution requests
(example: MikroTik RouterOS REST API).

### 5. Remote execution credentials
```cpp
String credentials = base64::encode(
  String("ROUTER_USER:ROUTER_PASSWORD")
);
```

Credentials used for HTTP Basic Authentication when sending remote commands.
