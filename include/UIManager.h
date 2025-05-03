#pragma once

#include <string>
#include <raylib.h>
#include "GameState.h"

// Button class for UI elements
class Button {
public:
    Button() = default;
    Button(Rectangle bounds, const std::string& text, Color color, Color hoverColor);
    
    // Update button state
    bool Update(Vector2 mousePosition);
    
    // Render the button
    void Render();
    
    // Check if button was clicked
    bool IsClicked() const { return clicked; }
    
    // Reset button state
    void Reset() { clicked = false; }

private:
    Rectangle bounds;
    std::string text;
    Color color;
    Color hoverColor;
    bool hovered;
    bool clicked;
};

// Text input box class
class TextBox {
public:
    TextBox() = default;
    TextBox(Rectangle bounds, const std::string& defaultText, int maxLength);
    
    // Update text box with keyboard input
    void Update();
    
    // Render the text box
    void Render();
    
    // Get the current text
    std::string GetText() const { return text; }
    
    // Set focus state
    void SetFocus(bool focus) { focused = focus; }
    
    // Check if focused
    bool IsFocused() const { return focused; }

private:
    Rectangle bounds;
    std::string text;
    std::string placeholder;
    int maxLength;
    bool focused;
    float cursorBlinkTime;
    bool showCursor;
};

// UI Manager class for handling all UI elements
class UIManager {
public:
    UIManager();
    
    // Initialize UI elements
    void Initialize();
    
    // Reset UI elements to their default positions
    void ResetUI();
    
    // Render UI based on game state
    void Render(GameState state, bool isHost, bool connected, int leftScore, int rightScore);
    
    // Update UI elements
    void Update();
    
    // Get IP address from text box
    std::string GetIPAddress() const;
    
    // Check if host button was clicked
    bool IsHostButtonClicked();
    
    // Check if join button was clicked
    bool IsJoinButtonClicked();
    
    // Check if restart button was clicked
    bool IsRestartButtonClicked();
    
    // Check if back button was clicked
    bool IsBackButtonClicked();
    
    // Check if start game button was clicked
    bool IsStartGameButtonClicked();

private:
    // UI elements
    Button hostButton;
    Button joinButton;
    Button startGameButton;
    Button restartButton;
    Button backButton;
    TextBox ipAddressInput;
    
    // UI colors
    Color backgroundColor;
    Color textColor;
    Color accentColor;
    
    // Font for text rendering
    Font gameFont;
    
    // Helper methods for rendering different screens
    void RenderMenu();
    void RenderLobby(bool isHost, bool connected);
    void RenderHUD(int leftScore, int rightScore);
    void RenderPauseScreen();
    void RenderGameOverScreen(int leftScore, int rightScore);
};
