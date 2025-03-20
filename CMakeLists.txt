cmake_minimum_required(VERSION 3.22)
project(TMS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_TRAVEL_TRACKING "Enable tracking of individual travel data and generating travel reports" OFF)

if(ENABLE_TRAVEL_TRACKING)
  message(STATUS "Travel tracking feature is enabled")
  # Add definitions or configurations specific to the travel tracking feature
  add_definitions(-DENABLE_TRAVEL_TRACKING)
endif()

if(WIN32)
  set(CMAKE_CXX_FLAGS_RELEASE "-DNOMINMAX ${CMAKE_CXX_FLAGS_RELEASE} /MT")
  set(CMAKE_CXX_FLAGS_DEBUG "-DNOMINMAX ${CMAKE_CXX_FLAGS_DEBUG} /MTd")

  # Sets the flag to disable Linux specific error handling
  add_definitions(-D__DISABLE_CRIT_ERR)
endif()

add_compile_options(-Wunused-variable)

# Include external projects
set(EXT_PROJECTS_DIR ${PROJECT_SOURCE_DIR}/ext)

# Add the source subdirectory
add_subdirectory(src)

# Setup test project
option(BUILD_TESTS "Build all tests." OFF) # Makes boolean 'test' available.
if(BUILD_TESTS)
  add_subdirectory(test)
endif()
