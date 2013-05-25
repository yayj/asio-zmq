project(asio-zmq-examples)
cmake_minimum_required(VERSION 2.8)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "-Wall -std=c++11 -stdlib=libc++")
else ()
  set(CMAKE_CXX_FLAGS "-Wall -std=c++11")
endif ()

add_definitions(-DASIO_HAS_STD_CHRONO)

find_library(ZMQ_LIBRARY zmq REQUIRED)

file(GLOB example_SRCS "${CMAKE_SOURCE_DIR}/*.cpp")

include_directories(${CMAKE_SOURCE_DIR}/../../include)

foreach(SRC ${example_SRCS})
  get_filename_component(EXE ${SRC} NAME_WE)
  add_executable(${EXE} ${SRC})
  target_link_libraries(${EXE} ${ZMQ_LIBRARY})
endforeach()
