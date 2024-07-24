cmake_minimum_required(VERSION 3.15)
project(webnote)

find_package(Crow REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} Crow::Crow)
