cmake_minimum_required(VERSION 3.16)  # Повысим версию для лучшей поддержки Windows
project(TextAnalyzer VERSION 1.0.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Опции сборки
if(MSVC)
    # Для MSVC добавляем флаги для консольного/оконного приложения
    add_compile_options(/W3 /MP)
    
    # Устанавливаем уровень предупреждений
    if(CMAKE_BUILD_TYPE MATCHES "Release")
        add_compile_options(/O2 /MT)
    else()
        add_compile_options(/Od /MTd)
    endif()
endif()

# Исходные файлы
set(SOURCES
    gui.cpp
    src/front/InputPage.cpp
    src/front/ResultPage.cpp
    src/front/SearchPage.cpp
    src/front/MainWindow.cpp
    src/front/lib/FlowLayout.cpp
    src/front/lib/TextMarkupWidget.cpp
    src/back/statistics.cpp
    src/back/bert_onnx_inference.cpp
    src/back/onnx_model.cpp
    src/back/cJSON.c
    src/back/simple_tokenizer.cpp
    src/back/save_result.cpp
)

set(HEADERS
    src/front/InputPage.hpp
    src/front/ResultPage.hpp
    src/front/SearchPage.hpp
    src/front/MainWindow.hpp
    src/front/lib/FlowLayout.hpp
    src/front/lib/TextMarkupWidget.hpp
    src/back/statistics.hpp
    src/back/bert_onnx_inference.hpp
    src/back/onnx_model.hpp
    src/back/cJSON.h
    src/back/simple_tokenizer.hpp
    src/back/save_result.hpp
)

# Поиск Qt
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
if(WIN32)
    find_package(ONNXRUNTIME REQUIRED)
else()
    find_package(onnxruntime REQUIRED)
endif()

# Создание исполняемого файла
add_executable(TextAnalyzer ${SOURCES} ${HEADERS})
# WIN32 означает, что это GUI-приложение (не консольное)

# Подключение Qt библиотек
target_link_libraries(TextAnalyzer 
    Qt6::Core 
    Qt6::Widgets
    onnxruntime
)

# if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/resources/app.svg")
#     set(RESOURCE_FILE "${CMAKE_CURRENT_BINARY_DIR}/resources.rc")
#     file(WRITE ${RESOURCE_FILE}
#     "IDI_ICON1 ICON DISCARDABLE \"${CMAKE_SOURCE_DIR}/resources/app.ico\"
#     ")
#     target_sources(TextAnalyzer PRIVATE ${RESOURCE_FILE})
# endif()
# ----------------------------------

# Установка правил для сборки
install(TARGETS TextAnalyzer
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
    LIBRARY DESTINATION lib
)

# Опционально: копирование ресурсов
if(EXISTS "${CMAKE_SOURCE_DIR}/resources/")
    install(DIRECTORY resources/ DESTINATION resources)
endif()

# Вывод информации о сборке
message(STATUS "Building TextAnalyzer version ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Qt version: ${Qt6_VERSION}")
if(MSVC)
    message(STATUS "Compiler: MSVC ${MSVC_VERSION}")
endif()
