#include "Ball.h"
#include "Game.h"
#include <cmath>

Ball::Ball() 
    : position({0, 0}), velocity({0, 0}), speed(0), radius(0), color(WHITE),
      previousPosition({0, 0}), targetPosition({0, 0}), interpolationFactor(1.0f) {
}

Ball::Ball(Vector2 position, float radius, Color color, Vector2 initialVelocity)
    : position(position), velocity(initialVelocity), speed(BALL_SPEED), radius(radius), color(color),
      previousPosition(position), targetPosition(position), interpolationFactor(1.0f) {
    
    const float velocityLength = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    if (velocityLength > 0) {
        velocity.x = (velocity.x / velocityLength) * speed;
        velocity.y = (velocity.y / velocityLength) * speed;
    }
}

void Ball::Update(float deltaTime) {
    const float MAX_INTERPOLATION = 1.0f;
    
    if (interpolationFactor < MAX_INTERPOLATION) {
        interpolationFactor += deltaTime * INTERPOLATION_SPEED;
        if (interpolationFactor > MAX_INTERPOLATION) {
            interpolationFactor = MAX_INTERPOLATION;
        }
        
        if (interpolationFactor >= MAX_INTERPOLATION) {
            position = targetPosition;
        }
    } else {
        previousPosition = position;
        
        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;
        
        targetPosition = position;
    }
}

void Ball::Render() {
    Vector2 renderPosition;
    renderPosition.x = previousPosition.x + (targetPosition.x - previousPosition.x) * interpolationFactor;
    renderPosition.y = previousPosition.y + (targetPosition.y - previousPosition.y) * interpolationFactor;
    
    DrawCircleV(renderPosition, radius, color);
}

bool Ball::CheckWallCollision(int screenWidth, int screenHeight) {
    bool collided = false;
    
    const float TOP_WALL_Y = 0;
    if (position.y - radius < TOP_WALL_Y) {
        position.y = radius;
        velocity.y = -velocity.y;
        collided = true;
    } else if (position.y + radius > screenHeight) {
        position.y = screenHeight - radius;
        velocity.y = -velocity.y;
        collided = true;
    }
    
    return collided;
}

bool Ball::CheckPaddleCollision(const Paddle& paddle) {
    Rectangle paddleBounds = paddle.GetBounds();
    
    if (CheckCollisionCircleRec(position, radius, paddleBounds)) {
        // Determine which side of the paddle was hit
        const bool hitLeftSide = position.x < paddleBounds.x;
        const bool hitRightSide = position.x > paddleBounds.x + paddleBounds.width;
        
        // Calculate normal vector (pointing away from paddle)
        Vector2 normal;
        if (hitLeftSide) {
            normal = {-1.0f, 0.0f}; // Left side normal points left
        } else if (hitRightSide) {
            normal = {1.0f, 0.0f};  // Right side normal points right
        } else {
            // If we hit the top or bottom (rare case), use appropriate normal
            if (position.y < paddleBounds.y) {
                normal = {0.0f, -1.0f}; // Top normal points up
            } else {
                normal = {0.0f, 1.0f};  // Bottom normal points down
            }
        }
        
        // Calculate reflection using the formula R = V - 2(V·N)N
        const float dotProduct = velocity.x * normal.x + velocity.y * normal.y;
        
        // Apply reflection
        velocity.x = velocity.x - 2.0f * dotProduct * normal.x;
        velocity.y = velocity.y - 2.0f * dotProduct * normal.y;
        
        // Apply paddle influence based on hit position
        const float paddleCenter = paddleBounds.y + paddleBounds.height / 2.0f;
        const float paddleHalfHeight = paddleBounds.height / 2.0f;
        const float hitPosition = (position.y - paddleCenter) / paddleHalfHeight;
        
        // Add a vertical component based on where the paddle was hit
        // This gives the player some control over the bounce direction
        const float paddleInfluence = 0.5f; // How much the paddle hit position affects the bounce
        velocity.y += hitPosition * speed * paddleInfluence;
        
        // Normalize and apply speed (possibly with a small boost)
        const float speedBoost = 1.05f; // 5% speed boost on paddle hit
        const float velocityLength = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
        if (velocityLength > 0) {
            velocity.x = (velocity.x / velocityLength) * speed * speedBoost;
            velocity.y = (velocity.y / velocityLength) * speed * speedBoost;
        }
        
        // Move ball outside of paddle to prevent multiple collisions
        if (velocity.x > 0) {
            position.x = paddleBounds.x + paddleBounds.width + radius;
        } else {
            position.x = paddleBounds.x - radius;
        }
        
        return true;
    }
    
    return false;
}

bool Ball::CheckBallCollision(Ball& otherBall) {
    const float dx = otherBall.position.x - position.x;
    const float dy = otherBall.position.y - position.y;
    const float distance = std::sqrt(dx * dx + dy * dy);
    const float combinedRadius = radius + otherBall.radius;
    
    if (distance < combinedRadius) {
        const float nx = dx / distance;
        const float ny = dy / distance;
        
        const float dvx = otherBall.velocity.x - velocity.x;
        const float dvy = otherBall.velocity.y - velocity.y;
        
        const float dotProduct = nx * dvx + ny * dvy;
        
        if (dotProduct > 0) {
            const float massRatio = 1.0f;  // Equal mass for both balls
            const float impulse = 2.0f * dotProduct / (massRatio + 1.0f);
            
            velocity.x += impulse * nx;
            velocity.y += impulse * ny;
            otherBall.velocity.x -= impulse * nx;
            otherBall.velocity.y -= impulse * ny;
            
            const float overlap = (combinedRadius - distance) / 2.0f;
            position.x -= overlap * nx;
            position.y -= overlap * ny;
            otherBall.position.x += overlap * nx;
            otherBall.position.y += overlap * ny;
            
            return true;
        }
    }
    
    return false;
}

void Ball::Reset(Vector2 position, Vector2 initialVelocity) {
    this->position = position;
    this->velocity = initialVelocity;
    
    const float MAX_INTERPOLATION = 1.0f;
    this->previousPosition = position;
    this->targetPosition = position;
    this->interpolationFactor = MAX_INTERPOLATION;
    
    const float velocityLength = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    if (velocityLength > 0) {
        velocity.x = (velocity.x / velocityLength) * speed;
        velocity.y = (velocity.y / velocityLength) * speed;
    }
}

void Ball::Serialize(Packet& packet) const {
    packet.Write(position.x);
    packet.Write(position.y);
    packet.Write(velocity.x);
    packet.Write(velocity.y);
    packet.Write(color.r);
    packet.Write(color.g);
    packet.Write(color.b);
    packet.Write(color.a);
}

void Ball::Deserialize(Packet& packet) {
    previousPosition = position;
    
    float newX, newY;
    packet.Read(newX);
    packet.Read(newY);
    
    targetPosition.x = newX;
    targetPosition.y = newY;
    
    const float RESET_INTERPOLATION = 0.0f;
    interpolationFactor = RESET_INTERPOLATION;
    
    packet.Read(velocity.x);
    packet.Read(velocity.y);
    
    packet.Read(color.r);
    packet.Read(color.g);
    packet.Read(color.b);
    packet.Read(color.a);
}
