cmake_minimum_required(VERSION 3.16)

project(WaveConv VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia Xml Concurrent)

add_executable(WaveConv
    MACOSX_BUNDLE 
    main.cpp
    mainwindow.cpp
    mainwindow.h
    synthengine.cpp
    synthengine.h
    ModularSynth.cpp
    ModularSynth.h
    pcmeditortab.cpp
    pcmeditortab.h
    oscilloscopetab.cpp
    oscilloscopetab.h
    drumsequencertab.cpp
    drumsequencertab.h
    melodyrenderertab.cpp
    melodyrenderertab.h
    effectstab.h
    effectstab.cpp
    vocalgenerator.h
    vocalgenerator.cpp
    samgenerator.cpp
    samgenerator.h
    drumeditortab.h
    drumeditortab.cpp
    naturelabtab.cpp
    naturelabtab.h
    SymbolicRegressionTab.h
    SymbolicRegressionTab.cpp
    MicroGP.h
    MicroGP.cpp
    vocaltransformtab.h
    vocaltransformtab.cpp
    xpressiveseqtab.cpp
    xpressiveseqtab.h
    wavetabletab.h
    wavetabletab.cpp
    spectralhologramtab.h
    spectralhologramtab.cpp

    libs/kiss_fft.c
    libs/kiss_fft.h
    libs/_kiss_fft_guts.h
    libs/kiss_fft_log.h
    libs/nanosvg.h

)

target_link_libraries(WaveConv PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::Xml Qt6::Concurrent)

target_include_directories(WaveConv PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/libs
)



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

if(Qt6_EXECUTABLE_PROPERTIES)
    set_target_properties(WaveConv PROPERTIES
        ${Qt6_EXECUTABLE_PROPERTIES}
    )
endif()
