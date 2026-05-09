#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h> 
#include <BluetoothSerial.h> // <--- NEW: Bluetooth Library

// --- Hardware Pin Definitions ---
const int RELAY_1_PIN = 4;    
const int RELAY_2_PIN = 5;    
const int BUTTON_1_PIN = 6;    
const int BUTTON_2_PIN = 7;    
const int MENU_BUTTON_PIN = 9; 
const int MOTOR_PIN = 10;      
const int RGB_LED_PIN = 48;   

// --- Initialize Components ---
Adafruit_NeoPixel rgbLed(1, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);
Preferences preferences; 
BluetoothSerial SerialBT; // <--- NEW: Bluetooth Object

// --- Wi-Fi Configuration Variables ---
String currentSSID = "";
String currentPassword = "";
String currentIPStr = "";
WiFiServer server(8080); 

// --- Unified Multiplayer Client Arrays (6 Slots Total) ---
// Slots 0-3 = Wi-Fi | Slot 4 = Bluetooth | Slot 5 = USB
WiFiClient wifiClients[4];
String clientNames[6]; 
String clientBuffers[6]; 
bool btConnected = false; // Tracks if someone is connected via BT

// ==========================================
// OS STATE MACHINE
// 0=Menu, 1=Home, 2=Morse1, 3=Morse2, 4=Motor, 5=WiFi Config
// ==========================================
int currentApp = 0; 

// --- Hardware Menu System Variables ---
bool inPreviewMode = false; int previewApp = 1;
unsigned long lastBlinkTime = 0; bool blinkState = false;
int menuBtnState = HIGH; int lastMenuBtnFlicker = HIGH;
unsigned long lastMenuDebounce = 0; unsigned long menuBtnPressTime = 0;
bool menuBtnIsPressing = false; bool menuLongPressTriggered = false;

// --- Smart Home Variables ---
bool targetState1 = false; bool targetState2 = false;
bool policeModeActive = false;
unsigned long lastPoliceBlinkTime = 0; bool policeBlinkState = false;
int shBtnState1 = HIGH; int shLastFlicker1 = HIGH; unsigned long shDebTime1 = 0;
unsigned long shBtnPressTime1 = 0; bool shBtnIsPressing1 = false; bool shBtnLongTriggered1 = false;
int shBtnState2 = HIGH; int shLastFlicker2 = HIGH; unsigned long shDebTime2 = 0;
unsigned long shBtnPressTime2 = 0; bool shBtnIsPressing2 = false; bool shBtnLongTriggered2 = false;

// --- Motor Controller Variables ---
int targetMotorSpeed = 0; int currentMotorSpeed = 0; 
bool isKickstarting = false; unsigned long kickstartTimer = 0;
int buttonState1 = HIGH; int lastFlicker1 = HIGH; unsigned long lastDebTime1 = 0;
int buttonState2 = HIGH; int lastFlicker2 = HIGH; unsigned long lastDebTime2 = 0;
const unsigned long debounceDelay = 50; 

// --- Morse Code Variables ---
const int dotLen = 200; const int dashLen = dotLen * 3;   
const int elemGap = dotLen; const int charGap = dotLen * 3; const int wordGap = dotLen * 7;   
int morseBtnState1 = HIGH; int lastMorseBtnState1 = HIGH; unsigned long lastMorseDebounce1 = 0; bool isPressing1 = false;
int morseBtnState2 = HIGH; int lastMorseBtnState2 = HIGH; unsigned long lastMorseDebounce2 = 0; bool isPressing2 = false;
unsigned long btnPressTime = 0; unsigned long btnReleaseTime = 0;
bool letterPending = false; bool spacePending = false;
String currentMorseSequence = ""; bool hwIsTyping = false; 

// --- Wi-Fi Config Variables ---
int wifiConfigStep = 0; String newSSID = ""; String newPASS = ""; String newIP = "";

// --- Function Prototypes ---
void printMenu(Print& c); void printSmartHomeUI(Print& c);
void printMorse1UI(Print& c); void printMorse2UI(Print& c);
void printMotorUI(Print& c); void printWiFiConfigUI(Print& c);
void broadcastMenu(); void broadcastSmartHomeUI(); void broadcastMorse1UI();
void broadcastMorse2UI(); void broadcastMotorUI(); void broadcastWiFiConfigUI();
void broadcast(String msg); void broadcastLine(String msg); void broadcastEmptyLine();
void reprintPrompt(); void handleInput(String input, int id, Print& out);
void readStream(Stream& stream, int id);
void runSmartHome(); void runMorseCode1Button(); void runMorseCode2Button();
void runMotorController(); void flashMorse(String sequence);
void translateToMorse(char c); char translateFromMorse(String seq);
void setLED(int r, int g, int b); void confirmAppSelection(int appID); 

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_1_PIN, OUTPUT); pinMode(RELAY_2_PIN, OUTPUT);
  digitalWrite(RELAY_1_PIN, LOW); digitalWrite(RELAY_2_PIN, LOW); 
  pinMode(MOTOR_PIN, OUTPUT); analogWrite(MOTOR_PIN, 0); 
  pinMode(BUTTON_1_PIN, INPUT_PULLUP); pinMode(BUTTON_2_PIN, INPUT_PULLUP); 
  pinMode(MENU_BUTTON_PIN, INPUT_PULLUP); 
  delay(100); 

  rgbLed.begin(); rgbLed.setBrightness(100); setLED(0, 0, 0); 

  // ==========================================
  // HARDWARE FACTORY RESET LOGIC
  // ==========================================
  if (digitalRead(MENU_BUTTON_PIN) == LOW) {
    Serial.println("!!! FACTORY RESET TRIGGERED !!!");
    preferences.begin("wifi_config", false); preferences.clear(); preferences.end();
    for(int i=0; i<15; i++) { setLED(255, 0, 0); delay(50); setLED(0, 0, 0); delay(50); }
  }

  preferences.begin("wifi_config", false);
  currentSSID = preferences.getString("ssid", "ESP32_Master_Node"); 
  currentPassword = preferences.getString("password", "adminpassword");
  currentIPStr = preferences.getString("ip", "192.168.4.1");
  preferences.end();

  // --- START BLUETOOTH & WIFI ---
  SerialBT.begin("ESP32_Batmobile_BT"); // The name that will show up on your phone's Bluetooth!

  IPAddress local_ip;
  if (!local_ip.fromString(currentIPStr)) { local_ip.fromString("192.168.4.1"); currentIPStr = "192.168.4.1"; }
  IPAddress gateway = local_ip; IPAddress subnet(255, 255, 255, 0);
  
  WiFi.softAPConfig(local_ip, gateway, subnet); 
  WiFi.softAP(currentSSID.c_str(), currentPassword.c_str(), 1, 0, 4); // Max 4 WiFi users
  server.begin();

  // --- INITIALIZE USB TERMINAL (Slot 5) ---
  Serial.println();
  Serial.println("==================================");
  Serial.println("       ESP32 MULTIPLAYER OS       ");
  Serial.println("==================================");
  Serial.println("Welcome to the local USB Terminal!");
  Serial.println("Please type your Username and press Enter:");
}

void loop() {
  unsigned long currentMillis = millis();

  // ==========================================
  // 1. WI-FI CONNECTION HANDLER (Slots 0-3)
  // ==========================================
  if (server.hasClient()) {
    bool connectionAccepted = false;
    for (int i = 0; i < 4; i++) {
      if (!wifiClients[i] || !wifiClients[i].connected()) {
        if (wifiClients[i]) wifiClients[i].stop(); 
        wifiClients[i] = server.available(); 
        clientNames[i] = ""; clientBuffers[i] = ""; 
        wifiClients[i].println("\n==================================\n       ESP32 MULTIPLAYER OS       \n==================================\nWelcome! Please type your Username and press Send:");
        connectionAccepted = true; break; 
      }
    }
    if (!connectionAccepted) { WiFiClient rejectClient = server.available(); rejectClient.println("Server is full."); rejectClient.stop(); }
  }

  // ==========================================
  // 1.5 DISCONNECTION TRACKER (WiFi & Bluetooth)
  // ==========================================
  // Tracker for Wi-Fi Users
  for (int i = 0; i < 4; i++) {
    if (wifiClients[i] && !wifiClients[i].connected()) {
      if (clientNames[i] != "") {
        String departedUser = clientNames[i];
        clientNames[i] = ""; clientBuffers[i] = ""; wifiClients[i].stop();
        broadcastEmptyLine(); broadcastLine("*** [" + departedUser + "] left the Wi-Fi server! ***");
        reprintPrompt();
      } else { wifiClients[i].stop(); clientNames[i] = ""; clientBuffers[i] = ""; }
    }
  }

  // Tracker for Bluetooth User (Slot 4)
  if (SerialBT.hasClient() && !btConnected) {
    btConnected = true;
    clientNames[4] = ""; clientBuffers[4] = "";
    SerialBT.println("\n==================================\n       ESP32 MULTIPLAYER OS       \n==================================\nWelcome to Bluetooth Terminal! Please type your Username and press Send:");
  } 
  else if (!SerialBT.hasClient() && btConnected) {
    btConnected = false;
    if (clientNames[4] != "") {
      String departedUser = clientNames[4];
      clientNames[4] = ""; clientBuffers[4] = "";
      broadcastEmptyLine(); broadcastLine("*** [" + departedUser + "] disconnected from Bluetooth! ***");
      reprintPrompt();
    }
  }

  // ==========================================
  // 2. UNIFIED INPUT READER (Non-Blocking)
  // ==========================================
  // Read from Wi-Fi users
  for (int i = 0; i < 4; i++) { if (wifiClients[i] && wifiClients[i].connected()) readStream(wifiClients[i], i); }
  
  // Read from Bluetooth user
  if (SerialBT.hasClient()) readStream(SerialBT, 4);

  // Read from Local USB user
  readStream(Serial, 5);

  // ==========================================
  // 3. HARDWARE MENU BUTTON LOGIC (GPIO 9)
  // ==========================================
  int rawMenu = digitalRead(MENU_BUTTON_PIN);
  if (rawMenu != lastMenuBtnFlicker) lastMenuDebounce = currentMillis;
  if (currentMillis - lastMenuDebounce > 20) {
    if (rawMenu != menuBtnState) { menuBtnState = rawMenu;
      if (menuBtnState == LOW) { menuBtnPressTime = currentMillis; menuBtnIsPressing = true; menuLongPressTriggered = false; } 
      else { 
        if (menuBtnIsPressing && !menuLongPressTriggered) {
          if (!inPreviewMode) { inPreviewMode = true; previewApp = (currentApp == 0) ? 1 : (currentApp % 5) + 1; } else { previewApp = (previewApp % 5) + 1; }
          digitalWrite(RELAY_1_PIN, LOW); digitalWrite(RELAY_2_PIN, LOW); setLED(0, 0, 0); policeModeActive = false;
        }
        menuBtnIsPressing = false;
      }
    }
  }
  lastMenuBtnFlicker = rawMenu;

  if (menuBtnIsPressing && !menuLongPressTriggered && (currentMillis - menuBtnPressTime > 800)) {
    menuLongPressTriggered = true;
    if (inPreviewMode) {
      if (currentApp == 4 && previewApp != 4) { targetMotorSpeed = 0; currentMotorSpeed = 0; isKickstarting = false; analogWrite(MOTOR_PIN, 0); }
      if (currentApp == 5 && previewApp != 5) { wifiConfigStep = 0; } 
      currentApp = previewApp; inPreviewMode = false; policeModeActive = false; confirmAppSelection(currentApp);
      
      broadcastEmptyLine();
      if (currentApp == 1) { broadcastLine("*** [Hardware Menu] switched to Smart Home ***"); broadcastSmartHomeUI(); }
      else if (currentApp == 2) { broadcastLine("*** [Hardware Menu] switched to 1-Button Telegraph ***"); broadcastMorse1UI(); } 
      else if (currentApp == 3) { broadcastLine("*** [Hardware Menu] switched to 2-Button Telegraph ***"); broadcastMorse2UI(); }
      else if (currentApp == 4) { broadcastLine("*** [Hardware Menu] switched to Motor Controller ***"); broadcastMotorUI(); }
      else if (currentApp == 5) { broadcastLine("*** [Hardware Menu] switched to Wi-Fi Configurator ***"); broadcastWiFiConfigUI(); wifiConfigStep = 0; }
    } else { inPreviewMode = true; previewApp = (currentApp == 0) ? 1 : currentApp; }
  }

  if (inPreviewMode) {
    if (currentMillis - lastBlinkTime > 150) { 
      lastBlinkTime = currentMillis; blinkState = !blinkState;
      if (previewApp == 1) { digitalWrite(RELAY_1_PIN, blinkState ? HIGH : LOW); digitalWrite(RELAY_2_PIN, blinkState ? HIGH : LOW); setLED(0, 0, 0); }
      else if (previewApp == 2) { digitalWrite(RELAY_1_PIN, LOW); digitalWrite(RELAY_2_PIN, LOW); if (blinkState) setLED(255, 0, 0); else setLED(0, 0, 0); }
      else if (previewApp == 3) { digitalWrite(RELAY_1_PIN, LOW); digitalWrite(RELAY_2_PIN, LOW); if (blinkState) setLED(255, 100, 0); else setLED(0, 0, 0); }
      else if (previewApp == 4) { digitalWrite(RELAY_1_PIN, LOW); digitalWrite(RELAY_2_PIN, LOW); if (blinkState) setLED(255, 255, 255); else setLED(0, 0, 0); }
      else if (previewApp == 5) { digitalWrite(RELAY_1_PIN, LOW); digitalWrite(RELAY_2_PIN, LOW); if (blinkState) setLED(0, 255, 255); else setLED(0, 0, 0); } 
    }
    return; 
  }

  // ==========================================
  // 4. PROCESS HARDWARE APPS
  // ==========================================
  if (currentApp == 1) runSmartHome();
  else if (currentApp == 2) runMorseCode1Button();
  else if (currentApp == 3) runMorseCode2Button();
  else if (currentApp == 4) runMotorController();
}

// ==========================================
// UNIFIED COMMAND PROCESSING LOGIC
// ==========================================
// This extracts characters from any stream (WiFi, BT, or USB)
void readStream(Stream& stream, int id) {
  while (stream.available()) {
    char c = stream.read();
    if (c == 8 || c == 127) { if (clientBuffers[id].length() > 0) clientBuffers[id].remove(clientBuffers[id].length() - 1); }
    else if (c == '\n' || c == '\r') {
      String input = clientBuffers[id]; clientBuffers[id] = ""; input.trim();
      if (input.length() > 0) handleInput(input, id, stream);
    } 
    else if (c >= 32 && c <= 126) { clientBuffers[id] += c; }
  }
}

// This processes the commands using standard Print references so it works on everything!
void handleInput(String input, int id, Print& out) {
  if (clientNames[id] == "") {
    clientNames[id] = input;
    out.println(); out.println("Welcome to the server, " + clientNames[id] + "!");
    broadcastEmptyLine(); broadcastLine("*** [" + clientNames[id] + "] joined the server! ***");
    if (currentApp == 0) printMenu(out);
    else if (currentApp == 1) printSmartHomeUI(out);
    else if (currentApp == 2) printMorse1UI(out);
    else if (currentApp == 3) printMorse2UI(out);
    else if (currentApp == 4) printMotorUI(out);
    else if (currentApp == 5) printWiFiConfigUI(out);
    return; 
  }

  if (input == "*") {
    if (currentApp == 4) { targetMotorSpeed = 0; currentMotorSpeed = 0; analogWrite(MOTOR_PIN, 0); }
    currentApp = 0; inPreviewMode = false; setLED(0, 0, 0); policeModeActive = false; wifiConfigStep = 0;
    broadcastEmptyLine(); broadcastLine("*** [" + clientNames[id] + "] returned the system to the Main Menu ***");
    broadcastMenu(); return; 
  }

  if (currentApp == 0) {
    if (input == "1") { currentApp = 1; broadcastEmptyLine(); broadcastLine("*** [" + clientNames[id] + "] launched Smart Home ***"); broadcastSmartHomeUI(); confirmAppSelection(1); }
    else if (input == "2") { currentApp = 2; broadcastEmptyLine(); broadcastLine("*** [" + clientNames[id] + "] launched 1-Button Telegraph ***"); broadcastMorse1UI(); confirmAppSelection(2); }
    else if (input == "3") { currentApp = 3; broadcastEmptyLine(); broadcastLine("*** [" + clientNames[id] + "] launched 2-Button Telegraph ***"); broadcastMorse2UI(); confirmAppSelection(3); }
    else if (input == "4") { currentApp = 4; broadcastEmptyLine(); broadcastLine("*** [" + clientNames[id] + "] launched Motor Controller ***"); broadcastMotorUI(); confirmAppSelection(4); }
    else if (input == "5") { currentApp = 5; wifiConfigStep = 0; broadcastEmptyLine(); broadcastLine("*** [" + clientNames[id] + "] launched Wi-Fi Configurator ***"); broadcastWiFiConfigUI(); confirmAppSelection(5); }
  }
  else if (currentApp == 1) {
    if (input == "P" || input == "p") { policeModeActive = !policeModeActive; broadcastLine("[" + clientNames[id] + "] commanded Police Mode: [" + (policeModeActive ? "ON" : "OFF") + "]"); }
    else if (input == "A" || input == "a") { if (policeModeActive) { policeModeActive = false; broadcastLine("[System] Police Mode Interrupted"); } targetState1 = !targetState1; broadcastLine("[" + clientNames[id] + "]  -> LED 1: [" + (targetState1 ? "ON" : "OFF") + "]"); } 
    else if (input == "B" || input == "b") { if (policeModeActive) { policeModeActive = false; broadcastLine("[System] Police Mode Interrupted"); } targetState2 = !targetState2; broadcastLine("[" + clientNames[id] + "]  -> LED 2: [" + (targetState2 ? "ON" : "OFF") + "]"); }
  }
  else if (currentApp == 2 || currentApp == 3) {
    input.toUpperCase(); broadcastEmptyLine(); broadcastLine("[" + clientNames[id] + " transmits]: " + input); 
    for (int j = 0; j < input.length(); j++) translateToMorse(input[j]);
    setLED(128, 0, 128); delay(1000); setLED(0, 0, 0); broadcastEmptyLine(); broadcast("[Morse]> "); 
  }
  else if (currentApp == 4) {
    int requestedSpeed = input.toInt();
    if (input == "0" || (requestedSpeed > 0 && requestedSpeed <= 100)) { targetMotorSpeed = requestedSpeed; broadcastLine("[" + clientNames[id] + "] commanded motor to: " + String(targetMotorSpeed) + "%"); } 
    else { out.println("[Error] Please send a number from 0 to 100"); }
  }
  else if (currentApp == 5) { 
    if (wifiConfigStep == 0) {
      newSSID = input; wifiConfigStep = 1; out.println("[System] Password must be 8+ chars:"); out.print("[Config]> ");
    } else if (wifiConfigStep == 1) {
      if (input.length() < 8) { out.println("[Error] Password too short!"); out.print("[Config]> "); } 
      else {
        newPASS = input; wifiConfigStep = 2; out.println("[System] Password accepted.");
        out.println("Type new IP address (e.g., 192.168.10.1)\nOr type 'D' to keep default:"); out.print("[Config]> ");
      }
    } else if (wifiConfigStep == 2) {
      if (input == "D" || input == "d") { newIP = "192.168.4.1"; } 
      else {
        IPAddress checkIP; if (checkIP.fromString(input)) { newIP = input; } else { out.println("[Error] Invalid IP! Try again:"); out.print("[Config]> "); return; }
      }
      wifiConfigStep = 3; out.println(); out.println("=================================="); out.println("PLEASE CONFIRM YOUR CREDENTIALS:");
      out.println("SSID: [" + newSSID + "]\nPASS: [" + newPASS + "]\n IP : [" + newIP + "]");
      out.println("Are these correct? Type Y (Yes) or N (No):"); out.print("[Config]> ");
    } else if (wifiConfigStep == 3) {
      if (input == "Y" || input == "y") {
        out.println(); out.println("*** SAVING TO PERMANENT MEMORY ***");
        preferences.begin("wifi_config", false); preferences.putString("ssid", newSSID); preferences.putString("password", newPASS); preferences.putString("ip", newIP); preferences.end();
        out.println("SUCCESS. Rebooting in 3 seconds..."); delay(3000); ESP.restart(); 
      } else { out.println(); out.println("Cancelled! Please type the new Network Name (SSID):"); out.print("[Config]> "); wifiConfigStep = 0; }
    }
  }
}

// ==========================================
// VISUAL CONFIRMATION FUNCTION
// ==========================================
void confirmAppSelection(int appID) {
  if (appID == 1) { for (int i = 0; i < 2; i++) { digitalWrite(RELAY_1_PIN, HIGH); digitalWrite(RELAY_2_PIN, HIGH); delay(100); digitalWrite(RELAY_1_PIN, LOW); digitalWrite(RELAY_2_PIN, LOW); delay(100); } digitalWrite(RELAY_1_PIN, targetState1 ? HIGH : LOW); digitalWrite(RELAY_2_PIN, targetState2 ? HIGH : LOW); } 
  else if (appID == 2) { digitalWrite(RELAY_1_PIN, LOW); digitalWrite(RELAY_2_PIN, LOW); for (int i = 0; i < 2; i++) { setLED(255, 0, 0); delay(100); setLED(0, 0, 0); delay(100); } } 
  else if (appID == 3) { digitalWrite(RELAY_1_PIN, LOW); digitalWrite(RELAY_2_PIN, LOW); for (int i = 0; i < 2; i++) { setLED(255, 100, 0); delay(100); setLED(0, 0, 0); delay(100); } }
  else if (appID == 4) { digitalWrite(RELAY_1_PIN, LOW); digitalWrite(RELAY_2_PIN, LOW); for (int i = 0; i < 2; i++) { setLED(255, 255, 255); delay(100); setLED(0, 0, 0); delay(100); } }
  else if (appID == 5) { digitalWrite(RELAY_1_PIN, LOW); digitalWrite(RELAY_2_PIN, LOW); for (int i = 0; i < 2; i++) { setLED(0, 255, 255); delay(100); setLED(0, 0, 0); delay(100); } }
}

// ==========================================
// APP LOGIC FUNCTIONS
// ==========================================
void runSmartHome() {
  unsigned long currentMillis = millis();
  int raw1 = digitalRead(BUTTON_1_PIN);
  if (raw1 != shLastFlicker1) shDebTime1 = currentMillis;
  if ((currentMillis - shDebTime1) > 20) {
    if (raw1 != shBtnState1) { shBtnState1 = raw1;
      if (shBtnState1 == LOW) { shBtnPressTime1 = currentMillis; shBtnIsPressing1 = true; shBtnLongTriggered1 = false; } 
      else { 
        if (shBtnIsPressing1 && !shBtnLongTriggered1) {
          if (policeModeActive) { policeModeActive = false; broadcastLine("[Hardware Key] Police Mode OFF"); } 
          else { targetState1 = !targetState1; broadcastLine("[Hardware Key] -> LED 1: [" + String(targetState1 ? "ON" : "OFF") + "]"); }
        }
        shBtnIsPressing1 = false;
      }
    }
  }
  shLastFlicker1 = raw1;
  if (shBtnIsPressing1 && !shBtnLongTriggered1 && (currentMillis - shBtnPressTime1 > 800)) { shBtnLongTriggered1 = true; policeModeActive = !policeModeActive; broadcastLine("[Hardware Key] Police Mode: [" + String(policeModeActive ? "ON" : "OFF") + "]"); }

  int raw2 = digitalRead(BUTTON_2_PIN);
  if (raw2 != shLastFlicker2) shDebTime2 = currentMillis;
  if ((currentMillis - shDebTime2) > 20) {
    if (raw2 != shBtnState2) { shBtnState2 = raw2;
      if (shBtnState2 == LOW) { shBtnPressTime2 = currentMillis; shBtnIsPressing2 = true; shBtnLongTriggered2 = false; } 
      else { 
        if (shBtnIsPressing2 && !shBtnLongTriggered2) {
          if (policeModeActive) { policeModeActive = false; broadcastLine("[Hardware Key] Police Mode OFF"); } 
          else { targetState2 = !targetState2; broadcastLine("[Hardware Key] -> LED 2: [" + String(targetState2 ? "ON" : "OFF") + "]"); }
        }
        shBtnIsPressing2 = false;
      }
    }
  }
  shLastFlicker2 = raw2;
  if (shBtnIsPressing2 && !shBtnLongTriggered2 && (currentMillis - shBtnPressTime2 > 800)) { shBtnLongTriggered2 = true; policeModeActive = !policeModeActive; broadcastLine("[Hardware Key] Police Mode: [" + String(policeModeActive ? "ON" : "OFF") + "]"); }

  if (policeModeActive) {
    if (currentMillis - lastPoliceBlinkTime > 150) {
      lastPoliceBlinkTime = currentMillis; policeBlinkState = !policeBlinkState;
      digitalWrite(RELAY_1_PIN, policeBlinkState ? HIGH : LOW); digitalWrite(RELAY_2_PIN, policeBlinkState ? LOW : HIGH);
    }
  } else { digitalWrite(RELAY_1_PIN, targetState1 ? HIGH : LOW); digitalWrite(RELAY_2_PIN, targetState2 ? HIGH : LOW); }
}

void runMotorController() {
  unsigned long currentMillis = millis(); int raw1 = digitalRead(BUTTON_1_PIN); int raw2 = digitalRead(BUTTON_2_PIN); 
  if (raw1 != lastFlicker1) lastDebTime1 = currentMillis; 
  if ((currentMillis - lastDebTime1) > debounceDelay) {
    if (raw1 != buttonState1) { buttonState1 = raw1; if (buttonState1 == LOW) { targetMotorSpeed = min(100, targetMotorSpeed + 10); broadcastLine("[Hardware] Throttle UP: " + String(targetMotorSpeed) + "%"); } }
  }
  lastFlicker1 = raw1;

  if (raw2 != lastFlicker2) lastDebTime2 = currentMillis; 
  if ((currentMillis - lastDebTime2) > debounceDelay) {
    if (raw2 != buttonState2) { buttonState2 = raw2; if (buttonState2 == LOW) { targetMotorSpeed = max(0, targetMotorSpeed - 10); broadcastLine("[Hardware] Throttle DOWN: " + String(targetMotorSpeed) + "%"); } }
  }
  lastFlicker2 = raw2;

  if (targetMotorSpeed > 0 && currentMotorSpeed == 0 && !isKickstarting) { isKickstarting = true; kickstartTimer = currentMillis; analogWrite(MOTOR_PIN, 255); broadcastLine("[System] Applying Kickstart Burst..."); }
  if (isKickstarting) {
    if (currentMillis - kickstartTimer > 50) { isKickstarting = false; currentMotorSpeed = targetMotorSpeed; int pwmValue = map(targetMotorSpeed, 0, 100, 0, 255); analogWrite(MOTOR_PIN, pwmValue);  }
  } else {
    if (currentMotorSpeed != targetMotorSpeed) { currentMotorSpeed = targetMotorSpeed; int pwmValue = map(targetMotorSpeed, 0, 100, 0, 255); analogWrite(MOTOR_PIN, pwmValue); }
  }
}

void runMorseCode1Button() {
  unsigned long currentMillis = millis(); int currentBtnState = digitalRead(BUTTON_1_PIN);
  if (currentBtnState != lastMorseBtnState1) lastMorseDebounce1 = currentMillis;
  if ((currentMillis - lastMorseDebounce1) > 20) { 
    if (currentBtnState != morseBtnState1) { morseBtnState1 = currentBtnState;
      if (morseBtnState1 == LOW) { btnPressTime = currentMillis; isPressing1 = true;
        if (currentMorseSequence == "" && (currentMillis - btnReleaseTime) > 2000 && spacePending) { broadcast(" "); setLED(255, 100, 0); delay(100); setLED(0, 0, 0); spacePending = false; }
      } else { unsigned long pressDuration = currentMillis - btnPressTime; btnReleaseTime = currentMillis; isPressing1 = false; letterPending = true; spacePending = true; setLED(0, 0, 0); 
        if (pressDuration < 300) currentMorseSequence += "."; else currentMorseSequence += "-";
      }
    }
  }
  lastMorseBtnState1 = currentBtnState;
  if (isPressing1) { if ((currentMillis - btnPressTime) < 300) setLED(0, 255, 0); else setLED(0, 0, 255); }
  if (!isPressing1 && letterPending && (currentMillis - btnReleaseTime) > 700) {
    if (!hwIsTyping) { broadcastEmptyLine(); broadcast("[Breadboard Key]: "); hwIsTyping = true; }
    broadcast(String(translateFromMorse(currentMorseSequence))); currentMorseSequence = ""; letterPending = false;
  }
  if (hwIsTyping && !isPressing1 && !letterPending && (currentMillis - btnReleaseTime) > 5000) { hwIsTyping = false; broadcastEmptyLine(); broadcast("[Morse]> "); }
}

void runMorseCode2Button() {
  unsigned long currentMillis = millis(); int raw1 = digitalRead(BUTTON_1_PIN); int raw2 = digitalRead(BUTTON_2_PIN); 
  
  if (raw1 != lastMorseBtnState1) lastMorseDebounce1 = currentMillis;
  if ((currentMillis - lastMorseDebounce1) > 20) { 
    if (raw1 != morseBtnState1) { morseBtnState1 = raw1;
      if (morseBtnState1 == LOW) { isPressing1 = true; setLED(0, 255, 0); 
        if (currentMorseSequence == "" && (currentMillis - btnReleaseTime) > 2000 && spacePending) { broadcast(" "); setLED(255, 100, 0); delay(100); setLED(0, 255, 0); spacePending = false; }
      } else { isPressing1 = false; btnReleaseTime = currentMillis; letterPending = true; spacePending = true; setLED(0, 0, 0); currentMorseSequence += "."; }
    }
  }
  lastMorseBtnState1 = raw1;

  if (raw2 != lastMorseBtnState2) lastMorseDebounce2 = currentMillis;
  if ((currentMillis - lastMorseDebounce2) > 20) { 
    if (raw2 != morseBtnState2) { morseBtnState2 = raw2;
      if (morseBtnState2 == LOW) { isPressing2 = true; setLED(0, 0, 255); 
        if (currentMorseSequence == "" && (currentMillis - btnReleaseTime) > 2000 && spacePending) { broadcast(" "); setLED(255, 100, 0); delay(100); setLED(0, 0, 255); spacePending = false; }
      } else { isPressing2 = false; btnReleaseTime = currentMillis; letterPending = true; spacePending = true; setLED(0, 0, 0); currentMorseSequence += "-"; }
    }
  }
  lastMorseBtnState2 = raw2;

  bool anyButtonPressing = isPressing1 || isPressing2;
  if (!anyButtonPressing && letterPending && (currentMillis - btnReleaseTime) > 700) {
    if (!hwIsTyping) { broadcastEmptyLine(); broadcast("[Breadboard Paddles]: "); hwIsTyping = true; }
    broadcast(String(translateFromMorse(currentMorseSequence))); currentMorseSequence = ""; letterPending = false;
  }
  if (hwIsTyping && !anyButtonPressing && !letterPending && (currentMillis - btnReleaseTime) > 5000) { hwIsTyping = false; broadcastEmptyLine(); broadcast("[Morse]> "); }
}

// ==========================================
// BROADCASTING & UI HELPERS (Polymorphic!)
// ==========================================
void broadcast(String msg) { 
  for (int i = 0; i < 4; i++) if (wifiClients[i] && wifiClients[i].connected() && clientNames[i] != "") wifiClients[i].print(msg); 
  if (SerialBT.hasClient() && clientNames[4] != "") SerialBT.print(msg);
  if (clientNames[5] != "") Serial.print(msg);
}
void broadcastLine(String msg) { 
  for (int i = 0; i < 4; i++) if (wifiClients[i] && wifiClients[i].connected() && clientNames[i] != "") wifiClients[i].println(msg); 
  if (SerialBT.hasClient() && clientNames[4] != "") SerialBT.println(msg);
  if (clientNames[5] != "") Serial.println(msg);
}
void broadcastEmptyLine() { 
  for (int i = 0; i < 4; i++) if (wifiClients[i] && wifiClients[i].connected() && clientNames[i] != "") wifiClients[i].println(); 
  if (SerialBT.hasClient() && clientNames[4] != "") SerialBT.println();
  if (clientNames[5] != "") Serial.println();
}
void reprintPrompt() {
  if (currentApp == 2 || currentApp == 3) broadcast("[Morse]> ");
  else if (currentApp == 4) broadcast("[Motor]> ");
  else if (currentApp == 5) broadcast("[Config]> ");
}

void printMenu(Print& c) {
  c.println(); c.println("=================================="); c.println("       ESP32 CHATROOM OS        "); c.println("==================================");
  c.println(" [1] Smart Home Controller\n [2] Telegraph (1-Button Mode)\n [3] Telegraph (2-Button Mode)\n [4] Motor Speed Controller\n [5] Wi-Fi Configurator\n----------------------------------\n > Select App (1-5):");
}
void printSmartHomeUI(Print& c) {
  c.println(); c.println("=================================="); c.println("      SMART HOME CONTROLLER       "); c.println("==================================");
  c.print(" Status: LED 1 ["); c.print(targetState1 ? "ON" : "OFF"); c.println("]"); c.print(" Status: LED 2 ["); c.print(targetState2 ? "ON" : "OFF"); c.println("]");
  c.println("----------------------------------\n Commands: [A] Toggle 1 | [B] Toggle 2\n           [P] Police Mode | [*] Menu");
}
void printMorse1UI(Print& c) {
  c.println(); c.println("=================================="); c.println("   TELEGRAPH (1-BUTTON MODE)      "); c.println("==================================");
  c.println(" HW: Button 1 (Short=Dot, Long=Dash)\n TX: Type text here to flash LED\n [*] Exit to Main Menu\n----------------------------------"); c.print("[Morse]> ");
}
void printMorse2UI(Print& c) {
  c.println(); c.println("=================================="); c.println("   TELEGRAPH (2-BUTTON PADDLE)    "); c.println("==================================");
  c.println(" HW: Button 1 = Dot | Button 2 = Dash\n TX: Type text here to flash LED\n [*] Exit to Main Menu\n----------------------------------"); c.print("[Morse]> ");
}
void printMotorUI(Print& c) {
  c.println(); c.println("=================================="); c.println("     MOTOR SPEED CONTROLLER       "); c.println("==================================");
  c.print(" Current Throttle: "); c.print(currentMotorSpeed); c.println("%");
  c.println("----------------------------------\n HW: Btn 1 (+10%) | Btn 2 (-10%)\n TX: Type 0 to 100 to set speed\n [*] Exit & Stop Motor\n----------------------------------"); c.print("[Motor]> ");
}
void printWiFiConfigUI(Print& c) {
  c.println(); c.println("=================================="); c.println("       WI-FI CONFIGURATOR         "); c.println("==================================");
  c.println("Please type the new Network Name\n(SSID) and press send. Or type [*]\nto cancel and exit.\n----------------------------------"); c.print("[Config]> ");
}

void broadcastMenu() { for(int i=0; i<4; i++) if(wifiClients[i] && wifiClients[i].connected() && clientNames[i]!="") printMenu(wifiClients[i]); if(SerialBT.hasClient() && clientNames[4]!="") printMenu(SerialBT); if(clientNames[5]!="") printMenu(Serial); }
void broadcastSmartHomeUI() { for(int i=0; i<4; i++) if(wifiClients[i] && wifiClients[i].connected() && clientNames[i]!="") printSmartHomeUI(wifiClients[i]); if(SerialBT.hasClient() && clientNames[4]!="") printSmartHomeUI(SerialBT); if(clientNames[5]!="") printSmartHomeUI(Serial); }
void broadcastMorse1UI() { for(int i=0; i<4; i++) if(wifiClients[i] && wifiClients[i].connected() && clientNames[i]!="") printMorse1UI(wifiClients[i]); if(SerialBT.hasClient() && clientNames[4]!="") printMorse1UI(SerialBT); if(clientNames[5]!="") printMorse1UI(Serial); }
void broadcastMorse2UI() { for(int i=0; i<4; i++) if(wifiClients[i] && wifiClients[i].connected() && clientNames[i]!="") printMorse2UI(wifiClients[i]); if(SerialBT.hasClient() && clientNames[4]!="") printMorse2UI(SerialBT); if(clientNames[5]!="") printMorse2UI(Serial); }
void broadcastMotorUI() { for(int i=0; i<4; i++) if(wifiClients[i] && wifiClients[i].connected() && clientNames[i]!="") printMotorUI(wifiClients[i]); if(SerialBT.hasClient() && clientNames[4]!="") printMotorUI(SerialBT); if(clientNames[5]!="") printMotorUI(Serial); }
void broadcastWiFiConfigUI() { for(int i=0; i<4; i++) if(wifiClients[i] && wifiClients[i].connected() && clientNames[i]!="") printWiFiConfigUI(wifiClients[i]); if(SerialBT.hasClient() && clientNames[4]!="") printWiFiConfigUI(SerialBT); if(clientNames[5]!="") printWiFiConfigUI(Serial); }

void setLED(int r, int g, int b) { rgbLed.setPixelColor(0, rgbLed.Color(r, g, b)); rgbLed.show(); }
void translateToMorse(char c) {
  if (c == ' ') { setLED(255, 100, 0); delay(wordGap); setLED(0, 0, 0); return; } 
  switch (c) {
    case 'A': flashMorse(".-"); break;   case 'B': flashMorse("-..."); break; case 'C': flashMorse("-.-."); break; case 'D': flashMorse("-.."); break; case 'E': flashMorse("."); break;    case 'F': flashMorse("..-."); break; case 'G': flashMorse("--."); break;  case 'H': flashMorse("...."); break; case 'I': flashMorse(".."); break;   case 'J': flashMorse(".---"); break; case 'K': flashMorse("-.-"); break;  case 'L': flashMorse(".-.."); break; case 'M': flashMorse("--"); break;   case 'N': flashMorse("-."); break; case 'O': flashMorse("---"); break;  case 'P': flashMorse(".--."); break; case 'Q': flashMorse("--.-"); break; case 'R': flashMorse(".-."); break; case 'S': flashMorse("..."); break;  case 'T': flashMorse("-"); break; case 'U': flashMorse("..-"); break;  case 'V': flashMorse("...-"); break; case 'W': flashMorse(".--"); break;  case 'X': flashMorse("-..-"); break; case 'Y': flashMorse("-.--"); break; case 'Z': flashMorse("--.."); break; case '1': flashMorse(".----"); break; case '2': flashMorse("..---"); break; case '3': flashMorse("...--"); break; case '4': flashMorse("....-"); break; case '5': flashMorse("....."); break; case '6': flashMorse("-...."); break; case '7': flashMorse("--..."); break; case '8': flashMorse("---.."); break; case '9': flashMorse("----."); break; case '0': flashMorse("-----"); break;
  }
  setLED(0, 0, 0); delay(charGap); 
}
void flashMorse(String sequence) {
  for (int i = 0; i < sequence.length(); i++) { if (sequence[i] == '.') { setLED(0, 255, 0); delay(dotLen); } else if (sequence[i] == '-') { setLED(0, 0, 255); delay(dashLen); } setLED(10, 10, 10); delay(elemGap); setLED(0, 0, 0); }
}
char translateFromMorse(String seq) {
  if (seq == ".-") return 'A';   if (seq == "-...") return 'B'; if (seq == "-.-.") return 'C'; if (seq == "-..") return 'D'; if (seq == ".") return 'E';    if (seq == "..-.") return 'F'; if (seq == "--.") return 'G';  if (seq == "....") return 'H'; if (seq == "..") return 'I';   if (seq == ".---") return 'J'; if (seq == "-.-") return 'K';  if (seq == ".-..") return 'L'; if (seq == "--") return 'M';   if (seq == "-.") return 'N'; if (seq == "---") return 'O';  if (seq == ".--.") return 'P'; if (seq == "--.-") return 'Q'; if (seq == ".-.") return 'R'; if (seq == "...") return 'S';  if (seq == "-") return 'T'; if (seq == "..-") return 'U';  if (seq == "...-") return 'V'; if (seq == ".--") return 'W';  if (seq == "-..-") return 'X'; if (seq == "-.--") return 'Y'; if (seq == "--..") return 'Z'; if (seq == ".----") return '1'; if (seq == "..---") return '2'; if (seq == "...--") return '3'; if (seq == "....-") return '4'; if (seq == ".....") return '5'; if (seq == "-....") return '6'; if (seq == "--...") return '7'; if (seq == "---..") return '8'; if (seq == "----.") return '9'; if (seq == "-----") return '0'; return '?'; 
}