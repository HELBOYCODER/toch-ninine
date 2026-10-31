cmake_minimum_required(VERSION 3.22)

project(ShaderForge VERSION 2.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Warning policy applies to ShaderForge targets only, never to fetched third-party code.
add_library(sf_warnings INTERFACE)
if(MSVC)
    target_compile_options(sf_warnings INTERFACE /W4 /permissive- /WX)
else()
    target_compile_options(sf_warnings INTERFACE -Wall -Wextra -Werror)
endif()

# Portable core: no JUCE, no GL. Builds and tests on Windows, macOS and Linux.
add_library(sf_core STATIC
    src/shared/Adsr.cpp
    src/shared/Bus.cpp
    src/shared/BusPosix.cpp
    src/shared/Diag.cpp
    src/shared/Log.cpp
    src/shared/ParamRegistry.cpp
    src/core/ShaderPreprocessor.cpp
    src/core/ShaderSlot.cpp
    src/core/State.cpp
)
target_include_directories(sf_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(sf_core PRIVATE sf_warnings)

# Command-line tool: prints the injected final source for a shader file.
add_executable(shaderforge_inject src/tools/shaderforge_inject.cpp)
target_link_libraries(shaderforge_inject PRIVATE sf_core sf_warnings)

include(FetchContent)
find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.7.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(Catch2)
endif()

enable_testing()
add_subdirectory(tests)
