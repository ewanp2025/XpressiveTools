cmake_minimum_required(VERSION 3.16)

project(WaveConv VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Find the necessary Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)

# 2. Define the executable and its source files
add_executable(WaveConv
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

# 3. Link the libraries to your project
target_link_libraries(WaveConv PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
)

# 4. Deployment settings (helps for cross-platform bundling)
if(Qt6_EXECUTABLE_PROPERTIES)
    set_target_properties(WaveConv PROPERTIES
        ${Qt6_EXECUTABLE_PROPERTIES}
    )
endif()
