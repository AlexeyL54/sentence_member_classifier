cmake_minimum_required(VERSION 3.14)
project(Tests)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(GTest REQUIRED)
# find_package(Threads REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test)

set(PROJECT_SOURCES
  ../src/back/text_splitter.cpp
  ../src/back/simple_tokenizer.cpp
  ../src/back/unistring.cpp
  ../src/back/statistics.cpp
)

set(PROJECT_HEADERS 
  ../src/back/text_splitter.hpp
  ../src/back/simple_tokenizer.hpp
  ../src/back/unistring.hpp
  ../src/back/statistics.hpp
)

function(add_spellchecker_test TEST_NAME TEST_FILE)
  add_executable(${TEST_NAME} 
    ${TEST_FILE}
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
  )
  
  target_link_libraries(${TEST_NAME}
    GTest::gtest
    GTest::gtest_main
    # Threads::Threads
    Qt6::Core 
    # Qt6::Widgets
    # Qt6::Test
  )
  
  target_include_directories(${TEST_NAME} PRIVATE ${GTEST_INCLUDE_DIRS})
  
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

add_spellchecker_test(tokenizer_test test_tokenizer.cpp)
add_spellchecker_test(statistics_test test_statistics.cpp)

enable_testing()
