cmake_minimum_required(VERSION 3.10)
project(szsTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)

add_executable(szsTest
    src/main.cpp
    src/szsDecode.cpp
    src/sarcReader.cpp
)

# Für ältere GCC-Versionen filesystem-Library linken
target_link_libraries(szsTest stdc++fs)