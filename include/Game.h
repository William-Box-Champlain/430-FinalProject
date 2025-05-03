#pragma once

#include <vector>
#include <raylib.h>
#include "GameState.h"
#include "Ball.h"
#include "Paddle.h"
#include "NetworkManager.h"
#include "UIManager.h"

// Structure to represent pending ball respawns
struct BallRespawnInfo {
    float timer;          // Time until respawn
    float originalTimer;  // For networking/serialization
};

// Constants
constexpr int SCREEN_WIDTH = 800;
constexpr int SCREEN_HEIGHT = 600;
constexpr int PADDLE_WIDTH = 15;
constexpr int PADDLE_HEIGHT = 100;
constexpr int MIN_BALL_RADIUS = 8;
constexpr int MAX_BALL_RADIUS = 12;
constexpr int DEFAULT_BALL_RADIUS = 10;
constexpr int BALL_COUNT = 5;
constexpr int WINNING_SCORE = 10;
constexpr float PADDLE_SPEED = 400.0f;
constexpr float BALL_SPEED = 300.0f;
constexpr int DEFAULT_PORT = 7777;

// Game class that manages the entire game
class Game {
public:
    Game();
    ~Game();

    // Initialize the game
    void Initialize();
    
    // Run the game loop
    void Run();
    
    // Process input
    void ProcessInput();
    
    // Update game logic
    void Update();
    
    // Render the game
    void Render();
    
    // Handle network messages
    void HandleNetworkMessages(MessageType type, Packet& packet);
    
    // Start hosting a game
    void StartHost();
    
    // Join a game as client
    void JoinGame(const std::string& address);
    
    // Start the game
    void StartGame();
    
    // Reset the game
    void ResetGame();
    
    // Toggle pause state
    void TogglePause();

private:
    // Game state
    GameState currentState;
    bool running;
    bool isHost;
    bool wasConnected;
    
    // Game objects
    std::vector<Ball> balls;
    Paddle leftPaddle;
    Paddle rightPaddle;
    
    // Scores
    int leftScore;
    int rightScore;
    
    // Network manager
    NetworkManager networkManager;
    
    // UI manager
    UIManager uiManager;
    
    // Input state
    bool leftPaddleUp;
    bool leftPaddleDown;
    bool rightPaddleUp;
    bool rightPaddleDown;
    
    // Time tracking
    float deltaTime;
    
    // Ball respawn timers
    std::vector<BallRespawnInfo> ballRespawnTimers;
    
    // Helper methods
    void InitializeBalls();
    void CheckBallCollisions();
    void CheckScoring();
    void SendGameState();
    void ProcessGameState(Packet& packet);
    void ProcessInputMessage(Packet& packet);
};
