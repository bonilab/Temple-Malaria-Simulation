# NOTE: when compile with old gcc (<= 8.0), remember to link stdc++fs

# Enable for compiler warnings on core simulation files
set(BUILD_WARNINGS false CACHE BOOL "Enable for build warnings")

# Include dependent libs, note that this might be extended if we are on the cluster
find_package(GSL REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(date CONFIG REQUIRED)
find_package(args CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/src)

# Define the core simulation files
file(GLOB SOURCE *.cpp
        Debug/*.cpp
        Core/*.cpp
        Core/Config/*.cpp
        Core/Config/Decoders/*.cpp
        Core/Index/*.cpp
        Environment/*.cpp
        Events/*.cpp
        Events/Environment/*.cpp
        Events/Population/*.cpp
        Events/Population/EventBuilders/*.cpp
        GIS/*.cpp
        Helpers/*.cpp
        MDC/*.cpp
        Parasites/*.cpp
        Population/*.cpp
        Population/ImmuneComponent/*.cpp
        Population/ParasiteDensity/*.cpp
        Population/Properties/*.cpp
        Reporters/*.cpp
        Reporters/Specialist/*.cpp
        Reporters/Utility/*.cpp
        Spatial/*.cpp
        Strategies/*.cpp
        Treatment/*.cpp
        Therapies/*.cpp
        Utility/*.cpp
        Validation/*.cpp
)


if(NOT ENABLE_TRAVEL_TRACKING)
  # remve the travel tracking files
  list(REMOVE_ITEM SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/Reporters/TravelTrackingReporter.cpp")
endif()

# Set the source files
# set(SRC_FILES ${SOURCE} ${LIBRARY})
add_library(MaSimCore STATIC ${SOURCE})
target_compile_features(MaSimCore PRIVATE cxx_std_17)

find_library(EASYLOGGINGPP_LIB easyloggingpp)

target_link_libraries(MaSimCore PUBLIC
        yaml-cpp::yaml-cpp
        GSL::gsl GSL::gslcblas
        fmt::fmt-header-only
        unofficial::sqlite3::sqlite3
        CLI11::CLI11
        date::date
        date::date-tz
        taywee::args
        ${EASYLOGGINGPP_LIB}
)

# Enable warnings when building debug
# -Wunused-parameter from -Wextra is disabled since the YAML parser will emit the warning
# -Wunused-variable from -Wall is disabled since easylogging++ frequently emits it
if(BUILD_WARNINGS)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable")
endif()

add_executable(MaSim MaSim/main.cpp)
target_compile_features(MaSim PRIVATE cxx_std_17)
add_dependencies(MaSim MaSimCore)
target_link_libraries(MaSim PRIVATE MaSimCore)

install(TARGETS MaSim DESTINATION ${PROJECT_SOURCE_DIR}/bin)
install(FILES ${PROJECT_SOURCE_DIR}/misc/input_dev.yml DESTINATION ${PROJECT_SOURCE_DIR}/bin)

file(GLOB DXG_SRC_FILES DxGGenerator/*.cpp DxGGenerator/*.h)

add_executable(DxGGenerator ${DXG_SRC_FILES})
target_compile_features(DxGGenerator PRIVATE cxx_std_17)
add_dependencies(DxGGenerator MaSimCore)
target_link_libraries(DxGGenerator PRIVATE MaSimCore)

# Note the flags that we ran with
message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})
message(STATUS "Library Type: " ${LIB_TYPE})
message(STATUS "Operating System: " ${CMAKE_SYSTEM_NAME})
message(STATUS "Compiler flags:" ${CMAKE_CXX_COMPILE_FLAGS})
message(STATUS "Compiler cxx debug flags:" ${CMAKE_CXX_FLAGS_DEBUG})
message(STATUS "Compiler cxx release flags:" ${CMAKE_CXX_FLAGS_RELEASE})
message(STATUS "Compiler cxx min size flags:" ${CMAKE_CXX_FLAGS_MINSIZEREL})
message(STATUS "Compiler cxx flags:" ${CMAKE_CXX_FLAGS})
