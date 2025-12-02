# Migration from Space Heater Controller to Pan-Tilt Controller

## What Changed

The ESP8266 firmware has been simplified to work as a pan-tilt motor controller for the HARC system.

### Removed Features
- ❌ WiFi connectivity
- ❌ Web server interface
- ❌ Temperature sensor control
- ❌ Ecobee API integration
- ❌ NTP time synchronization
- ❌ SPIFFS file system usage
- ❌ Timer library dependencies
- ❌ All web interface files (HTML, etc.)

### New Features
- ✅ Serial command interface (9600 baud)
- ✅ Pan-tilt motor control
- ✅ Water spray relay control
- ✅ Direct integration with HARC software
- ✅ Simplified codebase (no external dependencies)

## New File Structure

```
space-heater-controller-main/
├── PanTilt_Controller.ino    ← NEW: Main firmware file
├── platformio.ini             ← UPDATED: Removed dependencies
├── README.md                  ← UPDATED: New documentation
└── [old files kept for reference]
```

## How to Use

1. **Upload new firmware:**
   ```bash
   pio run --target upload
   ```

2. **Connect to HARC:**
   - Connect ESP8266 via USB
   - Update HARC `hardware_config.json` with correct serial port
   - Enable motor_controller in config

3. **Test connection:**
   - HARC backend will automatically connect
   - Monitor serial output: `pio device monitor`

## Old Files

The following files are kept for reference but are no longer used:
- `Ecobee_Heater_Controller.ino` (old firmware)
- `data/` folder (web interface files)
- `OFFLINE_CHANGES.md` (old documentation)

You can delete these if not needed.

## Pin Configuration

Update pin definitions in `PanTilt_Controller.ino` based on your hardware:

```cpp
#define PAN_DIR_PIN 4       // Your pan motor direction pin
#define PAN_SPEED_PIN 5     // Your pan motor speed pin
#define TILT_DIR_PIN 12     // Your tilt motor direction pin
#define TILT_SPEED_PIN 13   // Your tilt motor speed pin
#define RELAY_PIN 16        // Water spray relay (unchanged)
```

## Serial Protocol

All communication is via Serial at 9600 baud. Commands:
- `MOVE X Y` - Relative movement
- `POS X Y` - Absolute position
- `SPRAY ON/OFF` - Water spray control
- `RESET` - Home position
- `STOP` - Emergency stop

See `README.md` for full protocol documentation.

