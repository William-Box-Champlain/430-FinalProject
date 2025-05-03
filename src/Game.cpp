#include "Game.h"
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>

Game::Game()
    : currentState(GameState::MENU), running(false), isHost(false), wasConnected(false),
      leftScore(0), rightScore(0), leftPaddleUp(false), leftPaddleDown(false),
      rightPaddleUp(false), rightPaddleDown(false), deltaTime(0) {
}

Game::~Game() {
    // Clean up resources
}

void Game::Initialize() {
    // Initialize random seed
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    
    // Initialize window
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Multiball Pong");
    SetTargetFPS(60);
    
    // Initialize UI
    uiManager.Initialize();
    
    // Initialize paddles
    const float PADDLE_MARGIN = 50.0f;
    leftPaddle = Paddle(
        {PADDLE_MARGIN, SCREEN_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f},
        {PADDLE_WIDTH, PADDLE_HEIGHT},
        true,
        BLUE
    );
    
    rightPaddle = Paddle(
        {SCREEN_WIDTH - PADDLE_MARGIN - PADDLE_WIDTH, SCREEN_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f},
        {PADDLE_WIDTH, PADDLE_HEIGHT},
        false,
        RED
    );
    
    // Initialize balls
    InitializeBalls();
    
    // Set up network callback
    networkManager.SetMessageCallback([this](MessageType type, Packet& packet) {
        HandleNetworkMessages(type, packet);
    });
    
    // Set game as running
    running = true;
}

void Game::Run() {
    // Main game loop
    while (!WindowShouldClose() && running) {
        // Calculate delta time
        deltaTime = GetFrameTime();
        
        // Process input
        ProcessInput();
        
        // Update game
        Update();
        
        // Render game
        Render();
    }
    
    // Clean up
    CloseWindow();
}

void Game::ProcessInput() {
    // Process UI input
    uiManager.Update();
    
    // Process game state specific input
    switch (currentState) {
        case GameState::MENU:
            // Check for host/join button clicks
            if (uiManager.IsHostButtonClicked()) {
                StartHost();
            } else if (uiManager.IsJoinButtonClicked()) {
                JoinGame(uiManager.GetIPAddress());
            }
            break;
            
        case GameState::LOBBY:
            // Check for back button
            if (uiManager.IsBackButtonClicked() || IsKeyPressed(KEY_ESCAPE)) {
                networkManager.Shutdown();
                currentState = GameState::MENU;
                uiManager.ResetUI(); // Reset UI elements when returning to menu
            }
            
            // Start game if host clicks the start game button
            if (isHost && networkManager.IsConnected() && uiManager.IsStartGameButtonClicked()) {
                // Start the game locally
                StartGame();
                
                // Send start game message multiple times to increase chances of it getting through
                for (int i = 0; i < 10; i++) {
                    Packet packet;
                    // Add a unique identifier to the packet to prevent duplicate processing
                    packet.Write(static_cast<int>(i));
                    bool sent = networkManager.SendPacket(MessageType::START_GAME, packet);
                    std::cout << "Host: Sending START_GAME message attempt " << (i+1) << ", success: " << (sent ? "true" : "false") << std::endl;
                }
            }
            break;
            
        case GameState::PLAYING:
            // Handle paddle input
            leftPaddleUp = IsKeyDown(KEY_W);
            leftPaddleDown = IsKeyDown(KEY_S);
            rightPaddleUp = IsKeyDown(KEY_UP);
            rightPaddleDown = IsKeyDown(KEY_DOWN);
            
            // Pause game
            if (IsKeyPressed(KEY_P)) {
                TogglePause();
                
                // Send pause message
                Packet packet;
                networkManager.SendPacket(MessageType::PAUSE_GAME, packet);
            }
            
            // Send input to other player if we're connected
            if (networkManager.IsConnected()) {
                Packet packet;
                
                // Send our paddle input
                if (isHost) {
                    packet.Write(leftPaddleUp);
                    packet.Write(leftPaddleDown);
                } else {
                    packet.Write(rightPaddleUp);
                    packet.Write(rightPaddleDown);
                }
                
                networkManager.SendPacket(MessageType::INPUT, packet);
            }
            break;
            
        case GameState::PAUSED:
            // Unpause game
            if (IsKeyPressed(KEY_P)) {
                TogglePause();
                
                // Send pause message
                Packet packet;
                networkManager.SendPacket(MessageType::PAUSE_GAME, packet);
            }
            
            // Check for restart button
            if (uiManager.IsRestartButtonClicked()) {
                ResetGame();
                
                // Send restart message
                Packet packet;
                networkManager.SendPacket(MessageType::RESTART_GAME, packet);
            }
            
            // Check for back button
            if (uiManager.IsBackButtonClicked() || IsKeyPressed(KEY_ESCAPE)) {
                networkManager.Shutdown();
                currentState = GameState::MENU;
                uiManager.ResetUI(); // Reset UI elements when returning to menu
            }
            break;
            
        case GameState::GAME_OVER:
            // Check for restart button
            if (uiManager.IsRestartButtonClicked()) {
                ResetGame();
                
                // Send restart message
                Packet packet;
                networkManager.SendPacket(MessageType::RESTART_GAME, packet);
            }
            
            // Check for back button
            if (uiManager.IsBackButtonClicked() || IsKeyPressed(KEY_ESCAPE)) {
                networkManager.Shutdown();
                currentState = GameState::MENU;
                uiManager.ResetUI(); // Reset UI elements when returning to menu
            }
            break;
    }
}

void Game::Update() {
    // Update network
    networkManager.Update(deltaTime);
    
    // Check for connection failures in lobby state
    if (currentState == GameState::LOBBY && !networkManager.IsConnected()) {
        // If we were previously connected, go back to menu
        if (wasConnected) {
            std::cout << "Connection lost, returning to menu" << std::endl;
            currentState = GameState::MENU;
            wasConnected = false;
        }
    } else if (currentState == GameState::LOBBY && networkManager.IsConnected()) {
        // Track that we were connected
        wasConnected = true;
    }
    
    // Update game state
    if (currentState == GameState::PLAYING) {
        // Update paddles
        leftPaddle.Update(leftPaddleUp, leftPaddleDown, deltaTime, SCREEN_HEIGHT);
        rightPaddle.Update(rightPaddleUp, rightPaddleDown, deltaTime, SCREEN_HEIGHT);
        
        // Update balls
        for (auto& ball : balls) {
            ball.Update(deltaTime);
            
            // Check wall collisions
            ball.CheckWallCollision(SCREEN_WIDTH, SCREEN_HEIGHT);
            
            // Check paddle collisions
            ball.CheckPaddleCollision(leftPaddle);
            ball.CheckPaddleCollision(rightPaddle);
        }
        
        // Check ball-to-ball collisions
        CheckBallCollisions();
        
        // Check scoring
        CheckScoring();
        
        // Update ball respawn timers (host only)
        if (isHost) {
            for (auto it = ballRespawnTimers.begin(); it != ballRespawnTimers.end();) {
                // Decrease timer
                it->timer -= deltaTime;
                
                // If timer has expired, spawn a new ball
                if (it->timer <= 0.0f) {
                    // Random position in center area
                    const int CENTER_SPAWN_OFFSET = 50;
                    Vector2 position = {
                        SCREEN_WIDTH / 2.0f + static_cast<float>(GetRandomValue(-CENTER_SPAWN_OFFSET, CENTER_SPAWN_OFFSET)),
                        SCREEN_HEIGHT / 2.0f + static_cast<float>(GetRandomValue(-CENTER_SPAWN_OFFSET, CENTER_SPAWN_OFFSET))
                    };
                    
                    // Random direction
                    const int MAX_ANGLE_DEGREES = 360;
                    float angle = static_cast<float>(GetRandomValue(0, MAX_ANGLE_DEGREES)) * DEG2RAD;
                    Vector2 velocity = {
                        std::cos(angle) * BALL_SPEED,
                        std::sin(angle) * BALL_SPEED
                    };
                    
                    // Random color
                    Color colors[] = {RED, GREEN, BLUE, YELLOW, PURPLE};
                    const int COLOR_COUNT = 5;
                    Color color = colors[GetRandomValue(0, COLOR_COUNT - 1)];
                    
                    // Random size
                    float radius = static_cast<float>(GetRandomValue(MIN_BALL_RADIUS, MAX_BALL_RADIUS));
                    
                    // Create new ball
                    balls.emplace_back(position, radius, color, velocity);
                    
                    // Remove timer
                    it = ballRespawnTimers.erase(it);
                } else {
                    ++it;
                }
            }
        } else {
            // Client only updates timer values but doesn't spawn balls
            for (auto& timer : ballRespawnTimers) {
                timer.timer -= deltaTime;
            }
        }
        
        // Send game state if we're the host
        if (isHost && networkManager.IsConnected()) {
            SendGameState();
        }
    }
}

void Game::Render() {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    
    // Render game elements based on state
    switch (currentState) {
        case GameState::PLAYING:
            // Render paddles
            leftPaddle.Render();
            rightPaddle.Render();
            
            // Render balls
            for (auto& ball : balls) {
                ball.Render();
            }
            break;
            
        case GameState::PAUSED:
        case GameState::GAME_OVER:
            // Render game elements in background
            leftPaddle.Render();
            rightPaddle.Render();
            
            for (auto& ball : balls) {
                ball.Render();
            }
            break;
    }
    
    // Render UI
    uiManager.Render(currentState, isHost, networkManager.IsConnected(), leftScore, rightScore);
    
    EndDrawing();
}

void Game::HandleNetworkMessages(MessageType type, Packet& packet) {
    switch (type) {
        case MessageType::CONNECT:
            // Connection established
            if (!wasConnected) {
                std::cout << "Connection established" << std::endl;
                wasConnected = true;
                
                // If we're the host, send an acknowledgment back to the client
                if (isHost && networkManager.IsConnected()) {
                    std::cout << "Host: Sending CONNECT acknowledgment" << std::endl;
                    Packet ackPacket;
                    networkManager.SendPacket(MessageType::CONNECT, ackPacket);
                }
            } else {
                std::cout << "Ignoring duplicate CONNECT message" << std::endl;
            }
            break;
            
        case MessageType::DISCONNECT:
            // Log the disconnect but don't actually disconnect
            std::cout << "Received DISCONNECT message, but staying connected" << std::endl;
            
            // We'll stay connected and in the current state
            // This is to avoid disconnection issues
            break;
            
        case MessageType::GAME_STATE:
            // Process game state update
            ProcessGameState(packet);
            break;
            
        case MessageType::INPUT:
            // Process input from other player
            ProcessInputMessage(packet);
            break;
            
        case MessageType::START_GAME:
            {
                // Read the unique identifier if present
                int messageId = -1;
                if (packet.GetSize() >= sizeof(int)) {
                    packet.Read(messageId);
                }
                
                std::cout << "Client: Received START_GAME message (ID: " << messageId << ")" << std::endl;
                
                // Force transition to PLAYING state
                if (currentState != GameState::PLAYING) {
                    std::cout << "Client: Transitioning from " << static_cast<int>(currentState) << " to PLAYING state" << std::endl;
                    StartGame();
                    
                    // Send acknowledgment back to host with the same ID
                    Packet ackPacket;
                    if (messageId >= 0) {
                        ackPacket.Write(messageId);
                    }
                    networkManager.SendPacket(MessageType::START_GAME, ackPacket);
                    
                    // Send it multiple times to increase chances of it getting through
                    for (int i = 0; i < 5; i++) {
                        networkManager.SendPacket(MessageType::START_GAME, ackPacket);
                    }
                } else {
                    std::cout << "Client: Already in PLAYING state" << std::endl;
                }
                break;
            }
            
        case MessageType::RESTART_GAME:
            // Only restart the game if we're in GAME_OVER or PAUSED state
            if (currentState == GameState::GAME_OVER || currentState == GameState::PAUSED) {
                std::cout << "Received RESTART_GAME message, restarting game" << std::endl;
                ResetGame();
            } else {
                std::cout << "Ignoring RESTART_GAME message while in state: " << static_cast<int>(currentState) << std::endl;
            }
            break;
            
        case MessageType::PAUSE_GAME:
            // Toggle pause
            TogglePause();
            break;
    }
}

void Game::StartHost() {
    // Initialize as host
    if (networkManager.Initialize(NetworkRole::HOST, "0.0.0.0", DEFAULT_PORT)) {
        isHost = true;
        wasConnected = false;
        currentState = GameState::LOBBY;
    }
}

void Game::JoinGame(const std::string& address) {
    // Initialize as client
    if (networkManager.Initialize(NetworkRole::CLIENT, address, DEFAULT_PORT)) {
        isHost = false;
        wasConnected = false;
        currentState = GameState::LOBBY;
    }
}

void Game::StartGame() {
    // Reset scores and balls
    leftScore = 0;
    rightScore = 0;
    
    // Reset paddles to starting positions
    const float PADDLE_MARGIN = 50.0f;
    leftPaddle = Paddle(
        {PADDLE_MARGIN, SCREEN_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f},
        {PADDLE_WIDTH, PADDLE_HEIGHT},
        true,
        BLUE
    );
    
    rightPaddle = Paddle(
        {SCREEN_WIDTH - PADDLE_MARGIN - PADDLE_WIDTH, SCREEN_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f},
        {PADDLE_WIDTH, PADDLE_HEIGHT},
        false,
        RED
    );
    
    // Initialize balls in random positions near center
    InitializeBalls();
    
    // Clear any pending ball respawn timers
    ballRespawnTimers.clear();
    
    // Set game state to playing
    currentState = GameState::PLAYING;
}

void Game::ResetGame() {
    // Reset scores and balls
    leftScore = 0;
    rightScore = 0;
    
    // Reset paddles to starting positions
    const float PADDLE_MARGIN = 50.0f;
    leftPaddle = Paddle(
        {PADDLE_MARGIN, SCREEN_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f},
        {PADDLE_WIDTH, PADDLE_HEIGHT},
        true,
        BLUE
    );
    
    rightPaddle = Paddle(
        {SCREEN_WIDTH - PADDLE_MARGIN - PADDLE_WIDTH, SCREEN_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f},
        {PADDLE_WIDTH, PADDLE_HEIGHT},
        false,
        RED
    );
    
    // Initialize balls in random positions near center
    InitializeBalls();
    
    // Clear any pending ball respawn timers
    ballRespawnTimers.clear();
    
    // Set game state to playing
    currentState = GameState::PLAYING;
}

void Game::TogglePause() {
    // Toggle between playing and paused
    if (currentState == GameState::PLAYING) {
        currentState = GameState::PAUSED;
    } else if (currentState == GameState::PAUSED) {
        currentState = GameState::PLAYING;
    }
}

void Game::InitializeBalls() {
    // Clear existing balls
    balls.clear();
    
    // Create 5 balls with different colors and velocities
    Color colors[] = {RED, GREEN, BLUE, YELLOW, PURPLE};
    
    // Define spawn area radius around center
    const float SPAWN_RADIUS = 100.0f;
    
    // Track positions to avoid spawning balls too close to each other
    std::vector<Vector2> positions;
    
    for (int i = 0; i < BALL_COUNT; i++) {
        // Random direction for velocity
        const int MAX_ANGLE_DEGREES = 360;
        float angle = static_cast<float>(GetRandomValue(0, MAX_ANGLE_DEGREES)) * DEG2RAD;
        Vector2 velocity = {
            std::cos(angle) * BALL_SPEED,
            std::sin(angle) * BALL_SPEED
        };
        
        // Random position within spawn radius
        Vector2 position;
        bool validPosition = false;
        int attempts = 0;
        
        // Try to find a valid position that's not too close to other balls
        const int MAX_POSITION_ATTEMPTS = 20;
        while (!validPosition && attempts < MAX_POSITION_ATTEMPTS) {
            // Random angle and distance from center
            float posAngle = static_cast<float>(GetRandomValue(0, 360)) * DEG2RAD;
            float distance = static_cast<float>(GetRandomValue(0, static_cast<int>(SPAWN_RADIUS)));
            
            // Calculate position
            position.x = SCREEN_WIDTH / 2.0f + std::cos(posAngle) * distance;
            position.y = SCREEN_HEIGHT / 2.0f + std::sin(posAngle) * distance;
            
            // Check if position is valid (not too close to other balls)
            validPosition = true;
            for (const auto& pos : positions) {
                float dx = position.x - pos.x;
                float dy = position.y - pos.y;
                float dist = std::sqrt(dx * dx + dy * dy);
                
                // If too close to another ball, try again
                if (dist < DEFAULT_BALL_RADIUS * 3.0f) {
                    validPosition = false;
                    break;
                }
            }
            
            attempts++;
        }
        
        // If we couldn't find a valid position, use a fallback
        if (!validPosition) {
            position.x = SCREEN_WIDTH / 2.0f + (i - BALL_COUNT / 2.0f) * DEFAULT_BALL_RADIUS * 3.0f;
            position.y = SCREEN_HEIGHT / 2.0f;
        }
        
        // Add position to list
        positions.push_back(position);
        
        // Random size for the ball
        float radius = static_cast<float>(GetRandomValue(MIN_BALL_RADIUS, MAX_BALL_RADIUS));
        
        // Create ball
        balls.emplace_back(
            position,
            radius,
            colors[i],
            velocity
        );
    }
}

void Game::CheckBallCollisions() {
    // Check collisions between all pairs of balls
    for (size_t i = 0; i < balls.size(); i++) {
        for (size_t j = i + 1; j < balls.size(); j++) {
            balls[i].CheckBallCollision(balls[j]);
        }
    }
}

void Game::CheckScoring() {
    // Don't process scoring if the game is already over
    if (currentState == GameState::GAME_OVER) {
        return;
    }
    
    bool gameOverDetected = false;
    
    // Use an iterator to safely remove balls from the vector
    for (auto it = balls.begin(); it != balls.end();) {
        // Skip further processing if game over was detected
        if (gameOverDetected) {
            ++it;
            continue;
        }
        
        bool removeBall = false;
        
        // Check if ball passed left paddle
        if (it->GetPosition().x < 0) {
            // Right player scores
            rightScore++;
            
            // Only the host should create respawn timers
            if (isHost) {
                const float MIN_RESPAWN_TIME = 2.0f;
                const float MAX_ADDITIONAL_RESPAWN_TIME = 3.0f;
                float respawnTime = MIN_RESPAWN_TIME + static_cast<float>(GetRandomValue(0, static_cast<int>(MAX_ADDITIONAL_RESPAWN_TIME * 10.0f))) / 10.0f;
                ballRespawnTimers.push_back({respawnTime, respawnTime});
            }
            
            // Mark ball for removal
            removeBall = true;
            
            // Check for game over
            if (rightScore >= WINNING_SCORE) {
                currentState = GameState::GAME_OVER;
                gameOverDetected = true;
                std::cout << "Game over detected: Right player wins!" << std::endl;
            }
        }
        
        // Check if ball passed right paddle
        if (!gameOverDetected && !removeBall && it->GetPosition().x > SCREEN_WIDTH) {
            // Left player scores
            leftScore++;
            
            // Only the host should create respawn timers
            if (isHost) {
                const float MIN_RESPAWN_TIME = 2.0f;
                const float MAX_ADDITIONAL_RESPAWN_TIME = 3.0f;
                float respawnTime = MIN_RESPAWN_TIME + static_cast<float>(GetRandomValue(0, static_cast<int>(MAX_ADDITIONAL_RESPAWN_TIME * 10.0f))) / 10.0f;
                ballRespawnTimers.push_back({respawnTime, respawnTime});
            }
            
            // Mark ball for removal
            removeBall = true;
            
            // Check for game over
            if (leftScore >= WINNING_SCORE) {
                currentState = GameState::GAME_OVER;
                gameOverDetected = true;
                std::cout << "Game over detected: Left player wins!" << std::endl;
            }
        }
        
        // Remove the ball if it went off-screen
        if (removeBall) {
            it = balls.erase(it);
        } else {
            ++it;
        }
    }
}

void Game::SendGameState() {
    // Only send every few frames to reduce network traffic
    static int frameCounter = 0;
    const int NETWORK_UPDATE_INTERVAL = 10;  // Send every 10 frames
    
    frameCounter++;
    if (frameCounter % NETWORK_UPDATE_INTERVAL != 0) {
        return;
    }
    
    Packet packet;
    
    // Write game state
    packet.Write(static_cast<int>(currentState));
    packet.Write(leftScore);
    packet.Write(rightScore);
    
    // Write paddle positions
    leftPaddle.Serialize(packet);
    rightPaddle.Serialize(packet);
    
    // Write ball data
    packet.Write(static_cast<int>(balls.size()));
    for (auto& ball : balls) {
        ball.Serialize(packet);
    }
    
    // Write ball respawn timer data
    packet.Write(static_cast<int>(ballRespawnTimers.size()));
    for (const auto& timer : ballRespawnTimers) {
        packet.Write(timer.timer);
        packet.Write(timer.originalTimer);
    }
    
    // Try to send the packet multiple times to increase chances of success
    const int MAX_SEND_ATTEMPTS = 3;
    bool anySent = false;
    
    for (int i = 0; i < MAX_SEND_ATTEMPTS; i++) {
        bool sent = networkManager.SendPacket(MessageType::GAME_STATE, packet);
        anySent = anySent || sent;
        
        if (sent) {
            break;  // If successful, no need to try again
        }
    }
    
    std::cout << "Host: Sending GAME_STATE message, success: " << (anySent ? "true" : "false") << std::endl;
}

void Game::ProcessGameState(Packet& packet) {
    // Read game state
    int stateValue;
    packet.Read(stateValue);
    GameState newState = static_cast<GameState>(stateValue);
    
    std::cout << "Client: Received GAME_STATE message, state: " << static_cast<int>(newState) 
              << ", current state: " << static_cast<int>(currentState) << std::endl;
    
    // Don't allow automatic transition from GAME_OVER to PLAYING
    // This prevents the game from restarting automatically after game over
    if (currentState == GameState::GAME_OVER && newState == GameState::PLAYING) {
        std::cout << "Client: Ignoring automatic transition from GAME_OVER to PLAYING" << std::endl;
        // Keep the GAME_OVER state until player explicitly restarts
    } else {
        // Update current state for other state transitions
        currentState = newState;
    }
    
    // Read scores
    packet.Read(leftScore);
    packet.Read(rightScore);
    
    // Read paddle positions
    leftPaddle.Deserialize(packet);
    rightPaddle.Deserialize(packet);
    
    // Read ball data
    int ballCount;
    packet.Read(ballCount);
    
    // Ensure we have the right number of balls
    if (balls.size() != ballCount) {
        // Resize the balls vector to match the received count
        if (balls.size() < ballCount) {
            // Add new balls if we have fewer than needed
            while (balls.size() < ballCount) {
                // Create a default ball at center of screen
                // The actual properties will be set by Deserialize below
                balls.emplace_back(
                    Vector2{SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f},
                    DEFAULT_BALL_RADIUS,
                    WHITE,
                    Vector2{0, 0}
                );
            }
        } else {
            // Remove excess balls if we have more than needed
            balls.resize(ballCount);
        }
    }
    
    // Update ball data
    for (int i = 0; i < ballCount; i++) {
        balls[i].Deserialize(packet);
    }
    
    // Read ball respawn timer data
    int timerCount;
    packet.Read(timerCount);
    
    // Clear existing timers and read new ones
    ballRespawnTimers.clear();
    for (int i = 0; i < timerCount; i++) {
        float timer, originalTimer;
        packet.Read(timer);
        packet.Read(originalTimer);
        ballRespawnTimers.push_back({timer, originalTimer});
    }
}

void Game::ProcessInputMessage(Packet& packet) {
    // Read input from other player
    bool upPressed, downPressed;
    packet.Read(upPressed);
    packet.Read(downPressed);
    
    // Apply input to appropriate paddle
    if (isHost) {
        // Client controls right paddle
        rightPaddleUp = upPressed;
        rightPaddleDown = downPressed;
    } else {
        // Host controls left paddle
        leftPaddleUp = upPressed;
        leftPaddleDown = downPressed;
    }
}
