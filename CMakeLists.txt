cmake_minimum_required(VERSION 3.16)

# The project name here must match the names used in your YAML deployment steps
project(WaveConv VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Find the necessary Qt6 packages
# Multimedia is required for your AudioSynth class 
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)

# 2. Define the executable and its source files
# Using MACOSX_BUNDLE ensures it creates a .app folder instead of a raw binary on Mac
add_executable(WaveConv
    MACOSX_BUNDLE 
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

# 3. Link the libraries to your project
target_link_libraries(WaveConv PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
)

# 4. Platform Specific Configurations
if(APPLE)
    # This sets the name that appears in the macOS Finder and Menu Bar
    set_target_properties(WaveConv PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourdomain.WaveConv"
        MACOSX_BUNDLE_BUNDLE_NAME "WaveConv"
    )
elseif(WIN32)
    # This prevents a console window from popping up behind your app on Windows
    set_target_properties(WaveConv PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# 5. Deployment settings
if(Qt6_EXECUTABLE_PROPERTIES)
    set_target_properties(WaveConv PROPERTIES
        ${Qt6_EXECUTABLE_PROPERTIES}
    )
endif()
