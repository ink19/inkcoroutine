cmake_minimum_required(VERSION 3.22)
project(inkcoroutine VERSION 0.0.1)

find_package(Threads REQUIRED)

include_directories(.)
aux_source_directory(. SOURCE_FILE)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_library(
  inkcoroutine SHARED
  ${SOURCE_FILE}
)

target_link_libraries(
  inkcoroutine
  Threads::Threads
)

set_target_properties(inkcoroutine PROPERTIES VERSION ${PROJECT_VERSION})

add_subdirectory(unit_test)

enable_testing()
add_test(NAME inkcoroutine_check COMMAND inkcoroutine_check)
