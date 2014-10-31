project(asio-zmq-performance)
cmake_minimum_required(VERSION 2.8)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "-Wall -O2 -std=c++11 -stdlib=libc++")
else ()
  set(CMAKE_CXX_FLAGS "-Wall -O2 -std=c++11")
endif ()

add_definitions(-DBOOST_ASIO_HAS_STD_CHRONO)

find_package(Boost REQUIRED COMPONENTS system)
find_library(ZMQ_LIBRARY zmq REQUIRED)

file(GLOB perf_SRCS "${CMAKE_SOURCE_DIR}/*.cpp")

include_directories(
    ${CMAKE_SOURCE_DIR}/../../include
    ${Boost_INCLUDE_DIRS}
    )

foreach(SRC ${perf_SRCS})
  get_filename_component(EXE ${SRC} NAME_WE)
  add_executable(${EXE} ${SRC})
  target_link_libraries(${EXE} ${ZMQ_LIBRARY} ${Boost_LIBRARIES})
endforeach()
