#include "UIManager.h"
#include "Game.h"
#include <string>

Button::Button(Rectangle bounds, const std::string& text, Color color, Color hoverColor)
    : bounds(bounds), text(text), color(color), hoverColor(hoverColor), hovered(false), clicked(false) {
}

bool Button::Update(Vector2 mousePosition) {
    hovered = CheckCollisionPointRec(mousePosition, bounds);
    
    if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        clicked = true;
        return true;
    }
    
    return false;
}

void Button::Render() {
    DrawRectangleRec(bounds, hovered ? hoverColor : color);
    
    DrawRectangleLinesEx(bounds, 2, BLACK);
    
    int fontSize = 20;
    Vector2 textSize = MeasureTextEx(GetFontDefault(), text.c_str(), fontSize, 1);
    Vector2 textPosition = {
        bounds.x + (bounds.width - textSize.x) / 2,
        bounds.y + (bounds.height - textSize.y) / 2
    };
    
    DrawTextEx(GetFontDefault(), text.c_str(), textPosition, fontSize, 1, BLACK);
}

TextBox::TextBox(Rectangle bounds, const std::string& defaultText, int maxLength)
    : bounds(bounds), text(defaultText), placeholder("Enter IP address..."), 
      maxLength(maxLength), focused(false), cursorBlinkTime(0), showCursor(false) {
}

void TextBox::Update() {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        focused = CheckCollisionPointRec(GetMousePosition(), bounds);
    }
    
    if (focused) {
        int key = GetCharPressed();
        
        while (key > 0 && text.length() < maxLength) {
            if ((key >= '0' && key <= '9') || key == '.') {
                text += (char)key;
            }
            key = GetCharPressed();
        }
        
        if (IsKeyPressed(KEY_BACKSPACE) && !text.empty()) {
            text.pop_back();
        }
        
        cursorBlinkTime += GetFrameTime();
        if (cursorBlinkTime >= 0.5f) {
            cursorBlinkTime = 0;
            showCursor = !showCursor;
        }
    }
}

void TextBox::Render() {
    DrawRectangleRec(bounds, WHITE);
    
    DrawRectangleLinesEx(bounds, focused ? 2 : 1, focused ? BLUE : GRAY);
    
    int fontSize = 20;
    std::string displayText = text.empty() ? placeholder : text;
    Color textColor = text.empty() ? GRAY : BLACK;
    
    Vector2 textPosition = {
        bounds.x + 10,
        bounds.y + (bounds.height - fontSize) / 2
    };
    
    DrawTextEx(GetFontDefault(), displayText.c_str(), textPosition, fontSize, 1, textColor);
    
    if (focused && showCursor && !text.empty()) {
        float cursorX = textPosition.x + MeasureText(text.c_str(), fontSize);
        DrawRectangle(cursorX, textPosition.y, 2, fontSize, BLACK);
    }
}

UIManager::UIManager()
    : backgroundColor(RAYWHITE), textColor(BLACK), accentColor(BLUE) {
}

void UIManager::Initialize() {
    int buttonWidth = 200;
    int buttonHeight = 50;
    int centerX = SCREEN_WIDTH / 2 - buttonWidth / 2;
    
    hostButton = Button(
        {(float)centerX, 200, (float)buttonWidth, (float)buttonHeight},
        "Host Game",
        LIGHTGRAY,
        GRAY
    );
    
    joinButton = Button(
        {(float)centerX, 270, (float)buttonWidth, (float)buttonHeight},
        "Join Game",
        LIGHTGRAY,
        GRAY
    );
    
    startGameButton = Button(
        {(float)centerX, 270, (float)buttonWidth, (float)buttonHeight},
        "Start Game",
        GREEN,
        DARKGREEN
    );
    
    restartButton = Button(
        {(float)centerX, 340, (float)buttonWidth, (float)buttonHeight},
        "Restart Game",
        LIGHTGRAY,
        GRAY
    );
    
    backButton = Button(
        {(float)centerX, 410, (float)buttonWidth, (float)buttonHeight},
        "Back to Menu",
        LIGHTGRAY,
        GRAY
    );
    
    ipAddressInput = TextBox(
        {(float)centerX, 340, (float)buttonWidth, (float)buttonHeight},
        "127.0.0.1",
        15
    );
    
    gameFont = GetFontDefault();
}

void UIManager::ResetUI() {
    int buttonWidth = 200;
    int buttonHeight = 50;
    int centerX = SCREEN_WIDTH / 2 - buttonWidth / 2;
    
    hostButton = Button(
        {(float)centerX, 200, (float)buttonWidth, (float)buttonHeight},
        "Host Game",
        LIGHTGRAY,
        GRAY
    );
    
    joinButton = Button(
        {(float)centerX, 270, (float)buttonWidth, (float)buttonHeight},
        "Join Game",
        LIGHTGRAY,
        GRAY
    );
    
    startGameButton = Button(
        {(float)centerX, 270, (float)buttonWidth, (float)buttonHeight},
        "Start Game",
        GREEN,
        DARKGREEN
    );
    
    restartButton = Button(
        {(float)centerX, 340, (float)buttonWidth, (float)buttonHeight},
        "Restart Game",
        LIGHTGRAY,
        GRAY
    );
    
    backButton = Button(
        {(float)centerX, 410, (float)buttonWidth, (float)buttonHeight},
        "Back to Menu",
        LIGHTGRAY,
        GRAY
    );
    
    // Reset text box
    ipAddressInput = TextBox(
        {(float)centerX, 340, (float)buttonWidth, (float)buttonHeight},
        "127.0.0.1",
        15
    );
}

void UIManager::Render(GameState state, bool isHost, bool connected, int leftScore, int rightScore) {
    switch (state) {
        case GameState::MENU:
            RenderMenu();
            break;
        case GameState::LOBBY:
            RenderLobby(isHost, connected);
            break;
        case GameState::PLAYING:
            RenderHUD(leftScore, rightScore);
            break;
        case GameState::PAUSED:
            RenderHUD(leftScore, rightScore);
            RenderPauseScreen();
            break;
        case GameState::GAME_OVER:
            RenderHUD(leftScore, rightScore);
            RenderGameOverScreen(leftScore, rightScore);
            break;
    }
}

void UIManager::Update() {
    // Update buttons based on current state
    Vector2 mousePosition = GetMousePosition();
    
    hostButton.Update(mousePosition);
    joinButton.Update(mousePosition);
    startGameButton.Update(mousePosition);
    restartButton.Update(mousePosition);
    backButton.Update(mousePosition);
    
    // Update text box
    ipAddressInput.Update();
}

bool UIManager::IsStartGameButtonClicked() {
    bool clicked = startGameButton.IsClicked();
    startGameButton.Reset();
    return clicked;
}

std::string UIManager::GetIPAddress() const {
    return ipAddressInput.GetText();
}

bool UIManager::IsHostButtonClicked() {
    bool clicked = hostButton.IsClicked();
    hostButton.Reset();
    return clicked;
}

bool UIManager::IsJoinButtonClicked() {
    bool clicked = joinButton.IsClicked();
    joinButton.Reset();
    return clicked;
}

bool UIManager::IsRestartButtonClicked() {
    bool clicked = restartButton.IsClicked();
    restartButton.Reset();
    return clicked;
}

bool UIManager::IsBackButtonClicked() {
    bool clicked = backButton.IsClicked();
    backButton.Reset();
    return clicked;
}

void UIManager::RenderMenu() {
    // Draw title
    const char* title = "MULTIBALL PONG";
    int fontSize = 40;
    Vector2 textSize = MeasureTextEx(gameFont, title, fontSize, 1);
    Vector2 textPosition = {
        (SCREEN_WIDTH - textSize.x) / 2,
        100
    };
    
    DrawTextEx(gameFont, title, textPosition, fontSize, 1, textColor);
    
    // Draw buttons
    hostButton.Render();
    joinButton.Render();
    
    // Draw IP address input
    const char* ipLabel = "Server IP:";
    DrawText(ipLabel, SCREEN_WIDTH / 2 - 110 - MeasureText(ipLabel, 20), 355, 20, textColor);
    ipAddressInput.Render();
    
    // Draw instructions
    const char* instructions = "Press ESC to quit";
    DrawText(instructions, 10, SCREEN_HEIGHT - 30, 20, GRAY);
}

void UIManager::RenderLobby(bool isHost, bool connected) {
    // Draw title
    const char* title = isHost ? "HOSTING GAME" : "JOINING GAME";
    int fontSize = 40;
    Vector2 textSize = MeasureTextEx(gameFont, title, fontSize, 1);
    Vector2 textPosition = {
        (SCREEN_WIDTH - textSize.x) / 2,
        100
    };
    
    DrawTextEx(gameFont, title, textPosition, fontSize, 1, textColor);
    
    // Draw status
    const char* status = connected ? "Connected! Ready to start game." : "Waiting for connection...";
    fontSize = 24;
    textSize = MeasureTextEx(gameFont, status, fontSize, 1);
    textPosition = {
        (SCREEN_WIDTH - textSize.x) / 2,
        200
    };
    
    DrawTextEx(gameFont, status, textPosition, fontSize, 1, connected ? GREEN : ORANGE);
    
    // Draw IP address input for client if not connected
    if (!isHost && !connected) {
        const char* ipLabel = "Server IP:";
        DrawText(ipLabel, SCREEN_WIDTH / 2 - 100 - MeasureText(ipLabel, 20), 310, 20, textColor);
        ipAddressInput.Render();
        joinButton.Render();
    }
    
    // Draw start game button for host if connected
    if (isHost && connected) {
        startGameButton.Render();
        
        // Draw instructions for host
        const char* hostInstructions = "Click 'Start Game' when ready";
        DrawText(hostInstructions, SCREEN_WIDTH / 2 - MeasureText(hostInstructions, 20) / 2, 340, 20, GREEN);
    }
    
    // Draw back button
    backButton.Render();
    
    // Draw instructions
    const char* instructions = "Press ESC to return to menu";
    DrawText(instructions, 10, SCREEN_HEIGHT - 30, 20, GRAY);
}

void UIManager::RenderHUD(int leftScore, int rightScore) {
    // Draw scores
    int fontSize = 40;
    char scoreText[8];
    sprintf(scoreText, "%d", leftScore);
    DrawText(scoreText, SCREEN_WIDTH / 4, 20, fontSize, textColor);
    
    sprintf(scoreText, "%d", rightScore);
    DrawText(scoreText, 3 * SCREEN_WIDTH / 4, 20, fontSize, textColor);
    
    // Draw center line
    for (int y = 0; y < SCREEN_HEIGHT; y += 10) {
        DrawRectangle(SCREEN_WIDTH / 2 - 1, y, 2, 5, LIGHTGRAY);
    }
    
    // Draw instructions
    const char* instructions = "Press P to pause";
    DrawText(instructions, 10, SCREEN_HEIGHT - 30, 20, GRAY);
}

void UIManager::RenderPauseScreen() {
    // Draw semi-transparent overlay
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 150});
    
    // Draw pause text
    const char* pauseText = "GAME PAUSED";
    int fontSize = 40;
    Vector2 textSize = MeasureTextEx(gameFont, pauseText, fontSize, 1);
    Vector2 textPosition = {
        (SCREEN_WIDTH - textSize.x) / 2,
        SCREEN_HEIGHT / 3
    };
    
    DrawTextEx(gameFont, pauseText, textPosition, fontSize, 1, WHITE);
    
    // Draw buttons
    restartButton.Render();
    backButton.Render();
    
    // Draw instructions
    const char* instructions = "Press P to resume";
    DrawText(instructions, 10, SCREEN_HEIGHT - 30, 20, GRAY);
}

void UIManager::RenderGameOverScreen(int leftScore, int rightScore) {
    // Draw semi-transparent overlay
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 150});
    
    // Draw game over text
    const char* gameOverText = "GAME OVER";
    int fontSize = 40;
    Vector2 textSize = MeasureTextEx(gameFont, gameOverText, fontSize, 1);
    Vector2 textPosition = {
        (SCREEN_WIDTH - textSize.x) / 2,
        SCREEN_HEIGHT / 4
    };
    
    DrawTextEx(gameFont, gameOverText, textPosition, fontSize, 1, WHITE);
    
    // Draw winner text
    const char* winnerText;
    if (leftScore > rightScore) {
        winnerText = "LEFT PLAYER WINS!";
    } else if (rightScore > leftScore) {
        winnerText = "RIGHT PLAYER WINS!";
    } else {
        winnerText = "IT'S A TIE!";
    }
    
    fontSize = 30;
    textSize = MeasureTextEx(gameFont, winnerText, fontSize, 1);
    textPosition = {
        (SCREEN_WIDTH - textSize.x) / 2,
        SCREEN_HEIGHT / 3
    };
    
    DrawTextEx(gameFont, winnerText, textPosition, fontSize, 1, WHITE);
    
    // Draw buttons
    restartButton.Render();
    backButton.Render();
}
