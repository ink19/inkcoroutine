cmake_minimum_required(VERSION 3.22)
project(InkCoroutine LANGUAGES C)

set(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(inkcoroutine)

add_executable(${PROJECT_NAME} main.c)

target_link_libraries(
  ${PROJECT_NAME}
  inkcoroutine
)