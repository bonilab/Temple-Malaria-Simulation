#include "TestHelpers.h"
#include "catch2/matchers/catch_matchers.hpp"

class BoundaryValidationTest : public AdminLevelManagerTestHelper {
};

TEST_CASE_METHOD(BoundaryValidationTest, "Invalid Raster Validation", "[AdminLevel][Validation]") {
    SetUp();

    SECTION("Null raster handling") {
        REQUIRE_NOTHROW(manager.register_level("district"));
        REQUIRE_THROWS_AS(manager.setup_boundary("district", nullptr), 
                         std::runtime_error);
    }

    SECTION("Invalid raster dimensions") {
        std::ofstream file("test_invalid.asc");
        file << "ncols 0\n";  // Invalid dimension
        file << "nrows 3\n";
        file << "xllcorner 0.0\nyllcorner 0.0\ncellsize 1.0\nNODATA_value -9999\n";
        file << "1 1 1\n1 1 1\n1 1 1\n";
        file.close();

        REQUIRE_NOTHROW(manager.register_level("district"));
        auto district_raster = std::unique_ptr<AscFile>(AscFileManager::read("test_invalid.asc"));
        REQUIRE_THROWS(manager.setup_boundary("district", district_raster.get()));
    }

    SECTION("Invalid indexing values") {
        // Create raster with invalid starting index (2)
        std::vector<std::vector<int>> invalid_values = {
            {2, 2, 3},
            {2, 3, 3},
            {2, 3, 3}
        };
        create_custom_raster("test_invalid.asc", invalid_values);

        REQUIRE_NOTHROW(manager.register_level("district"));
        auto district_raster = std::unique_ptr<AscFile>(AscFileManager::read("test_invalid.asc"));
        REQUIRE_THROWS_AS(manager.setup_boundary("district", district_raster.get()),
            std::runtime_error);
    }

    SECTION("All NODATA values") {
        std::vector<std::vector<int>> nodata_values = {
            {-9999, -9999, -9999},
            {-9999, -9999, -9999},
            {-9999, -9999, -9999}
        };
        create_custom_raster("test_invalid.asc", nodata_values);

        REQUIRE_NOTHROW(manager.register_level("district"));
        auto district_raster = std::unique_ptr<AscFile>(AscFileManager::read("test_invalid.asc"));
        REQUIRE_THROWS(manager.setup_boundary("district", district_raster.get()));
    }

    TearDown();
}

TEST_CASE_METHOD(BoundaryValidationTest, "Raster Dimension Consistency", "[AdminLevel][Validation]") {
    SetUp();

    SECTION("Same dimensions are accepted") {
        // Create district raster (3x3)
        std::vector<std::vector<int>> district_values = {
            {1, 1, 2},
            {1, 2, 2},
            {3, 3, 3}
        };
        create_custom_raster("test_district.asc", district_values);

        // Create province raster with same dimensions
        std::vector<std::vector<int>> province_values = {
            {1, 1, 1},
            {1, 1, 1},
            {2, 2, 2}
        };
        create_custom_raster("test_province.asc", province_values);

        REQUIRE_NOTHROW(manager.register_level("district"));
        auto district_raster = std::unique_ptr<AscFile>(AscFileManager::read("test_district.asc"));
        REQUIRE_NOTHROW(manager.setup_boundary("district", district_raster.get()));
        
        REQUIRE_NOTHROW(manager.register_level("province"));    
        auto province_raster = std::unique_ptr<AscFile>(AscFileManager::read("test_province.asc"));
        REQUIRE_NOTHROW(manager.setup_boundary("province", province_raster.get()));
    }

    SECTION("Different dimensions are rejected") {
        // Create district raster (3x3)
        std::vector<std::vector<int>> district_values = {
            {1, 1, 2},
            {1, 2, 2},
            {3, 3, 3}
        };
        create_custom_raster("test_district.asc", district_values);

        // Create province raster with different dimensions (4x4)
        std::vector<std::vector<int>> province_values = {
            {1, 1, 1, 1},
            {1, 1, 1, 1},
            {2, 2, 2, 2},
            {2, 2, 2, 2}
        };
        create_custom_raster("test_province.asc", province_values);

        REQUIRE_NOTHROW(manager.register_level("district"));
        auto district_raster = std::unique_ptr<AscFile>(AscFileManager::read("test_district.asc"));
        REQUIRE_NOTHROW(manager.setup_boundary("district", district_raster.get()));
        
        REQUIRE_NOTHROW(manager.register_level("province"));
        auto province_raster = std::unique_ptr<AscFile>(AscFileManager::read("test_province.asc"));
        REQUIRE_NOTHROW(manager.setup_boundary("province", province_raster.get()));
        REQUIRE_THROWS(manager.validate(), "All boundaries must have the same dimensions.");
    }

    TearDown();
}

TEST_CASE_METHOD(BoundaryValidationTest, "Configuration Validation", "[AdminLevel][Validation]") {
    SetUp();

    SECTION("Missing district level") {
        REQUIRE_NOTHROW(manager.register_level("province"));
        REQUIRE_THROWS_AS(manager.validate(), std::runtime_error);
    }

    SECTION("Uninitialized boundary") {
        REQUIRE_NOTHROW(manager.register_level("district"));
        REQUIRE_THROWS_AS(manager.validate(), std::runtime_error);
    }

    SECTION("Valid configuration with district") {
        create_test_raster("test_district.asc");
        REQUIRE_NOTHROW(manager.register_level("district"));
        auto raster = std::unique_ptr<AscFile>(AscFileManager::read("test_district.asc"));
        REQUIRE_NOTHROW(manager.setup_boundary("district", raster.get()));
        REQUIRE_NOTHROW(manager.validate());
    }

    TearDown();
}
