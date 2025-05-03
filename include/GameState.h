#pragma once

// Enum for different game states
enum class GameState {
    MENU,       // Main menu
    LOBBY,      // Waiting for connection
    PLAYING,    // Game in progress
    PAUSED,     // Game paused
    GAME_OVER   // Game ended
};

// Enum for network roles
enum class NetworkRole {
    NONE,       // Not connected
    HOST,       // Server/host
    CLIENT      // Client
};
