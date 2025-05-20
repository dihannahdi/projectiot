/* NodeMCU ESP8266 version of Simon Game with Firebase Integration */
#include <ESP8266WiFi.h>
#include <Firebase.h> // Changed from FirebaseESP8266.h to Firebase.h

// WiFi credentials
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Firebase credentials - update to use the new library format
#define REFERENCE_URL "https://iotproject-de3d8-default-rtdb.firebaseio.com" // No trailing slash

// Initialize Firebase
Firebase fb(REFERENCE_URL); // Simplified initialization

// Game audio notes
#define NOTE_C3  131
#define NOTE_E4  330
#define NOTE_CS5 554
#define NOTE_E5  659
#define NOTE_A5  880

// Mapping pin dari Arduino UNO ke NodeMCU
int led[] = {D4, D1, D2, D3};     // Pengganti pin 12, 11, 10, 9
int button[] = {D0, D5, D6, D7};  // Pengganti pin 7, 6, 5, 4
int buzzpin = D8;                 // Pengganti pin 2

int notes[] = {NOTE_C3, NOTE_E4, NOTE_CS5, NOTE_E5, NOTE_A5};

int turn = 0; 
int seq[100];
int USeq[100];
int seqL = 0;
boolean played = false;
boolean gameActive = false;

// For button debouncing
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 200;

// Custom tone function for ESP8266 with volume control
// volume parameter ranges from 0 (silent) to 10 (maximum volume)
void playTone(int pin, int frequency, int duration, int volume = 10) {
  // Menghitung setengah periode (dalam mikrodetik)
  unsigned long halfPeriod = 1000000L / frequency / 2;
  unsigned long startTime = millis();
  
  // Calculate duty cycle based on volume (1-10)
  // Lower volume = smaller duty cycle
  volume = constrain(volume, 1, 10); // Ensure volume is between 1 and 10
  int onTime = halfPeriod * volume / 10;
  int offTime = halfPeriod * 2 - onTime; // Maintain the same frequency
  
  while (millis() - startTime < duration) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(onTime);
    digitalWrite(pin, LOW);
    delayMicroseconds(offTime);
  }
}

// Generate a unique session ID
String generateSessionId() {
  // Generate a random alphanumeric string
  const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  String result = "";
  for (int i = 0; i < 10; i++) {
    result += charset[random(0, 62)];
  }
  return result;
}

// Session ID for the current game
String sessionId = "";

// Send game state to Firebase
void updateGameState(String status, int level) {
  if (sessionId == "") return; // Don't update if no session
  
  // Create a JSON format string for the game state
  String jsonData = "{\"status\":\"" + status + "\",\"level\":" + level + "}";
  
  // Update the state node
  fb.setJson("/games/" + sessionId + "/state", jsonData);
}

// Send score to Firebase when game ends
void sendGameScore(int score) {
  if (sessionId == "") return; // Don't update if no session
  
  // Update the final score
  fb.setInt("/games/" + sessionId + "/finalScore", score);
}

void connectWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    
    // Flash LEDs while connecting
    for (int i = 0; i < 4; i++) {
      digitalWrite(led[i], HIGH);
      delay(50);
      digitalWrite(led[i], LOW);
    }
  }
  
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  
  for (int i = 0; i < 4; i++) {
    pinMode(led[i], OUTPUT);
    pinMode(button[i], INPUT_PULLUP); // penting untuk ESP
  }
  pinMode(buzzpin, OUTPUT);
  
  // Connect to WiFi
  connectWiFi();
  
  // Generate a new session ID for this game
  sessionId = generateSessionId();
  
  // Reset game data
  randomSeed(analogRead(A0)); // gunakan A0 pada NodeMCU
  for (int i = 0; i < 99; i++) {
    seq[i] = random(0, 4);
  }
  
  // Initialize game state
  turn = 0;
  seqL = 0;
  played = false;
  gameActive = true;
  
  // Send initial game state to Firebase
  Serial.println("New game session ID: " + sessionId);
  updateGameState("ready", 0);
}

void win() {
  for (int j = 0; j < 4; j++) {
    for (int i = 0; i < 4; i++) {
      playTone(buzzpin, notes[i], 50, 7); // Volume at 7/10
      digitalWrite(led[i], HIGH);
      delay(50);
      digitalWrite(led[i], LOW);
    }
  }
  delay(500);
}

void loss() {
  // Update game state to "failed"
  updateGameState("failed", turn);
  
  // Send final score
  sendGameScore(turn - 1);
  
  // Play loss sound
  playTone(buzzpin, NOTE_C3, 500, 10); // Volume at 8/10
  delay(1500);
  
  // Reset game
  turn = 0;
  seqL = 0;
  played = false;
  
  // Generate new session ID for next game
  sessionId = generateSessionId();
  Serial.println("New game session ID: " + sessionId);
  
  // Regenerate sequence
  for (int i = 0; i < 99; i++) {
    seq[i] = random(0, 4);
  }
  
  // Reset game state
  updateGameState("ready", 0);
}

void playSeq(int t) {
  updateGameState("playing_sequence", t);
  
  for (int i = 0; i < t; i++) {
    playTone(buzzpin, notes[seq[i]], 500, 6); // Volume at 6/10
    digitalWrite(led[seq[i]], HIGH);
    delay(500);
    digitalWrite(led[seq[i]], LOW);
    delay(200);   
  }
  
  updateGameState("waiting_player", t);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  
  if (turn == 0) {
    updateGameState("game_start", 1);
    win();
    turn = turn + 1;
  }
  
  if (seqL == 0 && played == false) {
    updateGameState("new_level", turn);
    Serial.print("Starting level: ");
    Serial.println(turn);
    
    // Short delay to allow UI to update
    delay(1000);
    
    playSeq(turn);
    played = true;
  }
  else if (seqL == turn) {
    updateGameState("level_complete", turn);
    win();
    played = false;
    turn = turn + 1;
    seqL = 0;
  }
  else {
    for (int i = 0; i < 4; i++) {
      if (digitalRead(button[i]) == LOW) {
        // Simple debouncing
        if ((millis() - lastDebounceTime) > debounceDelay) {
          lastDebounceTime = millis();
          
          USeq[seqL] = i;
          if (USeq[seqL] != seq[seqL]) {
            loss();
          }
          else {
            seqL = seqL + 1;
            digitalWrite(led[i], HIGH);
            playTone(buzzpin, notes[i], 500, 7); // Volume at 7/10
            
            // Update Firebase with correct button press
            String jsonData = "{\"buttonPress\":" + String(i) + ",\"progress\":\"" + String(seqL) + "/" + String(turn) + "\"}";
            fb.setJson("/games/" + sessionId + "/state", jsonData);
            
            delay(500);
            digitalWrite(led[i], LOW);
          }
        }
      }
    }
  }
}
