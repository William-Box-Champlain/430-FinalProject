cmake_minimum_required(VERSION 3.10)

# Project name
project(GPR-430-Final-Project VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# First try to use the local raylib installation
set(RAYLIB_LOCAL_DIR "C:/raylib")
if(EXISTS "${RAYLIB_LOCAL_DIR}/raylib/src/raylib.h")
    message(STATUS "Using local raylib installation at ${RAYLIB_LOCAL_DIR}")
    include_directories(${RAYLIB_LOCAL_DIR}/raylib/src)
    link_directories(${RAYLIB_LOCAL_DIR}/raylib/src)
    set(RAYLIB_FOUND TRUE)
else()
    message(STATUS "Local raylib installation not found, attempting to download...")
    # Include FetchContent for downloading dependencies
    include(FetchContent)

    # Download and configure raylib
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 4.5.0  # Specify a version/tag
    )
    # Configure raylib build options
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
    
    # Try to make raylib available
    FetchContent_MakeAvailable(raylib)
    
    # Check if raylib was successfully made available
    if(TARGET raylib)
        set(RAYLIB_FOUND TRUE)
    else()
        message(FATAL_ERROR "Failed to download or build raylib. Please ensure raylib is installed at ${RAYLIB_LOCAL_DIR} or free up disk space for downloading.")
    endif()
endif()

# Add include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Find all source files but exclude platform-specific ones we don't need
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Platform-specific source file handling
if(WIN32)
    # Exclude POSIX-specific source files on Windows
    list(FILTER SOURCES EXCLUDE REGEX ".*socklib_posix\\.cpp$")
elseif(UNIX)
    # Exclude Windows-specific source files on Unix
    list(FILTER SOURCES EXCLUDE REGEX ".*socklib_win32\\.cpp$")
endif()

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Platform-specific settings
if(WIN32)
    # Windows-specific settings if needed
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_WINDOWS)
    
    # Add Windows-specific libraries if needed
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 raylib)
elseif(UNIX AND NOT APPLE)
    # Linux-specific settings
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_LINUX)
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib m pthread dl)
elseif(APPLE)
    # macOS-specific settings
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_MACOS)
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
endif()

# Optional: Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Optional: Copy any needed resources to the output directory
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Optional: Set compiler flags
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()
