cmake_minimum_required(VERSION 3.22)
project(InkCoroutine LANGUAGES C)

set(CMAKE_BUILD_TYPE "Debug")

add_subdirectory(inkcoroutine)

add_executable(${PROJECT_NAME} main.c)

target_link_libraries(
  ${PROJECT_NAME}
  inkcoroutine
)