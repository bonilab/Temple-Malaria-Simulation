#include <catch2/catch_test_macros.hpp>
#include "GIS/SpatialData.h"
#include "TestHelpers.h"
#include <yaml-cpp/yaml.h>
#include "Core/Config/Config.h"
#include "Model.h"
#include "easylogging++.h"

class DistrictOperationsTest : public SpatialDataTestHelper {
};

TEST_CASE_METHOD(DistrictOperationsTest, "District Mapping Operations", "[GIS][District]") {
    SetUp();

    SECTION("Get district for location") {
        auto& spatial_data = SpatialData::get_instance();
        
        // Test various locations
        REQUIRE(spatial_data.get_admin_unit("district", 0) == 1);  // First cell, district 1
        REQUIRE(spatial_data.get_admin_unit("district", 2) == 2);  // Third cell, district 2
        REQUIRE(spatial_data.get_admin_unit("district", 4) == 2);  // Fifth cell, district 2
    }

    SECTION("Get district locations") {
        auto& spatial_data = SpatialData::get_instance();
        
        // Test district 1
        auto locations_d1 = spatial_data.get_locations_in_unit("district", 1);
        REQUIRE(locations_d1.size() == 3);  // District 1 has 3 cells
        
        // Test district 2
        auto locations_d2 = spatial_data.get_locations_in_unit("district", 2);
        REQUIRE(locations_d2.size() == 5);  // District 2 has 5 cells
    }

    // SECTION("District population") {
    //     auto& spatial_data = SpatialData::get_instance();
        
    //     // Calculate expected populations
    //     int pop_d1 = 100 + 120;  // Sum of population in district 1 cells
    //     int pop_d2 = 200 + 220 + 180 + 160 + 240;  // Sum of population in district 2 cells
        
    //     REQUIRE(spatial_data.get_district_population(1) == pop_d1);
    //     REQUIRE(spatial_data.get_district_population(2) == pop_d2);
    // }

    TearDown();
} 