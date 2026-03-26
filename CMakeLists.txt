cmake_minimum_required(VERSION 3.10)
project(TextAnalyzer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

set(SOURCES
    gui.cpp
    src/front/InputPage.cpp
    src/front/ResultPage.cpp
    src/front/SearchPage.cpp
    src/front/MainWindow.cpp
    src/front/lib/FlowLayout.cpp
    src/front/lib/TextMarkupWidget.cpp
)

set(HEADERS
    src/front/InputPage.hpp
    src/front/ResultPage.hpp
    src/front/SearchPage.hpp
    src/front/MainWindow.hpp
    src/front/lib/FlowLayout.hpp
    src/front/lib/TextMarkupWidget.hpp
)

add_executable(gui ${SOURCES} ${HEADERS})

target_link_libraries(gui Qt6::Core Qt6::Widgets)
