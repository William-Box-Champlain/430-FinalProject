#pragma once
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include "socklib.h"

typedef std::vector<char> ByteString;

// Custom error classes
class SocketError : public std::runtime_error {
public:
    SocketError(const std::string& msg) : std::runtime_error(msg) {}
};

class ConnectionHungUpError : public SocketError {
public:
    ConnectionHungUpError() : SocketError("Connection closed by peer") {}
};

class TimeoutError : public SocketError {
public:
    TimeoutError() : SocketError("Operation timed out") {}
};

// Wrapper for Socket to add non-blocking functionality
class SocketWrapper {
private:
    std::shared_ptr<Socket> socket;
    
public:
    // Create a new socket and own it
    SocketWrapper(std::shared_ptr<Socket> socket);
    
    // Create a new socket with specified parameters
    SocketWrapper(Socket::Family family, Socket::Type type, bool setNonBlocking = true);
    
    // Wrap an existing socket
    SocketWrapper(std::shared_ptr<Socket> existingSocket, bool setNonBlocking = true);
    
    // Move constructor and assignment
    SocketWrapper(SocketWrapper&& other);
    SocketWrapper& operator=(SocketWrapper&& other);
    
    // Copy constructor and assignment (now allowed with shared_ptr)
    SocketWrapper(const SocketWrapper& other);
    SocketWrapper& operator=(const SocketWrapper& other);
    
    // Destructor
    ~SocketWrapper();
    
    // Non-blocking connect attempt
    bool TryConnect(const Address& address, int port);
    
    // Non-blocking accept
    bool TryAccept(SocketWrapper& clientSocket);
    
    // Non-blocking receive
    bool TryReceive(ByteString& buffer, bool* connectionClosed = nullptr);
    
    // Send data (still blocking, but with better error handling)
    bool TrySend(const ByteString& data);
    
    // Get the underlying socket
    std::shared_ptr<Socket> GetSocket() const { return socket; }
    
    // Check if socket is valid
    bool IsValid() const;
};
