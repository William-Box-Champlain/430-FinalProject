#include "NetworkManager.h"
#include <iostream>
#include <cstring>

// Static member to track if socklib is initialized
bool NetworkManager::sockLibInitialized = false;
// Static counter to track active NetworkManager instances
int NetworkManager::instanceCount = 0;

// Helper function to log the current network state - only used in debug builds
void NetworkManager::LogNetworkState(const std::string& functionName) {
#ifdef _DEBUG
    std::string roleStr;
    switch (role) {
        case NetworkRole::NONE: roleStr = "NONE"; break;
        case NetworkRole::HOST: roleStr = "HOST"; break;
        case NetworkRole::CLIENT: roleStr = "CLIENT"; break;
        default: roleStr = "UNKNOWN"; break;
    }
    
    std::string stateStr;
    switch (connectionState) {
        case ConnectionState::NONE: stateStr = "NONE"; break;
        case ConnectionState::CONNECTING: stateStr = "CONNECTING"; break;
        case ConnectionState::CONNECTED: stateStr = "CONNECTED"; break;
        case ConnectionState::FAILED: stateStr = "FAILED"; break;
        default: stateStr = "UNKNOWN"; break;
    }
    
    std::cout << "NetworkManager::" << functionName << " - Role: " << roleStr 
              << ", State: " << stateStr 
              << ", Connected: " << (connected ? "true" : "false") << std::endl;
#endif
}

NetworkManager::NetworkManager()
    : serverSocket(nullptr), clientSocket(nullptr), serverWrapper(nullptr), clientWrapper(nullptr),
      role(NetworkRole::NONE), connected(false), connectionState(ConnectionState::NONE), 
      connectionPort(0), connectionTimeout(5.0f), connectionTimer(0.0f),
      connectionStabilizeDelay(0.5f), stabilizeTimer(0.0f) {
    // Initialize receive buffer
    recvBuffer.resize(1024);
    
    // Increment instance counter
    instanceCount++;
}

NetworkManager::~NetworkManager() {
    Shutdown();
    
    // Decrement instance counter
    instanceCount--;
}

bool NetworkManager::Initialize(NetworkRole role, const std::string& address, int port) {
    LogNetworkState("Initialize");
    this->role = role;
    
    try {
        // Initialize socklib if not already initialized
        if (!sockLibInitialized) {
#ifdef _DEBUG
            std::cout << "Initializing SockLib" << std::endl;
#endif
            SockLibInit();
            sockLibInitialized = true;
        }
        
        if (role == NetworkRole::HOST) {
            try {
                // Create server socket
                serverSocket = std::make_shared<Socket>(Socket::INET, Socket::STREAM);
                
                // Bind to address and port
                if (serverSocket->Bind(Address(address, port)) != 0) {
                    std::cerr << "Failed to bind server socket" << std::endl;
                    serverSocket = nullptr;
                    return false;
                }
                
                // Start listening for connections
                if (serverSocket->Listen() != 0) {
                    std::cerr << "Failed to listen on server socket" << std::endl;
                    serverSocket = nullptr;
                    return false;
                }
                
                // Create server wrapper (use non-blocking mode like everything else)
                serverWrapper = std::make_unique<SocketWrapper>(serverSocket, true);
                
                // Log server start
                std::cout << "Server started on port " << port << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error creating server socket: " << e.what() << std::endl;
                serverSocket = nullptr;
                return false;
            }
        } else if (role == NetworkRole::CLIENT) {
            try {
                // Create client socket
                clientSocket = std::make_shared<Socket>(Socket::INET, Socket::STREAM);
                
                // Create client wrapper
                clientWrapper = std::make_unique<SocketWrapper>(clientSocket, true);
                
                // Store connection info for non-blocking connect
                connectionAddress = address;
                connectionPort = port;
                connectionState = ConnectionState::CONNECTING;
                
                // Log connection attempt
                std::cout << "Connecting to " << address << ":" << port << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error creating client socket: " << e.what() << std::endl;
                clientSocket = nullptr;
                return false;
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Network initialization error: " << e.what() << std::endl;
        return false;
    }
}

void NetworkManager::Shutdown() {
    LogNetworkState("Shutdown");
    
    // Reset socket pointers to properly clean up resources
    serverWrapper.reset();
    clientWrapper.reset();
    serverSocket.reset();
    clientSocket.reset();
    
    // Reset state variables
    role = NetworkRole::NONE;
    connected = false;
    connectionState = ConnectionState::NONE;
    
#ifdef _DEBUG
    std::cout << "NetworkManager::Shutdown completed" << std::endl;
#endif
}

void NetworkManager::Update(float deltaTime) {
    LogNetworkState("Update");
    // Update connection state for client
    if (role == NetworkRole::CLIENT) {
        if (connectionState == ConnectionState::CONNECTING) {
            // Update connection timer
            connectionTimer += deltaTime;
            
            // Try to connect (non-blocking)
            try {
                // Check if the clientWrapper is valid
                if (!clientWrapper || !clientWrapper->IsValid()) {
#ifdef _DEBUG
                    std::cerr << "Client socket is invalid, recreating..." << std::endl;
#endif
                    
                    // Create a new socket and wrapper
                    try {
                        clientSocket = std::make_shared<Socket>(Socket::INET, Socket::STREAM);
                        clientWrapper = std::make_unique<SocketWrapper>(clientSocket, true);
#ifdef _DEBUG
                        std::cout << "Recreated client socket" << std::endl;
#endif
                    } catch (const std::exception& e) {
                        std::cerr << "Failed to recreate client socket: " << e.what() << std::endl;
                        connectionState = ConnectionState::FAILED;
                        
                        // Notify callback if set
                        if (messageCallback) {
                            Packet packet;
                            messageCallback(MessageType::DISCONNECT, packet);
                        }
                        return;
                    }
                }
                
                if (clientWrapper && clientWrapper->TryConnect(Address(connectionAddress, connectionPort), 0)) {
                    // Connection successful
                    connectionState = ConnectionState::CONNECTED;
                    connected = true;
                    
                    // Reset stabilize timer - we'll send the CONNECT message after a short delay
                    stabilizeTimer = 0.0f;
                    
                    // Log successful connection
                    std::cout << "Connected to " << connectionAddress << ":" << connectionPort << std::endl;
                } else if (connectionTimer > connectionTimeout) {
                    // Connection timed out
                    connectionState = ConnectionState::FAILED;
                    std::cerr << "Connection timed out" << std::endl;
                    
                    // Notify callback if set
                    if (messageCallback) {
                        Packet packet;
                        messageCallback(MessageType::DISCONNECT, packet);
                    }
                }
            } catch (const std::exception& e) {
                std::string errorMsg = e.what();
                
                // Check if the error is "already connected"
                if (errorMsg.find("already connected") != std::string::npos) {
#ifdef _DEBUG
                    std::cout << "Socket is already connected, considering this a success" << std::endl;
#endif
                    connectionState = ConnectionState::CONNECTED;
                    connected = true;
                } else {
                    // If the error is connection refused, set the connection state to failed
                    if (errorMsg.find("connection refused") != std::string::npos || 
                        errorMsg.find("host is down") != std::string::npos ||
                        errorMsg.find("network is unreachable") != std::string::npos ||
                        errorMsg.find("operation in progress") != std::string::npos) {
                        std::cerr << "Connection refused or host not available" << std::endl;
                        connectionState = ConnectionState::FAILED;
                        
                        // Notify callback if set
                        if (messageCallback) {
                            Packet packet;
                            messageCallback(MessageType::DISCONNECT, packet);
                        }
                    } else {
                        std::cerr << "Error in Update (connect): " << errorMsg << std::endl;
                    }
                }
            }
        }
        else if (connectionState == ConnectionState::CONNECTED) {
            // We're already connected, check if the socket is still valid
            if (!clientWrapper || !clientWrapper->IsValid()) {
#ifdef _DEBUG
                std::cerr << "Client: Socket became invalid while connected, attempting to recreate..." << std::endl;
#endif
                
                // Create a new socket and wrapper
                try {
                    clientSocket = std::make_shared<Socket>(Socket::INET, Socket::STREAM);
                    clientWrapper = std::make_unique<SocketWrapper>(clientSocket, true);
                    
                    // Try to reconnect
                    if (clientWrapper->TryConnect(Address(connectionAddress, connectionPort), 0)) {
#ifdef _DEBUG
                        std::cout << "Client: Successfully reconnected" << std::endl;
#endif
                        // Reset stabilize timer
                        stabilizeTimer = 0.0f;
                    }
                } catch (const std::exception& e) {
#ifdef _DEBUG
                    std::cerr << "Client: Error recreating socket: " << e.what() << std::endl;
#endif
                    // Stay in CONNECTED state anyway
                }
            }
        }
    }
    
    // Check for new connections (host only)
    if (!connected && role == NetworkRole::HOST && serverWrapper && serverWrapper->IsValid()) {
        // Try to accept a connection (non-blocking)
        if (TryAcceptConnection()) {
            // Connection accepted, reset stabilize timer
            stabilizeTimer = 0.0f;
#ifdef _DEBUG
            std::cout << "Connection accepted, stabilizing for " << connectionStabilizeDelay << " seconds" << std::endl;
#endif
        }
    }
    
    // Check for incoming data (if connected)
    if (connected && clientWrapper && clientWrapper->IsValid()) {
        // Update stabilize timer
        if (stabilizeTimer < connectionStabilizeDelay) {
            stabilizeTimer += deltaTime;
            if (stabilizeTimer >= connectionStabilizeDelay) {
#ifdef _DEBUG
                std::cout << "Connection stabilized, starting to receive data" << std::endl;
#endif
                
                // If we're a client, send the CONNECT message now that the connection has stabilized
                if (role == NetworkRole::CLIENT) {
#ifdef _DEBUG
                    std::cout << "Client: Sending delayed CONNECT message" << std::endl;
#endif
                    Packet packet;
                    SendPacket(MessageType::CONNECT, packet);
                }
            }
        }
        
        // Only try to receive data after the stabilize delay
        if (stabilizeTimer >= connectionStabilizeDelay) {
            // Try to receive data (non-blocking)
            try {
                TryReceiveData();
            } catch (const std::exception& e) {
                // Log the error but don't disconnect
#ifdef _DEBUG
                std::cerr << "Error in Update (receive): " << e.what() << std::endl;
#endif
            }
        }
    }
}

bool NetworkManager::SendPacket(MessageType type, const Packet& packet) {
    LogNetworkState("SendPacket");
    // Basic validity check - but stay connected even if socket is invalid
    if (!connected) {
#ifdef _DEBUG
        std::cerr << "SendPacket: Not connected" << std::endl;
#endif
        return false;
    }
    
    if (!clientWrapper) {
#ifdef _DEBUG
        std::cerr << "SendPacket: Client wrapper is null" << std::endl;
#endif
        return false;
    }
    
    try {
        // Create header (message type + size)
        uint8_t messageType = static_cast<uint8_t>(type);
        uint32_t packetSize = static_cast<uint32_t>(packet.GetSize());
        
        // Create buffer for header + data
        ByteString buffer;
        buffer.resize(sizeof(messageType) + sizeof(packetSize) + packetSize);
        
        // Copy header and data to buffer
        std::memcpy(buffer.data(), &messageType, sizeof(messageType));
        std::memcpy(buffer.data() + sizeof(messageType), &packetSize, sizeof(packetSize));
        std::memcpy(buffer.data() + sizeof(messageType) + sizeof(packetSize), packet.GetData(), packetSize);
        
        // Send data using the wrapper (handles errors internally)
        return clientWrapper->TrySend(buffer);
    } catch (const std::exception& e) {
        // Handle any exceptions
#ifdef _DEBUG
        std::cerr << "Error in SendPacket: " << e.what() << std::endl;
#endif
        return false;
    }
}

bool NetworkManager::TryAcceptConnection() {
    LogNetworkState("TryAcceptConnection");
    
    if (!serverWrapper || !serverWrapper->IsValid()) {
        std::cerr << "TryAcceptConnection: serverWrapper is null or invalid" << std::endl;
        return false;
    }
    
    // If we already have a connection, don't accept a new one
    if (connected && clientSocket) {
#ifdef _DEBUG
        std::cout << "TryAcceptConnection: Already connected" << std::endl;
#endif
        return true;
    }
    
    try {
        // Create a temporary wrapper for the new client socket
        SocketWrapper tempClientWrapper(std::shared_ptr<Socket>(), true);
        
        // Try to accept a connection (non-blocking)
        if (serverWrapper->TryAccept(tempClientWrapper)) {
            // We got a connection
            
            // Store the new client socket and create a wrapper
            clientSocket = tempClientWrapper.GetSocket();
            
            if (!clientSocket || !clientSocket->_has_socket) {
                std::cerr << "TryAcceptConnection: Invalid socket received" << std::endl;
                return false;
            }
            
            clientWrapper = std::make_unique<SocketWrapper>(clientSocket, true);
            
            // Update connection state
            connected = true;
            connectionState = ConnectionState::CONNECTED;
            
            // Log connection success
            std::cout << "Client connected" << std::endl;
            return true;
        }
    } catch (const std::exception& e) {
        // Handle any exceptions
        std::cerr << "Error in TryAcceptConnection: " << e.what() << std::endl;
    }
    
    return false;
}

bool NetworkManager::TryReceiveData() {
    LogNetworkState("TryReceiveData");
    // Basic validity check - but stay connected even if socket is invalid
    if (!clientWrapper || !connected) {
        return false;
    }
    
    // Create a buffer for receiving data
    ByteString buffer(1024);
    bool connectionClosed = false;
    
    try {
        // Try to receive data (non-blocking)
        if (clientWrapper->TryReceive(buffer, &connectionClosed)) {
            // We got some data - check if it's valid
            if (buffer.size() > 0 && buffer.size() < 10000) { // Sanity check on size
                // Process the data
                ProcessReceivedData(buffer.data(), buffer.size());
                return true;
            } else {
#ifdef _DEBUG
                std::cerr << "TryReceiveData: Received invalid data size: " << buffer.size() << std::endl;
#endif
            }
        }
        
        // Even if the connection is closed, we'll stay "connected" in our state
        // This is to avoid disconnection issues
        if (connectionClosed) {
#ifdef _DEBUG
            std::cout << "Connection may have closed, but staying connected" << std::endl;
#endif
        }
    } catch (const std::exception& e) {
        // Handle any exceptions
        std::string errorMsg = e.what();
        
        // Don't log timeouts or would-block errors, they're normal for non-blocking sockets
        if (errorMsg.find("timed out") == std::string::npos && 
            errorMsg.find("would block") == std::string::npos) {
            std::cerr << "Error in TryReceiveData: " << errorMsg << std::endl;
        }
    }
    
    return false;
}

void NetworkManager::ProcessReceivedData(const char* data, size_t size) {
    LogNetworkState("ProcessReceivedData");
    if (size < sizeof(uint8_t) + sizeof(uint32_t)) {
        // Not enough data for header
#ifdef _DEBUG
        std::cout << "ProcessReceivedData: Not enough data for header, size: " << size << std::endl;
#endif
        return;
    }
    
    // Extract header
    uint8_t messageType;
    uint32_t packetSize;
    
    std::memcpy(&messageType, data, sizeof(messageType));
    std::memcpy(&packetSize, data + sizeof(messageType), sizeof(packetSize));
    
#ifdef _DEBUG
    std::cout << "ProcessReceivedData: Received message type: " << static_cast<int>(messageType) 
              << ", packet size: " << packetSize << std::endl;
#endif
    
    // Check if we have the complete packet
    if (size < sizeof(messageType) + sizeof(packetSize) + packetSize) {
        // Incomplete packet
#ifdef _DEBUG
        std::cout << "ProcessReceivedData: Incomplete packet" << std::endl;
#endif
        return;
    }
    
    // Extract packet data
    const char* packetData = data + sizeof(messageType) + sizeof(packetSize);
    
    // Create packet from data
    Packet packet;
    packet.Write(packetData, packetSize);
    
    // Call message callback if set
    if (messageCallback) {
        messageCallback(static_cast<MessageType>(messageType), packet);
    }
}
