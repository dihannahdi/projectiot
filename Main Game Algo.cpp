/* NodeMCU ESP8266 version of Simon Game with Firebase Integration */
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

// WiFi credentials
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Firebase credentials
#define FIREBASE_HOST "iotproject-de3d8-default-rtdb.firebaseio.com" // Without https:// or /
#define FIREBASE_AUTH "6QQmheQh54hvM8jJ1RHz6yBOXAFG669ri1qExTbk" // You need to get this from Firebase console

// Game audio notes
#define NOTE_C3  131
#define NOTE_E4  330
#define NOTE_CS5 554
#define NOTE_E5  659
#define NOTE_A5  880

// Firebase data objects
FirebaseData firebaseData;
FirebaseJson gameData;

// Session ID for the current game
String sessionId = "";

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

// Send game state to Firebase
void updateGameState(String status, int level) {
  if (sessionId == "") return; // Don't update if no session
  
  gameData.clear();
  gameData.set("status", status);
  gameData.set("level", level);
  gameData.set("timestamp", Firebase.getCurrentTime());
  
  Firebase.updateNode(firebaseData, "/games/" + sessionId + "/state", gameData);
}

// Send score to Firebase when game ends
void sendGameScore(int score) {
  if (sessionId == "") return; // Don't update if no session
  
  gameData.clear();
  gameData.set("finalScore", score);
  gameData.set("timestamp", Firebase.getCurrentTime());
  
  Firebase.updateNode(firebaseData, "/games/" + sessionId, gameData);
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
  
  // Initialize Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  
  // Set firebaseData timeout
  firebaseData.setResponseSize(1024);
  
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
      tone(buzzpin, notes[i], 50);
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
  tone(buzzpin, NOTE_C3, 500);
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
    tone(buzzpin, notes[seq[i]], 500);
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
            tone(buzzpin, notes[i], 500);
            
            // Update Firebase with correct button press
            gameData.clear();
            gameData.set("buttonPress", i);
            gameData.set("progress", (String)seqL + "/" + turn);
            Firebase.updateNode(firebaseData, "/games/" + sessionId + "/state", gameData);
            
            delay(500);
            digitalWrite(led[i], LOW);
          }
        }
      }
    }
  }
}
