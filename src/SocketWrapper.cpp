#include "SocketWrapper.h"
#include "socklib.h"
#include <iostream>
#include <chrono>
#include <memory>

SocketWrapper::SocketWrapper(std::shared_ptr<Socket> socket)
    : socket(socket) {
    if (socket && socket->_has_socket) {
        try {
            socket->SetNonBlockingMode(true);
            
            socket->SetTimeout(0.05f);
            
            std::cout << "SocketWrapper: Set socket to non-blocking mode with timeout" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "SocketWrapper: Error setting socket options: " << e.what() << std::endl;
        }
    }
}

SocketWrapper::SocketWrapper(Socket::Family family, Socket::Type type, bool setNonBlocking)
    : socket(std::make_shared<Socket>(family, type)) {
    if (socket && socket->_has_socket && setNonBlocking) {
        try {
            socket->SetNonBlockingMode(true);
            
            socket->SetTimeout(0.05f);
            
            std::cout << "SocketWrapper: Set socket to non-blocking mode with timeout" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "SocketWrapper: Error setting socket options: " << e.what() << std::endl;
        }
    }
}

SocketWrapper::SocketWrapper(std::shared_ptr<Socket> existingSocket, bool setNonBlocking)
    : socket(existingSocket) {
    if (socket && socket->_has_socket && setNonBlocking) {
        try {
            socket->SetNonBlockingMode(true);
            
            socket->SetTimeout(0.05f);
            
            std::cout << "SocketWrapper: Set socket to non-blocking mode with timeout" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "SocketWrapper: Error setting socket options: " << e.what() << std::endl;
        }
    }
}

SocketWrapper::SocketWrapper(SocketWrapper&& other)
    : socket(std::move(other.socket)) {
}

SocketWrapper& SocketWrapper::operator=(SocketWrapper&& other) {
    if (this != &other) {
        socket = std::move(other.socket);
    }
    return *this;
}

SocketWrapper::SocketWrapper(const SocketWrapper& other)
    : socket(other.socket) {
}

SocketWrapper& SocketWrapper::operator=(const SocketWrapper& other) {
    if (this != &other) {
        socket = other.socket;
    }
    return *this;
}

SocketWrapper::~SocketWrapper() {
}

bool SocketWrapper::TryConnect(const Address& address, int port) {
    if (!socket || !socket->_has_socket) {
        std::cerr << "TryConnect: Socket is invalid" << std::endl;
        return false;
    }
    
    try {
        std::cout << "TryConnect: Attempting to connect..." << std::endl;
        
        int result = socket->Connect(address);
        
        std::cout << "TryConnect: Connect returned " << result << std::endl;
        
        if (result == 0) {
            return true;
        }
        
        if (socket->_last_error == Socket::SOCKLIB_ETIMEDOUT) {
            return false;
        }
        
        std::cerr << "TryConnect: Connection failed with error code " << socket->_last_error << std::endl;
        throw std::runtime_error("Connection failed");
    } catch (const std::exception& e) {
        std::string errorMsg = e.what();
        
        if (errorMsg.find("already connected") != std::string::npos) {
            std::cout << "TryConnect: Socket is already connected" << std::endl;
            return true;  
        }
        
        if (errorMsg.find("connection refused") != std::string::npos ||
            errorMsg.find("host is down") != std::string::npos ||
            errorMsg.find("network is unreachable") != std::string::npos ||
            errorMsg.find("Connection failed") != std::string::npos ||
            errorMsg.find("operation in progress") != std::string::npos) {
            std::cerr << "TryConnect: Fatal connection error: " << errorMsg << std::endl;
            
            if (errorMsg.find("operation in progress") != std::string::npos) {
                std::cout << "TryConnect: Operation already in progress, need to recreate socket" << std::endl;
                
                if (socket) {
                    Socket::Family family = Socket::INET;
                    Socket::Type type = Socket::STREAM;
                    
                    try {
                        socket = std::make_shared<Socket>(family, type);
                        socket->SetNonBlockingMode(true);
                        socket->SetTimeout(0.05f);
                    } catch (const std::exception& e) {
                        std::cerr << "TryConnect: Failed to recreate socket: " << e.what() << std::endl;
                        socket = nullptr;
                    }
                }
            }
            
            throw; 
        }
        
        std::cout << "TryConnect: Exception caught (likely timeout): " << errorMsg << std::endl;
        return false;
    }
}

bool SocketWrapper::TryAccept(SocketWrapper& clientSocket) {
    if (!socket || !socket->_has_socket) {
        std::cerr << "TryAccept: Socket is invalid" << std::endl;
        return false;
    }
    
    try {
        std::cout << "TryAccept: Attempting to accept connection..." << std::endl;
        
        Socket newClient = socket->Accept();
        
        if (newClient._has_socket) {
            std::cout << "TryAccept: Got valid socket" << std::endl;
            
            auto newSocket = std::make_shared<Socket>();
            *newSocket = std::move(newClient);
            
            if (!newSocket->_has_socket) {
                std::cerr << "TryAccept: Socket move failed, socket is invalid" << std::endl;
                return false;
            }
            
            try {
                newSocket->SetNonBlockingMode(true);
                newSocket->SetTimeout(0.05f);
                std::cout << "TryAccept: Set non-blocking mode on new socket" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "TryAccept: Error setting socket options: " << e.what() << std::endl;
            }
            
            clientSocket.socket = newSocket;
            return true;
        }
        
        std::cout << "TryAccept: No valid socket returned" << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "TryAccept: Exception caught: " << e.what() << std::endl;
        return false;
    }
}

bool SocketWrapper::TryReceive(ByteString& buffer, bool* connectionClosed) {
    if (connectionClosed) *connectionClosed = false;
    
    if (!socket || !socket->_has_socket) {
        std::cerr << "TryReceive: Socket is invalid" << std::endl;
        if (connectionClosed) *connectionClosed = true;
        return false;
    }
    
    try {
        if (buffer.size() < 1024) {
            buffer.resize(1024);
        }
        
        int bytesReceived = socket->Recv(buffer.data(), buffer.size());
        
        if (bytesReceived > 0) {
            buffer.resize(bytesReceived);
            return true;
        } else if (bytesReceived == 0) {
            if (connectionClosed) *connectionClosed = true;
            return false;
        } else {
            return false;
        }
    } catch (const std::exception& e) {
        std::string errorMsg = e.what();
        
        if (errorMsg.find("not a socket") != std::string::npos ||
            errorMsg.find("reset by peer") != std::string::npos ||
            errorMsg.find("connection abort") != std::string::npos ||
            errorMsg.find("forcibly closed") != std::string::npos) {
            if (connectionClosed) *connectionClosed = true;
        } else if (errorMsg.find("timed out") != std::string::npos ||
                  errorMsg.find("would block") != std::string::npos) {
            return false;
        } else {
            std::cerr << "TryReceive: Exception: " << errorMsg << std::endl;
        }
        return false;
    }
}

bool SocketWrapper::TrySend(const ByteString& data) {
    if (!socket || !socket->_has_socket) {
        std::cerr << "TrySend: Socket is invalid" << std::endl;
        return false;
    }
    
    try {
        std::cout << "TrySend: Attempting to send " << data.size() << " bytes..." << std::endl;
        
        const size_t MAX_CHUNK_SIZE = 1024; 
        size_t totalSent = 0;
        
        while (totalSent < data.size()) {
            size_t chunkSize = std::min(MAX_CHUNK_SIZE, data.size() - totalSent);
            
            ByteString chunk(data.begin() + totalSent, data.begin() + totalSent + chunkSize);
            
            size_t sent = socket->Send(chunk.data(), chunk.size());
            
            if (sent == 0) {
                std::cerr << "TrySend: Failed to send chunk, socket would block" << std::endl;
                break;
            }
            
            // Update total sent
            totalSent += sent;
        }
        
        if (totalSent == data.size()) {
            std::cout << "TrySend: Data sent successfully" << std::endl;
            return true;
        } else {
            std::cerr << "TrySend: Only sent " << totalSent << " of " << data.size() << " bytes" << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::string errorMsg = e.what();
        
        if (errorMsg.find("not a socket") != std::string::npos ||
            errorMsg.find("reset by peer") != std::string::npos ||
            errorMsg.find("connection abort") != std::string::npos ||
            errorMsg.find("broken pipe") != std::string::npos ||
            errorMsg.find("forcibly closed") != std::string::npos) {
            std::cout << "TrySend: Connection closed: " << errorMsg << std::endl;
        } else {
            std::cout << "TrySend: Exception caught: " << errorMsg << std::endl;
        }
        return false;
    }
}

bool SocketWrapper::IsValid() const {
    return socket && socket->_has_socket;
}
