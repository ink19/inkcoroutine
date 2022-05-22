cmake_minimum_required(VERSION 3.22)
project(inkcoroutine VERSION 0.0.1)

find_package(pthread REQUIRED)

add_library(
  inkcoroutine SHARED
  inkcoroutine.h
  inkcoroutine.c
)

target_link_libraries(
  inkcoroutine
  pthread::pthread
)

set_target_properties(inkcoroutine PROPERTIES VERSION ${PROJECT_VERSION})
