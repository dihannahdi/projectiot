// Import Firebase modules
import { initializeApp } from "https://www.gstatic.com/firebasejs/11.7.3/firebase-app.js";
import { getAnalytics } from "https://www.gstatic.com/firebasejs/11.7.3/firebase-analytics.js";
import { getFirestore, collection, doc, setDoc, updateDoc, getDoc, query, where, orderBy, limit, getDocs, serverTimestamp } from "https://www.gstatic.com/firebasejs/11.7.3/firebase-firestore.js";
import { getDatabase, ref, set, update, onValue, get } from "https://www.gstatic.com/firebasejs/11.7.3/firebase-database.js";

// Firebase Configuration
const firebaseConfig = {
    apiKey: "AIzaSyA-D92yF6fvY5-WF_lTKyRylMj4x3OYupY",
    authDomain: "iotproject-de3d8.firebaseapp.com",
    databaseURL: "https://iotproject-de3d8-default-rtdb.firebaseio.com",
    projectId: "iotproject-de3d8",
    storageBucket: "iotproject-de3d8.firebasestorage.app",
    messagingSenderId: "468210701486",
    appId: "1:468210701486:web:ba5d87e7e6d8a28749c93f",
    measurementId: "G-P6L6DBTH9Z"
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const analytics = getAnalytics(app);
const db = getFirestore(app);
const rtdb = getDatabase(app);

// Global variables
let currentSessionId = null;
let playerName = null;
let gameStateRef = null;
let gameListenerActive = false;

// DOM Elements
const playerNameInput = document.getElementById('player-name');
const sessionIdInput = document.getElementById('session-id');
const connectBtn = document.getElementById('connect-btn');
const playerRegistration = document.getElementById('player-registration');
const gamePanel = document.getElementById('game-panel');
const playerDisplay = document.getElementById('player-display');
const statusContainer = document.getElementById('status-container');
const currentLevelElement = document.getElementById('current-level');
const progressBar = document.getElementById('progress-bar');
const notificationList = document.getElementById('notification-list');
const leaderboardBody = document.getElementById('leaderboard-body');

// Connect to game session
connectBtn.addEventListener('click', connectToGame);

async function connectToGame() {
    playerName = playerNameInput.value.trim();
    currentSessionId = sessionIdInput.value.trim();
    
    if (!playerName) {
        alert('Please enter your name');
        return;
    }
    
    if (!currentSessionId) {
        alert('Please enter the session ID from your device');
        return;
    }
    
    try {
        // Check if session exists
        const sessionRef = ref(rtdb, `/games/${currentSessionId}`);
        const snapshot = await get(sessionRef);
        
        if (snapshot.exists()) {
            // Register player with session
            await set(ref(rtdb, `/games/${currentSessionId}/player`), playerName);
            
            // Show game panel
            playerRegistration.style.display = 'none';
            gamePanel.style.display = 'block';
            playerDisplay.textContent = `Player: ${playerName}`;
            
            // Start listening for game updates
            setupGameListeners();
            
            // Add game session to Firestore for leaderboard
            await setDoc(doc(db, 'game_sessions', currentSessionId), {
                playerName: playerName,
                startTime: serverTimestamp(),
                status: 'active'
            });
            
            // Add notification
            addNotification('Connected to game session', 'notification-info');
            
            // Load leaderboard
            loadLeaderboard();
        } else {
            alert('Session not found. Please check the Session ID and try again.');
        }
    } catch (error) {
        console.error("Error checking session:", error);
        alert('Error connecting to game: ' + error.message);
    }
}

// Setup listeners for real-time game state
function setupGameListeners() {
    if (gameListenerActive) return;
    
    gameStateRef = ref(rtdb, `/games/${currentSessionId}/state`);
    
    // Listen for game state changes
    onValue(gameStateRef, (snapshot) => {
        const gameState = snapshot.val();
        if (!gameState) return;
        
        updateGameUI(gameState);
    });
    
    // Listen for game completion
    onValue(ref(rtdb, `/games/${currentSessionId}/finalScore`), async (snapshot) => {
        const finalScore = snapshot.val();
        if (finalScore) {
            // Update Firestore for leaderboard
            try {
                await updateDoc(doc(db, 'game_sessions', currentSessionId), {
                    finalScore: finalScore,
                    finalLevel: finalScore,
                    endTime: serverTimestamp(),
                    status: 'completed'
                });
                loadLeaderboard();
            } catch (error) {
                console.error("Error updating game completion:", error);
            }
        }
    });
    
    gameListenerActive = true;
}

// Update game UI based on state
function updateGameUI(gameState) {
    const { status, level } = gameState;
    
    // Update level display
    if (level !== undefined) {
        currentLevelElement.textContent = level;
    }
    
    // Update status indicators
    statusContainer.className = 'alert mb-3';
    
    switch(status) {
        case 'ready':
            statusContainer.textContent = 'Game is ready to start';
            statusContainer.classList.add('alert-info');
            addNotification('Game is ready to start', 'notification-info');
            break;
            
        case 'game_start':
            statusContainer.textContent = 'Game is starting!';
            statusContainer.classList.add('alert-info');
            addNotification('Game has started', 'notification-info');
            break;
            
        case 'new_level':
            statusContainer.textContent = `Starting level ${level}`;
            statusContainer.classList.add('status-new-level');
            addNotification(`Starting level ${level}`, 'notification-new-level');
            break;
            
        case 'playing_sequence':
            statusContainer.textContent = `Watch the sequence for level ${level}`;
            statusContainer.classList.add('status-playing');
            addNotification(`Playing sequence for level ${level}`, 'notification-info');
            break;
            
        case 'waiting_player':
            statusContainer.textContent = `Your turn! Repeat the sequence for level ${level}`;
            statusContainer.classList.add('status-waiting');
            addNotification(`Your turn to repeat the sequence`, 'notification-info');
            break;
            
        case 'level_complete':
            statusContainer.textContent = `Level ${level} completed!`;
            statusContainer.classList.add('status-new-level');
            addNotification(`Level ${level} completed!`, 'notification-success');
            break;
            
        case 'failed':
            statusContainer.textContent = `Game over! You reached level ${level}`;
            statusContainer.classList.add('status-failed');
            addNotification(`Game over! You reached level ${level}`, 'notification-error');
            break;
    }
    
    // Update progress if available
    if (gameState.progress) {
        const [current, total] = gameState.progress.split('/').map(Number);
        const progressPercent = (current / total) * 100;
        progressBar.style.width = `${progressPercent}%`;
        progressBar.textContent = `${current}/${total}`;
    } else if (status === 'waiting_player' || status === 'playing_sequence') {
        progressBar.style.width = '0%';
        progressBar.textContent = '';
    }
    
    // Handle button press updates
    if (status !== 'failed' && gameState.buttonPress !== undefined) {
        const buttonColors = ['Red', 'Green', 'Blue', 'Yellow'];
        addNotification(`Button press: ${buttonColors[gameState.buttonPress]}`, 'notification-info');
    }
}

// Add a notification to the list
function addNotification(message, className) {
    const li = document.createElement('li');
    li.className = `list-group-item ${className}`;
    
    const now = new Date();
    const timeStr = now.toLocaleTimeString();
    
    li.innerHTML = `
        ${message}
        <span class="notification-timestamp">${timeStr}</span>
    `;
    
    notificationList.prepend(li);
    
    // Limit to 20 notifications
    if (notificationList.children.length > 20) {
        notificationList.removeChild(notificationList.lastChild);
    }
}

// Load leaderboard data from Firestore
async function loadLeaderboard() {
    try {
        const q = query(
            collection(db, 'game_sessions'),
            where('status', '==', 'completed'),
            orderBy('finalLevel', 'desc'),
            limit(10)
        );
        
        const querySnapshot = await getDocs(q);
        
        // Clear existing leaderboard
        leaderboardBody.innerHTML = '';
        
        let rank = 1;
        querySnapshot.forEach((doc) => {
            const data = doc.data();
            const row = document.createElement('tr');
            
            // Highlight current player's score
            if (doc.id === currentSessionId) {
                row.classList.add('highscore-row');
            }
            
            const date = data.endTime ? data.endTime.toDate() : new Date();
            const dateStr = date.toLocaleDateString();
            
            row.innerHTML = `
                <td>${rank}</td>
                <td>${data.playerName || 'Unknown'}</td>
                <td>${data.finalLevel || 0}</td>
                <td>${dateStr}</td>
            `;
            
            leaderboardBody.appendChild(row);
            rank++;
        });
    } catch (error) {
        console.error("Error loading leaderboard:", error);
    }
} 