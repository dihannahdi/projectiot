# Firebase Configuration Guide for Simon Game

This guide provides detailed instructions for setting up Firebase services for the Simon Game project.

## 1. Create a Firebase Project

1. Go to the [Firebase Console](https://console.firebase.google.com/)
2. Click "Add project"
3. Enter a project name (e.g., "simon-game")
4. Enable or disable Google Analytics as preferred
5. Click "Create project"

## 2. Set Up Firebase Realtime Database

The Realtime Database is used for real-time game state updates between the NodeMCU and web interface.

1. In the Firebase Console, navigate to "Build" → "Realtime Database"
2. Click "Create Database"
3. Start in test mode for development (we'll update security rules later)
4. Choose a database location close to your users
5. Click "Enable"

### Database Rules

For development, you can use these rules (replace later for production):

```json
{
  "rules": {
    "games": {
      "$gameId": {
        ".read": true,
        ".write": true,
        "player": {
          ".read": true,
          ".write": true
        },
        "state": {
          ".read": true,
          ".write": true
        }
      }
    }
  }
}
```

For production, consider more restrictive rules.

## 3. Set Up Firestore Database

Firestore is used for storing the leaderboard data.

1. In the Firebase Console, navigate to "Build" → "Firestore Database"
2. Click "Create Database"
3. Start in test mode for development 
4. Choose a database location close to your users
5. Click "Enable"

### Firestore Rules

For development, you can use these rules:

```
rules_version = '2';
service cloud.firestore {
  match /databases/{database}/documents {
    match /game_sessions/{sessionId} {
      allow read;
      allow write: if request.resource.data.keys().hasAll(['playerName', 'status']);
    }
  }
}
```

## 4. Register a Web App

1. In the Firebase Console, click the gear icon next to "Project Overview" and select "Project settings"
2. In the "Your apps" section, click the web icon (</>) to add a web app
3. Register the app with a nickname (e.g., "Simon Game Web")
4. Optionally enable Firebase Hosting
5. Click "Register app"
6. Copy the Firebase configuration object that appears - you'll need to paste this into your `app.js` file

### Firebase SDK Integration

For this project, we're now using the ES modules approach with Firebase SDK v11.7.3:

```html
<!-- In index.html, we use type="module" to enable ES module imports -->
<script type="module" src="app.js"></script>
```

Then in app.js, we import the Firebase modules directly:

```javascript
// Import Firebase modules
import { initializeApp } from "https://www.gstatic.com/firebasejs/11.7.3/firebase-app.js";
import { getAnalytics } from "https://www.gstatic.com/firebasejs/11.7.3/firebase-analytics.js";
import { getFirestore, collection, doc, setDoc, updateDoc, query, where, orderBy, limit, getDocs, serverTimestamp } from "https://www.gstatic.com/firebasejs/11.7.3/firebase-firestore.js";
import { getDatabase, ref, set, update, onValue, get } from "https://www.gstatic.com/firebasejs/11.7.3/firebase-database.js";

// Firebase Configuration
const firebaseConfig = {
    apiKey: "YOUR_API_KEY",
    authDomain: "YOUR_PROJECT_ID.firebaseapp.com",
    databaseURL: "https://YOUR_PROJECT_ID-default-rtdb.firebaseio.com",
    projectId: "YOUR_PROJECT_ID",
    storageBucket: "YOUR_PROJECT_ID.firebasestorage.app",
    messagingSenderId: "YOUR_MESSAGING_SENDER_ID",
    appId: "YOUR_APP_ID",
    measurementId: "YOUR_MEASUREMENT_ID"
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const analytics = getAnalytics(app);
const db = getFirestore(app);
const rtdb = getDatabase(app);
```

The ES modules approach offers several advantages:
- Better code organization with explicit imports
- Tree-shaking to include only what you need
- Future-proof approach aligned with modern JavaScript standards
- No need for global variables

Note: When using ES modules, all async operations should use async/await or promises for cleaner code structure.

## 5. Get Firebase Database Secret (For NodeMCU)

For the ESP8266 to connect to Firebase:

1. In the Firebase Console, go to "Project settings"
2. Navigate to the "Service accounts" tab
3. Click "Database secrets" (in the Realtime Database section)
4. Click "Show" to reveal your database secret
5. Copy this secret - you'll need to paste it into your NodeMCU code as `FIREBASE_AUTH`

## 6. Deploy to Firebase Hosting

Your project is already set up with Firebase Hosting with GitHub integration:

### 6.1 Project Structure

Based on your Firebase initialization, your project is configured with:
- Public directory: `public` (place all web files here)
- Single-page app configuration (all URLs rewrite to /index.html)
- GitHub repository integration: `dihannahdi/projectiot`
- Build script: `npm run build` runs before each deployment

### 6.2 File Organization

Move your web interface files to match this structure:
```
IOT FIX/
├── public/           # Your web files go here
│   ├── index.html    
│   ├── app.js        
│   └── styles.css    
├── .github/          # GitHub workflows (auto-created)
├── firebase.json     # Firebase configuration
└── .firebaserc       # Project aliases
```

### 6.3 Build Process

Since you've set up a build process, you should:
1. Create a `package.json` file if you don't have one:
```json
{
  "name": "simon-game-dashboard",
  "version": "1.0.0",
  "scripts": {
    "build": "echo 'Building project...'",
    "test": "echo 'Testing project...'"
  }
}
```
2. Customize the `build` script to perform any necessary build steps

### 6.4 Deployment Options

Your project is set up with two automatic deployment workflows:

1. **Pull Request Workflow**: Deploys preview channels when PRs are created
2. **Merge Workflow**: Deploys to live channel when PRs are merged to `main`

To manually deploy from your local machine:

```bash
firebase deploy --only hosting
```

### 6.5 Access Your Deployed Site

Your Simon Game dashboard will be accessible at:
- `https://iotproject-de3d8.web.app`
- `https://iotproject-de3d8.firebaseapp.com`

## 7. Database Structure

### Realtime Database

The NodeMCU code creates this structure automatically:

```
/games
  /{sessionId}
    /player: "PlayerName"
    /state
      /status: "ready|game_start|new_level|playing_sequence|waiting_player|level_complete|failed"
      /level: 1
      /buttonPress: 0-3
      /progress: "1/3"
      /timestamp: 1234567890
    /finalScore: 10
```

### Firestore Database

The web interface creates this structure:

```
/game_sessions
  /{sessionId}
    playerName: "PlayerName"
    startTime: Timestamp
    status: "active|completed"
    finalScore: 10
    finalLevel: 10
    endTime: Timestamp
```

## 8. Testing Your Setup

1. After uploading code to your NodeMCU, check the Serial Monitor for the session ID
2. Open your web interface and enter that session ID
3. Verify that:
   - The NodeMCU connects to WiFi and Firebase
   - The web interface can retrieve game state
   - Game updates appear in real time on the web interface
   - Game scores are recorded in the leaderboard

## 9. Common Issues and Solutions

- **NodeMCU can't connect to WiFi**: Double-check SSID and password, ensure WiFi network is 2.4GHz (ESP8266 doesn't support 5GHz)
- **Firebase connection errors**: Verify your Firebase Host and Auth key, check database rules
- **Session ID not found**: Make sure you're entering the exact session ID from Serial Monitor
- **Web app can't connect**: Check Firebase config object in app.js, ensure Firebase services are enabled
- **No updates in real-time**: Check browser console for errors, verify Realtime Database paths match

For more support, visit the [Firebase documentation](https://firebase.google.com/docs) 