#pragma once

#include <raylib.h>
#include "Packet.h"
#include "Paddle.h"

// Ball class for game objects
class Ball {
public:
    Ball();
    Ball(Vector2 position, float radius, Color color, Vector2 initialVelocity);

    // Update ball position
    void Update(float deltaTime);
    
    // Render the ball
    void Render();
    
    // Check collision with walls and handle bouncing
    bool CheckWallCollision(int screenWidth, int screenHeight);
    
    // Check collision with paddle and handle bouncing using physics-based reflection
    // The ball will reflect based on its incoming angle and the paddle's surface normal
    // The hit position on the paddle will also influence the bounce direction
    bool CheckPaddleCollision(const Paddle& paddle);
    
    // Check collision with another ball and handle bouncing
    bool CheckBallCollision(Ball& otherBall);
    
    // Reset ball to initial position
    void Reset(Vector2 position, Vector2 initialVelocity);
    
    // Serialize ball data for network transmission
    void Serialize(Packet& packet) const;
    
    // Deserialize ball data from network
    void Deserialize(Packet& packet);
    
    // Getters
    Vector2 GetPosition() const { return position; }
    Vector2 GetVelocity() const { return velocity; }
    float GetRadius() const { return radius; }
    
    // Setters
    void SetPosition(Vector2 newPosition) { position = newPosition; }
    void SetVelocity(Vector2 newVelocity) { velocity = newVelocity; }

private:
    Vector2 position;
    Vector2 velocity;
    float speed;
    float radius;
    Color color;
    
    // Interpolation variables
    Vector2 previousPosition;
    Vector2 targetPosition;
    float interpolationFactor;
    static constexpr float INTERPOLATION_SPEED = 10.0f; // Adjust for smoother/faster interpolation
};
