#include "Paddle.h"
#include "Game.h"

Paddle::Paddle() 
    : position({0, 0}), size({0, 0}), speed(0), isLeftPaddle(true), color(WHITE) {
}

Paddle::Paddle(Vector2 position, Vector2 size, bool isLeftPaddle, Color color)
    : position(position), size(size), speed(PADDLE_SPEED), isLeftPaddle(isLeftPaddle), color(color) {
}

void Paddle::Update(bool upPressed, bool downPressed, float deltaTime, int screenHeight) {
    // Move paddle based on input
    if (upPressed) {
        position.y -= speed * deltaTime;
    }
    if (downPressed) {
        position.y += speed * deltaTime;
    }
    
    // Clamp paddle position to screen bounds
    if (position.y < 0) {
        position.y = 0;
    }
    if (position.y + size.y > screenHeight) {
        position.y = screenHeight - size.y;
    }
}

void Paddle::Render() {
    DrawRectangleV(position, size, color);
}

Rectangle Paddle::GetBounds() const {
    return {position.x, position.y, size.x, size.y};
}

void Paddle::Serialize(Packet& packet) const {
    packet.Write(position.x);
    packet.Write(position.y);
}

void Paddle::Deserialize(Packet& packet) {
    packet.Read(position.x);
    packet.Read(position.y);
}
