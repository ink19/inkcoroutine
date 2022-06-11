cmake_minimum_required(VERSION 3.22)
project(InkCoroutine LANGUAGES C)

set(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

aux_source_directory(src MAIN_SOURCE)
aux_source_directory(./3rdparty/list/src/ MAIN_SOURCE)

include_directories(./src)
include_directories(./3rdparty)
include_directories(./3rdparty/list/src/)

add_executable(${PROJECT_NAME} main.c ${MAIN_SOURCE})

target_link_libraries(
  ${PROJECT_NAME}
  Threads::Threads
)
