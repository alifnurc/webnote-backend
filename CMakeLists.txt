cmake_minimum_required(VERSION 3.15)
project(webnote CXX)

find_package(Crow CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB SOURCES "src/*.cpp")
find_path(JWT_CPP_INCLUDE_DIRS "jwt-cpp/base.h")

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE include ${JWT_CPP_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE Crow::Crow libpqxx::pqxx)
