/*
 * HARC Pan-Tilt Motor Controller
 * 
 * ESP8266 firmware for controlling pan-tilt mechanism via serial commands from HARC software.
 * Receives commands via Serial (USB) and controls motors + water spray relay.
 * 
 * This firmware is mode-agnostic - it doesn't know about Manual/Manual+Aim-Bot/Full Auto modes.
 * All mode logic (including aim-bot assistance) happens in HARC software layer.
 * This firmware just executes movement commands sent from HARC.
 * 
 * Serial Protocol (9600 baud):
 * - MOVE X Y        : Relative movement (-100 to 100 for each axis)
 * - POS X Y         : Absolute position (-100 to 100 for each axis)
 * - SPRAY ON        : Turn on water spray relay
 * - SPRAY OFF       : Turn off water spray relay
 * - RESET           : Home/reset position
 * - STOP            : Emergency stop all motors
 * 
 * Hardware:
 * - Pin 16: Water spray relay control
 * - Pin 4:  Pan motor direction (or step pin for stepper)
 * - Pin 5:  Pan motor speed/pulse (or direction pin for stepper)
 * - Pin 12: Tilt motor direction (or step pin for stepper)
 * - Pin 13: Tilt motor speed/pulse (or direction pin for stepper)
 * 
 * Note: Adjust pin definitions based on your motor driver setup
 * 
 * Compatible with all HARC modes:
 * - Manual: Direct joystick commands
 * - Manual + Aim-Bot: Blended joystick + AI commands (blending done in HARC)
 * - Full Auto: AI-generated commands
 */

// Pin Definitions
#define RELAY_PIN 16        // Water spray relay
#define PAN_DIR_PIN 4       // Pan motor direction (adjust for your setup)
#define PAN_SPEED_PIN 5    // Pan motor speed/PWM (adjust for your setup)
#define TILT_DIR_PIN 12    // Tilt motor direction (adjust for your setup)
#define TILT_SPEED_PIN 13   // Tilt motor speed/PWM (adjust for your setup)

// Motor control variables
float current_pan_position = 0.0;   // Current pan position (-100 to 100)
float current_tilt_position = 0.0;  // Current tilt position (-100 to 100)
bool spray_active = false;          // Water spray state

// Serial command buffer
String commandBuffer = "";
const int MAX_BUFFER_SIZE = 64;

void setup() {
  // Initialize serial communication (9600 baud to match HARC)
  Serial.begin(9600);
  delay(100);
  
  // Initialize pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PAN_DIR_PIN, OUTPUT);
  pinMode(PAN_SPEED_PIN, OUTPUT);
  pinMode(TILT_DIR_PIN, OUTPUT);
  pinMode(TILT_SPEED_PIN, OUTPUT);
  
  // Initialize to safe state
  digitalWrite(RELAY_PIN, LOW);
  stop_all_motors();
  
  // Send ready message
  Serial.println("HARC Pan-Tilt Controller Ready");
  Serial.println("Commands: MOVE, POS, SPRAY ON/OFF, RESET, STOP");
}

void loop() {
  // Read serial commands
  if (Serial.available() > 0) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      // Command complete, process it
      if (commandBuffer.length() > 0) {
        processCommand(commandBuffer);
        commandBuffer = "";
      }
    } else {
      // Add character to buffer
      if (commandBuffer.length() < MAX_BUFFER_SIZE) {
        commandBuffer += c;
      }
    }
  }
  
  // Update motor positions (if using continuous movement)
  updateMotors();
}

void processCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();
  
  // Parse command
  if (cmd.startsWith("MOVE")) {
    // Format: MOVE X Y
    int xIndex = cmd.indexOf(' ');
    int yIndex = cmd.indexOf(' ', xIndex + 1);
    
    if (xIndex > 0 && yIndex > 0) {
      int xSpeed = cmd.substring(xIndex + 1, yIndex).toInt();
      int ySpeed = cmd.substring(yIndex + 1).toInt();
      
      moveCannon(xSpeed, ySpeed);
      Serial.println("OK MOVE");
    } else {
      Serial.println("ERROR: Invalid MOVE format");
    }
  }
  else if (cmd.startsWith("POS")) {
    // Format: POS X Y
    int xIndex = cmd.indexOf(' ');
    int yIndex = cmd.indexOf(' ', xIndex + 1);
    
    if (xIndex > 0 && yIndex > 0) {
      int xPos = cmd.substring(xIndex + 1, yIndex).toInt();
      int yPos = cmd.substring(yIndex + 1).toInt();
      
      setCannonPosition(xPos, yPos);
      Serial.println("OK POS");
    } else {
      Serial.println("ERROR: Invalid POS format");
    }
  }
  else if (cmd == "SPRAY ON") {
    startSpray();
    Serial.println("OK SPRAY ON");
  }
  else if (cmd == "SPRAY OFF") {
    stopSpray();
    Serial.println("OK SPRAY OFF");
  }
  else if (cmd == "RESET") {
    resetPosition();
    Serial.println("OK RESET");
  }
  else if (cmd == "STOP") {
    stop_all_motors();
    Serial.println("OK STOP");
  }
  else {
    Serial.println("ERROR: Unknown command");
  }
}

void moveCannon(int xSpeed, int ySpeed) {
  // Clamp speeds to valid range
  xSpeed = constrain(xSpeed, -100, 100);
  ySpeed = constrain(ySpeed, -100, 100);
  
  // Update positions (scaled) - tracks position for feedback
  // Note: This is approximate tracking. For precise position, use encoders/feedback
  current_pan_position += xSpeed * 0.1;
  current_tilt_position += ySpeed * 0.1;
  
  // Clamp positions
  current_pan_position = constrain(current_pan_position, -100.0, 100.0);
  current_tilt_position = constrain(current_tilt_position, -100.0, 100.0);
  
  // Control motors based on speed
  // This works for all modes:
  // - Manual: Direct joystick input
  // - Manual + Aim-Bot: Pre-blended input from HARC (joystick + AI assistance)
  // - Full Auto: AI-generated movement commands
  controlPanMotor(xSpeed);
  controlTiltMotor(ySpeed);
}

void setCannonPosition(int x, int y) {
  // Clamp positions
  x = constrain(x, -100, 100);
  y = constrain(y, -100, 100);
  
  // Update current positions
  current_pan_position = x;
  current_tilt_position = y;
  
  // Calculate required movement
  int panSpeed = 0;  // Will be calculated if needed
  int tiltSpeed = 0; // Will be calculated if needed
  
  // For now, just set position directly
  // In a real implementation, you might want to move gradually to position
  controlPanMotor(0);  // Stop pan
  controlTiltMotor(0); // Stop tilt
}

void controlPanMotor(int speed) {
  // Control pan motor based on speed (-100 to 100)
  if (speed == 0) {
    // Stop motor
    digitalWrite(PAN_SPEED_PIN, LOW);
    digitalWrite(PAN_DIR_PIN, LOW);
  } else if (speed > 0) {
    // Move right/positive
    digitalWrite(PAN_DIR_PIN, HIGH);
    analogWrite(PAN_SPEED_PIN, map(abs(speed), 0, 100, 0, 255));
  } else {
    // Move left/negative
    digitalWrite(PAN_DIR_PIN, LOW);
    analogWrite(PAN_SPEED_PIN, map(abs(speed), 0, 100, 0, 255));
  }
}

void controlTiltMotor(int speed) {
  // Control tilt motor based on speed (-100 to 100)
  if (speed == 0) {
    // Stop motor
    digitalWrite(TILT_SPEED_PIN, LOW);
    digitalWrite(TILT_DIR_PIN, LOW);
  } else if (speed > 0) {
    // Move up/positive
    digitalWrite(TILT_DIR_PIN, HIGH);
    analogWrite(TILT_SPEED_PIN, map(abs(speed), 0, 100, 0, 255));
  } else {
    // Move down/negative
    digitalWrite(TILT_DIR_PIN, LOW);
    analogWrite(TILT_SPEED_PIN, map(abs(speed), 0, 100, 0, 255));
  }
}

void startSpray() {
  spray_active = true;
  digitalWrite(RELAY_PIN, HIGH);
}

void stopSpray() {
  spray_active = false;
  digitalWrite(RELAY_PIN, LOW);
}

void resetPosition() {
  // Stop all motors
  stop_all_motors();
  
  // Reset positions to center
  current_pan_position = 0.0;
  current_tilt_position = 0.0;
  
  // Move to home position (implement based on your hardware)
  // For now, just reset the position variables
}

void stop_all_motors() {
  // Emergency stop - stop all motors immediately
  controlPanMotor(0);
  controlTiltMotor(0);
  stopSpray();
}

void updateMotors() {
  // This function can be used for continuous position tracking
  // or for stepper motor step generation
  // Currently motors are controlled directly in moveCannon/setCannonPosition
}

