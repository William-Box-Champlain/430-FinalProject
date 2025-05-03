#include "Game.h"
#include <iostream>

int main() {
    try {
        // Create and initialize game
        Game game;
        game.Initialize();
        
        // Run game
        game.Run();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}
