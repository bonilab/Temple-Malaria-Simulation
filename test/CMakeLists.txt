set(PROJECT_TEST_NAME masim_test)

include_directories(${PROJECT_SOURCE_DIR}/src)

find_package(Catch2 CONFIG REQUIRED)

# Group GIS test files
set(GIS_TEST_FILES
    GIS/SpatialDataTest/BasicOperationsTest.cpp
    GIS/SpatialDataTest/DistrictOperationsTest.cpp
    GIS/SpatialDataTest/RasterOperationsTest.cpp
    GIS/SpatialDataTest/ErrorHandlingTest.cpp
    GIS/SpatialDataTest/MultiAdminLevelTest.cpp
    GIS/AdminLevelManagerTest/BasicOperationsTest.cpp
    GIS/AdminLevelManagerTest/BoundaryValidationTest.cpp
    GIS/AdminLevelManagerTest/ConfigurationTest.cpp
    GIS/AdminLevelManagerTest/LookupOperationsTest.cpp
)

set(TEST_SRC_FILES
    main.cpp
    sample_catch_test.cpp
    sample_yaml_cpp_test.cpp
    person_test.cpp
    Environment/SeasonalPatternTest.cpp
    ${GIS_TEST_FILES}
    #SimpleFakeItTest.cpp
    #Spatial/CoordinateTest.cpp
    #Spatial/LocationTest.cpp
    #Core/RandomTest.cpp
    #Core/TimeHelpersTest.cpp
    #Core/StringHelpersTest.cpp
    #Core/Config/ConfigTest.cpp
    )

add_executable(${PROJECT_TEST_NAME} ${TEST_SRC_FILES})
add_dependencies(${PROJECT_TEST_NAME} MaSimCore)

target_link_libraries(${PROJECT_TEST_NAME}
    MaSimCore
    Catch2::Catch2
# Catch2::Catch2WithMain
    )

target_compile_features(${PROJECT_TEST_NAME} PRIVATE cxx_std_17)


# add_custom_command(TARGET ${PROJECT_TEST_NAME} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different
#     ${PROJECT_SOURCE_DIR}/misc/input.yml $<TARGET_FILE_DIR:${PROJECT_TEST_NAME}>)

#
install(TARGETS ${PROJECT_TEST_NAME} DESTINATION ${PROJECT_SOURCE_DIR}/bin)
#install(FILES ${PROJECT_SOURCE_DIR}/misc/input.yml DESTINATION ${PROJECT_SOURCE_DIR}/bin)

