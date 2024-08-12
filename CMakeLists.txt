cmake_minimum_required(VERSION 3.15)
project(webnote CXX)

find_package(Crow CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)
file(GLOB SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE Crow::Crow libpqxx::pqxx)
