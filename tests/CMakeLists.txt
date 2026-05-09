cmake_minimum_required(VERSION 3.14)
project(Tests)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Находим Google Test
find_package(GTest REQUIRED)

# Добавляем исполняемый файл для тестов
add_executable(test
  back/test_tokenizer.cpp
  ../src/back/text_splitter.cpp
  ../src/back/simple_tokenizer.cpp
  ../src/back/unistring.cpp
)

# Линкуем с Google Test
target_link_libraries(test
    GTest::GTest
    GTest::Main
)

# Включаем тесты
include(GoogleTest)
