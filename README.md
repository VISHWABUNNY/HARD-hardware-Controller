# HARC Pan-Tilt Motor Controller

ESP8266 firmware for controlling pan-tilt mechanism via serial commands from HARC software system.

## Overview

This firmware runs on an ESP8266 microcontroller and receives serial commands from the HARC software (Python/FastAPI backend) to control:
- **Pan motor** - Horizontal rotation
- **Tilt motor** - Vertical rotation  
- **Water spray relay** - On/off control for water cannon

## Features

- ✅ **Serial Command Interface** - Receives commands via USB Serial (9600 baud)
- ✅ **Pan-Tilt Control** - Independent control of pan and tilt motors
- ✅ **Water Spray Control** - Relay control for water cannon
- ✅ **No WiFi Required** - Pure serial communication
- ✅ **No Web Interface** - All control via HARC software
- ✅ **Simple & Lightweight** - Minimal dependencies

## Hardware Requirements

- ESP8266 development board (NodeMCU, Wemos D1 Mini, etc.)
- Pan motor driver (DC motor driver or stepper driver)
- Tilt motor driver (DC motor driver or stepper driver)
- Relay module (for water spray control)
- USB cable for programming and communication

## Pin Configuration

Default pin assignments (modify in code if needed):

```
ESP8266 Pin    Function
-----------    --------
Pin 16         Water spray relay control
Pin 4          Pan motor direction
Pin 5          Pan motor speed/PWM
Pin 12         Tilt motor direction
Pin 13         Tilt motor speed/PWM
```

**Note:** Adjust pin definitions in `PanTilt_Controller.ino` based on your motor driver setup.

## Serial Communication Protocol

Communication: **9600 baud, 8N1**

### Commands

| Command | Format | Description |
|---------|--------|-------------|
| `MOVE` | `MOVE X Y` | Relative movement (-100 to 100 for each axis) |
| `POS` | `POS X Y` | Absolute position (-100 to 100 for each axis) |
| `SPRAY ON` | `SPRAY ON` | Turn on water spray relay |
| `SPRAY OFF` | `SPRAY OFF` | Turn off water spray relay |
| `RESET` | `RESET` | Home/reset position |
| `STOP` | `STOP` | Emergency stop all motors |

### Response Format

- Success: `OK <COMMAND>`
- Error: `ERROR: <message>`

### Examples

```
MOVE 50 -30    → Move pan right 50%, tilt down 30%
POS 0 0        → Move to center position
SPRAY ON       → Turn on water spray
SPRAY OFF      → Turn off water spray
RESET          → Reset to home position
STOP           → Emergency stop
```

## Setup Instructions

### Using PlatformIO (Recommended)

1. Install PlatformIO:
   ```bash
   pip install platformio
   ```

2. Navigate to project directory:
   ```bash
   cd space-heater-controller-main
   ```

3. Upload code:
   ```bash
   pio run --target upload
   ```

4. Monitor serial output:
   ```bash
   pio device monitor
   ```

### Using Arduino IDE

1. Install ESP8266 board support in Arduino IDE
2. Open `PanTilt_Controller.ino`
3. Select board: **NodeMCU 1.0 (ESP-12E Module)**
4. Select port: Your ESP8266 USB port
5. Upload

## Configuration

### Adjust Motor Pins

Edit pin definitions in `PanTilt_Controller.ino`:

```cpp
#define PAN_DIR_PIN 4       // Change to your pan direction pin
#define PAN_SPEED_PIN 5    // Change to your pan speed/PWM pin
#define TILT_DIR_PIN 12    // Change to your tilt direction pin
#define TILT_SPEED_PIN 13  // Change to your tilt speed/PWM pin
```

### Adjust Serial Baud Rate

Default is 9600 baud (matches HARC backend). To change:

1. In `PanTilt_Controller.ino`: Change `Serial.begin(9600);`
2. In HARC `hardware_config.json`: Update `baudrate` for `motor_controller`

## Motor Driver Compatibility

This code is designed for:
- **DC Motors with H-Bridge**: Uses direction pin + PWM speed control
- **Stepper Motors**: Can be adapted for step/direction control

### For Stepper Motors

Modify `controlPanMotor()` and `controlTiltMotor()` functions to generate step pulses instead of PWM. Example:

```cpp
void controlPanMotor(int speed) {
  if (speed != 0) {
    // Generate step pulses
    digitalWrite(PAN_DIR_PIN, speed > 0 ? HIGH : LOW);
    for (int i = 0; i < abs(speed); i++) {
      digitalWrite(PAN_SPEED_PIN, HIGH);
      delayMicroseconds(500);
      digitalWrite(PAN_SPEED_PIN, LOW);
      delayMicroseconds(500);
    }
  }
}
```

## Integration with HARC

1. Connect ESP8266 to computer via USB
2. Identify serial port: `/dev/ttyUSB1` (Linux) or `COM3` (Windows)
3. Update HARC `hardware_config.json`:
   ```json
   {
     "motor_controller": {
       "enabled": true,
       "path": "/dev/ttyUSB1",
       "type": "serial",
       "baudrate": 9600
     }
   }
   ```
4. HARC backend will automatically connect and send commands

## Mode Compatibility

This firmware is **mode-agnostic** and works with all HARC system modes:

- **Manual Mode**: Receives direct joystick commands
- **Manual + Aim-Bot Mode**: Receives pre-blended commands (joystick + AI assistance blended in HARC software)
- **Full Auto Mode**: Receives AI-generated movement commands

**Important**: The ESP8266 doesn't need to know which mode is active. All mode logic (including aim-bot assistance blending) happens in the HARC software layer before commands are sent to hardware. The firmware just executes whatever movement commands it receives.

## Troubleshooting

### ESP8266 Not Responding

- Check USB connection
- Verify serial port in HARC config matches actual port
- Check baud rate matches (9600)
- Monitor serial output: `pio device monitor` or Arduino Serial Monitor

### Motors Not Moving

- Verify motor driver connections
- Check pin definitions match your wiring
- Test with simple commands: `MOVE 50 0` (pan only)
- Verify power supply to motors

### Commands Not Recognized

- Ensure commands are uppercase: `MOVE` not `move`
- Check for newline characters (commands end with `\n`)
- Monitor serial output to see received commands

## Wiring Example

```
ESP8266          Motor Driver
--------          -----------
Pin 4     ----->  Pan Direction Pin
Pin 5     ----->  Pan Speed/PWM Pin
Pin 12    ----->  Tilt Direction Pin
Pin 13    ----->  Tilt Speed/PWM Pin
Pin 16    ----->  Relay Module (IN)
GND       ----->  Motor Driver GND, Relay GND
5V/3.3V   ----->  Motor Driver VCC (if needed)
```

## License

Private project - All rights reserved

---

**Version**: 2.0.0  
**Last Updated**: December 2024  
**Compatible with**: HARC System v1.0.0+
