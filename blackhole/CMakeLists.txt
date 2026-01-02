cmake_minimum_required(VERSION 3.20)
project(blackhole LANGUAGES C CXX)

# --- Basics ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # clangd uses this

# --- GLFW (Homebrew) ---
# Homebrew on Apple Silicon installs to /opt/homebrew
# This helps CMake find glfw3Config.cmake if it's not already found
if(APPLE AND EXISTS "/opt/homebrew")
  list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
endif()

find_package(glfw3 CONFIG REQUIRED)

# --- Your executable ---
add_executable(app
  src/main.cpp
  src/glad.c
)

# GLAD headers live in ./include
target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# GLFW include dirs come from the imported target, but doesn't hurt to be explicit:
target_link_libraries(app PRIVATE glfw)

# macOS frameworks (usually already handled by glfw target, but this is safe)
if(APPLE)
  target_link_libraries(app PRIVATE
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
  )
endif()

