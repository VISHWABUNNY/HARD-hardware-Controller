# Space Heater Controller - Offline Version

A fully offline ESP8266-based IoT controller that automatically manages a space heater based on local temperature sensor readings. **No internet connection required!**

## Project Overview

This project uses an ESP8266 microcontroller to:
- Read temperature from a local analog sensor (LM35 or similar)
- Control a relay (connected to pin 16) to turn a space heater ON/OFF
- Provide a local web interface for configuration (works in Access Point mode)
- Automatically maintain temperature within a set range during specified time windows
- Operate completely offline with no external dependencies

## Features

- **100% Offline**: No internet connection required
- **Local Temperature Sensor**: Uses analog sensor (LM35) connected to ESP8266
- **Temperature Control**: Automatically turns heater ON when temperature drops below minimum threshold and OFF when it exceeds maximum threshold
- **Time-based Operation**: Only operates during configured time windows
- **Local Web Interface**: Accessible via WiFi Access Point (no router needed)
- **Local Time Management**: Manual time setting with automatic tracking
- **Logging**: Built-in logging system for debugging and monitoring
- **No Third-Party APIs**: Completely self-contained, no external services

## Hardware Requirements

- ESP8266 development board (NodeMCU, Wemos D1 Mini, etc.)
- Relay module (connected to pin 16)
- Space heater (connected through relay)
- LM35 temperature sensor (or similar analog sensor) connected to pin A0
- USB cable for programming

## Software Requirements

- Arduino IDE with ESP8266 board support, OR
- PlatformIO (recommended)
- Arduino CLI (already installed in this project)

## Dependencies

**Built-in Libraries Only:**
- ESP8266WiFi (built-in)
- ESP8266WebServer (built-in)
- FS/SPIFFS (built-in)

**Custom Library:**
- Timer.h (included in project - no external dependency)

**Removed Dependencies:**
- ❌ ArduinoJson (removed)
- ❌ NTPClient (removed)
- ❌ Ecobee API (removed)
- ❌ WiFiClientSecure (removed)
- ❌ All third-party services

## Configuration

### Temperature Sensor

The project uses an analog temperature sensor (LM35) connected to pin A0:
- LM35 outputs 10mV per degree Celsius
- 0V = 0°C
- For ESP8266: reading is converted to Fahrenheit

**To use a different sensor**, modify the `ReadTemperature()` function in the code.

### WiFi Configuration

The device can operate in two modes:

1. **Access Point Mode (Default - Fully Offline)**:
   - SSID: `HeaterController`
   - Password: `heater12345`
   - Connect to this network and access web interface at `192.168.4.1`

2. **Station Mode (Optional)**:
   - Configure WiFi credentials via web interface
   - Device will connect to your local WiFi network
   - Still works offline (no internet needed)

### Time Management

- Time is set manually via web interface
- Time is tracked locally using `millis()`
- Time persists across reboots (saved to SPIFFS)
- No NTP or internet time sync required

## Data Files (SPIFFS)

The following files need to be uploaded to the ESP8266's SPIFFS filesystem:

- `data/index.html` - Web interface
- `data/log.html` - Log viewer page
- `data/temp.txt` - Temperature range (min on line 1, max on line 2)
- `data/time.txt` - Time range (start on line 1, end on line 2, format: HH:MM)
- `data/log.txt` - Log file (created automatically)
- `data/currenttime.txt` - Current time (created automatically)
- `data/wifi.txt` - WiFi configuration (created automatically)

## Setup Instructions

### Using Arduino CLI (Already Installed)

1. Navigate to project directory:
   ```bash
   cd space-heater-controller-main
   ```

2. Compile the project:
   ```bash
   export PATH="$PATH:/home/vishwa/Vishwa/OCF/download/space-heater-controller-main/bin"
   arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 Ecobee_Heater_Controller
   ```

3. Upload code (when ESP8266 connected):
   ```bash
   arduino-cli upload -p /dev/ttyUSB0 --fqbn esp8266:esp8266:nodemcuv2 Ecobee_Heater_Controller
   ```

4. Upload SPIFFS data files:
   ```bash
   # Use ESP8266FS tool or similar to upload data/ folder contents
   ```

### Using PlatformIO

1. Install PlatformIO:
   ```bash
   pip install platformio
   ```

2. Navigate to project directory:
   ```bash
   cd space-heater-controller-main
   ```

3. Upload filesystem:
   ```bash
   pio run --target uploadfs
   ```

4. Upload code:
   ```bash
   pio run --target upload
   ```

5. Monitor serial output:
   ```bash
   pio device monitor
   ```

## Web Interface

Once running, access the web interface:

- **Access Point Mode**: Connect to `HeaterController` network, then visit `192.168.4.1`
- **Station Mode**: Visit the IP address shown in serial monitor

The interface allows you to:
- View current heater state and temperature
- Configure temperature range
- Configure time window
- Set current time manually
- Configure WiFi (optional)
- View logs
- Manually trigger temperature check

## How It Works

1. Device starts in Access Point mode (or connects to WiFi if configured)
2. Every 30 seconds, reads temperature from local analog sensor
3. Checks if current time is within active window
4. If temperature < min threshold → turns heater ON
5. If temperature > max threshold → turns heater OFF
6. Every second, checks if still within time window (turns OFF if outside)
7. Time is tracked locally using millis() - no internet needed

## Default Settings

- Temperature Range: 68°F - 72°F (configurable)
- Time Window: 09:30 - 22:00 (configurable)
- Temperature Check Interval: 30 seconds
- Time Check Interval: 1 second
- WiFi AP SSID: `HeaterController`
- WiFi AP Password: `heater12345`

## Wiring

```
ESP8266          Component
--------          --------
Pin 16    ----->  Relay Module (IN)
Pin A0     ----->  LM35 Temperature Sensor (Vout)
3.3V      ----->  LM35 VCC
GND       ----->  LM35 GND, Relay GND
```

## Troubleshooting

- **Temperature readings incorrect**: Check LM35 wiring and adjust `ReadTemperature()` function
- **Web interface not accessible**: Check if device is in AP mode, connect to `HeaterController` network
- **Time resets on reboot**: Ensure SPIFFS is properly initialized and `currenttime.txt` is being saved
- **Heater not turning on/off**: Check relay wiring and verify temperature thresholds are set correctly

## Advantages of Offline Version

✅ **No Internet Required**: Works completely offline
✅ **No Third-Party Dependencies**: No external APIs or services
✅ **Privacy**: All data stays local
✅ **Reliability**: No network outages or API failures
✅ **Lower Cost**: No API subscriptions or cloud services
✅ **Faster Response**: No network latency
✅ **Simple Setup**: Just connect sensor and configure

## License

[Add license information here]
