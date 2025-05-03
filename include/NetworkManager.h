#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "socklib.h"
#include "SocketWrapper.h"
#include "GameState.h"
#include "Packet.h"

// Message types for network communication
enum class MessageType : uint8_t {
    CONNECT,        // Initial connection
    DISCONNECT,     // Disconnect notification
    GAME_STATE,     // Full game state update
    INPUT,          // Player input
    START_GAME,     // Start game signal
    RESTART_GAME,   // Restart game signal
    PAUSE_GAME      // Pause/unpause game
};

// Connection state for non-blocking operations
enum class ConnectionState : uint8_t {
    NONE,           // Not connected or connecting
    CONNECTING,     // Attempting to connect
    CONNECTED,      // Successfully connected
    FAILED          // Connection failed
};

// Network manager class for handling socket communication
class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    // Initialize as host or client
    bool Initialize(NetworkRole role, const std::string& address, int port);
    
    // Shutdown network connection
    void Shutdown();
    
    // Update network state (non-blocking)
    void Update(float deltaTime);
    
    // Send a packet
    bool SendPacket(MessageType type, const Packet& packet);
    
    // Check if connected
    bool IsConnected() const { return connected; }
    
    // Check if host
    bool IsHost() const { return role == NetworkRole::HOST; }
    
    // Set callback for received packets
    void SetMessageCallback(std::function<void(MessageType, Packet&)> callback) {
        messageCallback = callback;
    }

private:
    // Static members for tracking SockLib initialization
    static bool sockLibInitialized;
    static int instanceCount;
    
    std::shared_ptr<Socket> serverSocket;
    std::shared_ptr<Socket> clientSocket;
    std::unique_ptr<SocketWrapper> serverWrapper;
    std::unique_ptr<SocketWrapper> clientWrapper;
    NetworkRole role;
    bool connected;
    std::vector<char> recvBuffer;
    std::function<void(MessageType, Packet&)> messageCallback;
    
    // Connection state tracking
    ConnectionState connectionState;
    std::string connectionAddress;
    int connectionPort;
    float connectionTimeout;
    float connectionTimer;
    
    // Connection stabilization
    float connectionStabilizeDelay;  // Delay before starting to receive data after connection
    float stabilizeTimer;            // Timer for connection stabilization
    
    // Polling interval
    static constexpr float POLL_INTERVAL = 0.01f; // Poll every 10ms
    
    // Helper function to log network state
    void LogNetworkState(const std::string& functionName);
    
    // Process received data
    void ProcessReceivedData(const char* data, size_t size);
    
    // Non-blocking network operations
    bool TryAcceptConnection();
    bool TryReceiveData();
};
