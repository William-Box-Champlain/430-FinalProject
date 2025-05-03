cmake_minimum_required(VERSION 3.10)

# Set policy version minimum to avoid raylib CMake errors
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# Project name
project(GPR-430-Final-Project VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try to find raylib in multiple common locations
set(RAYLIB_FOUND FALSE)

# Check common installation paths
set(RAYLIB_SEARCH_PATHS
    "C:/raylib"
    "C:/Program Files/raylib"
    "C:/Program Files (x86)/raylib"
    "$ENV{USERPROFILE}/raylib"
    "$ENV{USERPROFILE}/Documents/raylib"
    "$ENV{USERPROFILE}/Desktop/raylib"
)

# Try to find raylib in the search paths
foreach(SEARCH_PATH ${RAYLIB_SEARCH_PATHS})
    if(EXISTS "${SEARCH_PATH}/raylib/src/raylib.h")
        message(STATUS "Found raylib installation at ${SEARCH_PATH}")
        include_directories(${SEARCH_PATH}/raylib/src)
        link_directories(${SEARCH_PATH}/raylib/src)
        set(RAYLIB_FOUND TRUE)
        break()
    endif()
endforeach()

# If not found locally, download and build it
if(NOT RAYLIB_FOUND)
    message(STATUS "Local raylib installation not found, attempting to download...")
    # Include FetchContent for downloading dependencies
    include(FetchContent)

    # Download and configure raylib
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 4.5.0  # Specify a version/tag
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy 
            ${CMAKE_CURRENT_SOURCE_DIR}/CMakePresets.json
            <SOURCE_DIR>/CMakePresets.json
    )
    # Configure raylib build options
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
    
    # Try to make raylib available
    FetchContent_MakeAvailable(raylib)
    
# Check if raylib was successfully made available
    if(TARGET raylib)
        set(RAYLIB_FOUND TRUE)
        message(STATUS "Successfully downloaded and configured raylib")
    else()
        message(FATAL_ERROR "Failed to download or build raylib. Please install raylib manually or check your internet connection.")
    endif()
endif()

# Verify raylib was found one way or another
if(NOT RAYLIB_FOUND)
    message(FATAL_ERROR "Could not find or download raylib. Please install raylib manually and try again.")
endif()

# Add include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Find all source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Platform-specific source file handling
if(WIN32)
    # Exclude POSIX-specific source files on Windows
    list(FILTER SOURCES EXCLUDE REGEX ".*socklib_posix\\.cpp$")
    list(FILTER SOURCES EXCLUDE REGEX ".*_posix\\.cpp$")
    message(STATUS "Building for Windows platform")
elseif(UNIX)
    # Exclude Windows-specific source files on Unix
    list(FILTER SOURCES EXCLUDE REGEX ".*socklib_win32\\.cpp$")
    list(FILTER SOURCES EXCLUDE REGEX ".*_win32\\.cpp$")
    message(STATUS "Building for Unix platform")
endif()

# Print the source files being used
message(STATUS "Source files: ${SOURCES}")

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Add a custom target to clean the build directory
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMENT "Cleaning all build files and directories"
)

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

# Set output directory and ensure it exists
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Add post-build command to copy the executable to the bin directory
# This ensures the executable is always in the expected location
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
        $<TARGET_FILE:${PROJECT_NAME}> 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<TARGET_FILE_NAME:${PROJECT_NAME}>
    COMMENT "Copying executable to bin directory"
)

# Optional: Copy any needed resources to the output directory
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Optional: Set compiler flags
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()
