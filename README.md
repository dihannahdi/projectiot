# Simon Game with Cloud Integration

A hardware-based Simon Game using NodeMCU ESP8266 with Google Firebase integration for real-time game updates, leaderboards, and notifications.

## Project Structure

- **Main Game Algo.cpp**: NodeMCU code for the physical Simon game with WiFi and Firebase integration
- **index.html**: Web interface for game status and leaderboard
- **styles.css**: Styling for the web interface
- **app.js**: Frontend JavaScript for Firebase integration and UI updates

## Hardware Requirements

- NodeMCU ESP8266 development board
- 4 LEDs (different colors)
- 4 push buttons
- 1 buzzer
- Resistors (220Ω for LEDs, 10kΩ for buttons)
- Breadboard and jumper wires
- USB cable for power and programming

## Connection Diagram

```
NodeMCU Pin -> Component
-------------+---------------
D4           -> LED 1
D1           -> LED 2
D2           -> LED 3
D3           -> LED 4
D0           -> Button 1
D5           -> Button 2
D6           -> Button 3
D7           -> Button 4
D8           -> Buzzer
```

## Setup Instructions

### 1. Firebase Setup

1. Create a new Firebase project at https://console.firebase.google.com/
2. Add a web app to your project and get the configuration
3. Enable Firebase Realtime Database
   - Set up rules to allow read/write
4. Enable Firestore Database
   - Create a collection named `game_sessions`
5. Get your Firebase API keys and credentials

### 2. Hardware Setup

1. Connect the components according to the connection diagram
2. Connect the NodeMCU to your computer via USB

### 3. NodeMCU Code Setup

1. Install the Arduino IDE
2. Add ESP8266 board support in Arduino IDE
3. Install the following libraries:
   - ESP8266WiFi
   - FirebaseESP8266
4. Open `Main Game Algo.cpp` in Arduino IDE
5. Update the following values:
   ```cpp
   #define WIFI_SSID "YOUR_WIFI_SSID"
   #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
   #define FIREBASE_HOST "your-project-id.firebaseio.com"
   #define FIREBASE_AUTH "YOUR_FIREBASE_DATABASE_SECRET"
   ```
6. Upload the code to your NodeMCU
7. Open Serial Monitor at 115200 baud to see the Session ID

### 4. Web Interface Setup

1. Update the Firebase configuration in `app.js`:
   ```js
   const firebaseConfig = {
       apiKey: "YOUR_API_KEY",
       authDomain: "YOUR_PROJECT_ID.firebaseapp.com",
       projectId: "YOUR_PROJECT_ID",
       storageBucket: "YOUR_PROJECT_ID.appspot.com",
       messagingSenderId: "YOUR_MESSAGING_SENDER_ID",
       appId: "YOUR_APP_ID",
       databaseURL: "https://YOUR_PROJECT_ID.firebaseio.com"
   };
   ```
2. Deploy the web files (index.html, styles.css, app.js) to Firebase Hosting:
   - Install Firebase CLI: `npm install -g firebase-tools`
   - Login: `firebase login`
   - Initialize hosting: `firebase init hosting`
   - Deploy: `firebase deploy --only hosting`

Alternatively, you can host the files on any web server.

## How to Play

1. Upload the code to your NodeMCU and power it up
2. Note the Session ID that appears in the Serial Monitor
3. Open the web interface in a browser
4. Enter your name and the Session ID
5. Connect to the game
6. Play the Simon Game using the physical buttons
7. The web interface will show game progress and notifications in real-time
8. After the game ends, your score will appear on the leaderboard

## Game Rules

1. Watch the LED sequence
2. Repeat the sequence by pressing the buttons in the same order
3. With each successful round, the sequence gets one step longer
4. The game ends when you make a mistake

## Troubleshooting

- **No connection to Firebase**: Check your WiFi credentials and Firebase configuration
- **Session ID not found**: Make sure to use the exact Session ID from the Serial Monitor
- **LEDs not lighting up**: Check the wiring and pin connections
- **Buttons not responding**: Verify the button connections and check for proper grounding # projectiot
