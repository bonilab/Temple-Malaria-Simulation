#include <catch2/catch_test_macros.hpp>
#include "GIS/SpatialData.h"
#include "TestHelpers.h"
#include <yaml-cpp/yaml.h>
#include "Core/Config/Config.h"
#include "Model.h"
#include "easylogging++.h"

class BasicOperationsTest : public SpatialDataTestHelper {
};

TEST_CASE_METHOD(BasicOperationsTest, "Basic Raster Loading", "[GIS][Basic]") {
    SetUp();

    SECTION("Load district raster") {
        auto& spatial_data = SpatialData::get_instance();
        auto node = createBasicNode();
        
        REQUIRE(spatial_data.parse(node));
        REQUIRE(spatial_data.using_raster);
        REQUIRE(spatial_data.district_count == 2);  // Districts 1 and 2
    }

    SECTION("Load population raster") {
        auto& spatial_data = SpatialData::get_instance();
        auto node = createBasicNode();
        
        REQUIRE(spatial_data.parse(node));
        REQUIRE(spatial_data.using_raster);
        auto header = spatial_data.get_raster_header();
        REQUIRE(header.number_columns == 3);
        REQUIRE(header.number_rows == 3);
        REQUIRE(header.cellsize == 1.0f);
    }

    SECTION("Parse complete configuration") {
        auto& spatial_data = SpatialData::get_instance();
        auto node = createBasicNode();
        
        REQUIRE(spatial_data.parse(node));
        REQUIRE(spatial_data.using_raster);
        REQUIRE(spatial_data.district_count == 2);
    }

    TearDown();
}