cmake_minimum_required(VERSION 3.16)
project(AudioEditor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Compiler warnings for better code quality
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# Find Qt
set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt@6" ${CMAKE_PREFIX_PATH})
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia Concurrent)

# Find mpg123
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPG123 REQUIRED libmpg123)

# Find LAME
find_library(LAME_LIBRARY NAMES mp3lame lame)
find_path(LAME_INCLUDE_DIR lame/lame.h)

# Source files
set(CORE_SOURCES
    Core/AudioClip.cpp
    Core/EffectFactory.cpp
    Core/Logging/FileLogger.cpp
    Core/Logging/ConsoleLogger.cpp
    Core/Logging/CompositeLogger.cpp
    Core/Adapters/Mp3.cpp
    Core/Effects/Reverb.cpp
    Core/Effects/Speed.cpp
    Core/Effects/Volume.cpp
    Core/Effects/Normalize.cpp
    Core/Commands/CommandHistory.cpp
    Core/Commands/ApplyEffect.cpp
    Core/Commands/EffectStateCommand.cpp  # NEW - moved from header-only
    Core/Services/CaptionParser.cpp
)

set(GUI_SOURCES
    GUI/MainWindow.cpp
    GUI/AudioEngine.cpp
    GUI/TransportBar.cpp
    GUI/WaveformWidget.cpp
    GUI/EffectsPanel.cpp
    GUI/EffectWidget.cpp
    GUI/CaptionPanel.cpp
)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Core
    ${CMAKE_SOURCE_DIR}/Core/Adapters
    ${CMAKE_SOURCE_DIR}/Core/Effects
    ${CMAKE_SOURCE_DIR}/Core/Commands
    ${CMAKE_SOURCE_DIR}/Core/Logging
    ${CMAKE_SOURCE_DIR}/Core/Services
    ${CMAKE_SOURCE_DIR}/GUI
    ${MPG123_INCLUDE_DIRS}
    ${LAME_INCLUDE_DIR}
)

# Executable
add_executable(AudioEditor 
    main.cpp
    ${CORE_SOURCES}
    ${GUI_SOURCES}
)

# Link libraries
target_link_libraries(AudioEditor
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Concurrent
    ${MPG123_LIBRARIES}
    ${LAME_LIBRARY}
)

# Compile options
target_compile_options(AudioEditor PRIVATE ${MPG123_CFLAGS_OTHER})
target_link_directories(AudioEditor PRIVATE ${MPG123_LIBRARY_DIRS})

# macOS bundle
if(APPLE)
    set_target_properties(AudioEditor PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.audioeditor.app"
        MACOSX_BUNDLE_BUNDLE_NAME "Audio Editor"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    )
endif()
