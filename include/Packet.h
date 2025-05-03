#pragma once

#include <vector>
#include <cstring>
#include <string>

// Packet class for serializing and deserializing game data
class Packet {
public:
    Packet() : readPos(0), writePos(0) {}

    // Clear the packet
    void Clear() {
        buffer.clear();
        readPos = 0;
        writePos = 0;
    }

    // Write raw data
    void Write(const void* data, size_t size) {
        if (data && size > 0) {
            size_t start = buffer.size();
            buffer.resize(start + size);
            std::memcpy(buffer.data() + start, data, size);
            writePos = buffer.size();
        }
    }

    // Read raw data
    void Read(void* data, size_t size) {
        if (data && size > 0 && readPos + size <= buffer.size()) {
            std::memcpy(data, buffer.data() + readPos, size);
            readPos += size;
        }
    }

    // Template for writing basic types
    template<typename T>
    void Write(const T& data) {
        Write(&data, sizeof(T));
    }

    // Template for reading basic types
    template<typename T>
    void Read(T& data) {
        Read(&data, sizeof(T));
    }

    // Specialized write for strings
    void WriteString(const std::string& str) {
        size_t length = str.length();
        Write(length);
        if (length > 0) {
            Write(str.c_str(), length);
        }
    }

    // Specialized read for strings
    void ReadString(std::string& str) {
        size_t length = 0;
        Read(length);
        if (length > 0) {
            str.resize(length);
            Read(&str[0], length);
        } else {
            str.clear();
        }
    }

    // Get raw data pointer
    const char* GetData() const {
        return buffer.data();
    }

    // Get size of data
    size_t GetSize() const {
        return buffer.size();
    }

private:
    std::vector<char> buffer;
    size_t readPos;
    size_t writePos;
};
