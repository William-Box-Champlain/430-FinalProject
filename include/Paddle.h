#pragma once

#include <raylib.h>
#include "Packet.h"

// Paddle class for player controls
class Paddle {
public:
    Paddle();
    Paddle(Vector2 position, Vector2 size, bool isLeftPaddle, Color color);

    // Update paddle position based on input
    void Update(bool upPressed, bool downPressed, float deltaTime, int screenHeight);
    
    // Render the paddle
    void Render();
    
    // Get the paddle's collision rectangle
    Rectangle GetBounds() const;
    
    // Serialize paddle data for network transmission
    void Serialize(Packet& packet) const;
    
    // Deserialize paddle data from network
    void Deserialize(Packet& packet);
    
    // Getters
    Vector2 GetPosition() const { return position; }
    Vector2 GetSize() const { return size; }
    
    // Setters
    void SetPosition(Vector2 newPosition) { position = newPosition; }

private:
    Vector2 position;
    Vector2 size;
    float speed;
    bool isLeftPaddle;
    Color color;
};
