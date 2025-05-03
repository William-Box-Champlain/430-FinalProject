# Multiball Pong

A networked multiplayer version of Pong with 5 balls, created for the final project of the networking course.

## Features

- Multiplayer Pong with 5 balls that bounce off walls, paddles, and each other
- TCP networking for reliable gameplay
- Host/client architecture
- Simple UI for hosting or joining games
- Game state synchronization
- Score tracking and win conditions
- Ability to restart the game without restarting the program

## Requirements

- C++17 compatible compiler
- CMake 3.10 or higher
- Raylib (automatically downloaded by CMake if not found)

## Building the Game

### Windows

1. Make sure you have CMake and a C++ compiler (like Visual Studio or MinGW) installed
2. Clone the repository
3. Open a command prompt in the project directory
4. Run the following commands:

```
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

5. The executable will be in the `bin` directory

### Linux

1. Make sure you have CMake and a C++ compiler installed
2. Install required dependencies:
   ```
   sudo apt-get install build-essential libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev libxinerama-dev libxcursor-dev
   ```
3. Clone the repository
4. Open a terminal in the project directory
5. Run the following commands:

```
mkdir build
cd build
cmake ..
make
```

6. The executable will be in the `bin` directory

## How to Play

### Starting a Game

1. One player must host the game by clicking "Host Game" on the main menu
2. The other player must join by clicking "Join Game" and entering the host's IP address
3. Once connected, the host can press SPACE to start the game

### Controls

- Left paddle: W (up) and S (down)
- Right paddle: Up Arrow (up) and Down Arrow (down)
- Pause/unpause: P
- Return to menu: ESC

### Game Rules

- Each player controls a paddle on their side of the screen
- 5 balls bounce around the screen
- When a ball passes a player's paddle, the opponent scores a point
- First player to reach 10 points wins
- Balls bounce off walls, paddles, and each other
- The angle of the bounce depends on where the ball hits the paddle

## Network Architecture

- Uses TCP sockets for reliable communication
- Host acts as the server and maintains the authoritative game state
- Client sends input to the host and receives game state updates
- Game runs at 60 FPS with non-blocking network operations

## Troubleshooting

- If you can't connect, make sure your firewall allows the game to access the network
- The default port is 7777, make sure this port is open if you're hosting
- If the game crashes, check the console output for error messages
