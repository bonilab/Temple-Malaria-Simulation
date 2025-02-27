#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "GIS/SpatialData.h"
#include "TestHelpers.h"
#include <yaml-cpp/yaml.h>
#include "Core/Config/Config.h"
#include "Model.h"
#include "easylogging++.h"

using Catch::Matchers::WithinRel;

class RasterOperationsTest : public SpatialDataTestHelper {

};

TEST_CASE_METHOD(RasterOperationsTest, "Raster Data Operations", "[GIS][Raster]") {
    SetUp();

    // SECTION("Get cell population") {
    //     auto& spatial_data = SpatialData::get_instance();
        
    //     REQUIRE_THAT(spatial_data.get_population(0), WithinRel(100.0, 0.001));
    //     REQUIRE_THAT(spatial_data.get_population(4), WithinRel(180.0, 0.001));
    //     REQUIRE_THAT(spatial_data.get_population(7), WithinRel(240.0, 0.001));
    // }

    // SECTION("Get cell coordinates") {
    //     auto& spatial_data = SpatialData::get_instance();
        
    //     auto coords = spatial_data.get_cell_coordinates(0);
    //     REQUIRE(coords.first == 0);   // x coordinate
    //     REQUIRE(coords.second == 0);  // y coordinate
        
    //     coords = spatial_data.get_cell_coordinates(4);
    //     REQUIRE(coords.first == 1);   // x coordinate
    //     REQUIRE(coords.second == 1);  // y coordinate
    // }

    // SECTION("Location validation") {
    //     auto& spatial_data = SpatialData::get_instance();
        
    //     REQUIRE(spatial_data.is_valid_location(0));
    //     REQUIRE(spatial_data.is_valid_location(4));
    //     REQUIRE_FALSE(spatial_data.is_valid_location(6));  // NODATA cell
    //     REQUIRE_FALSE(spatial_data.is_valid_location(9));  // Out of bounds
    // }

    TearDown();
} 