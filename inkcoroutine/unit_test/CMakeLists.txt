include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(CHECK REQUIRED)
include_directories(${CHECK_INCLUDE_DIR})
aux_source_directory(. TEST_SOURCE)

add_executable(inkcoroutine_check ${TEST_SOURCE})
target_link_libraries(inkcoroutine_check inkcoroutine ${CHECK_LIBRARIES})
