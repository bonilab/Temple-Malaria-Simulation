#include "TestHelpers.h"

class BasicOperationsTest : public AdminLevelManagerTestHelper {
};

TEST_CASE_METHOD(BasicOperationsTest, "Basic Registration Operations", "[AdminLevel][Basic]") {
    SetUp();

    SECTION("Register new administrative level") {
        REQUIRE_NOTHROW(manager.register_level("district", "Test District"));
        REQUIRE(manager.has_level("district"));
        REQUIRE(manager.has_district());
    }

    SECTION("Register multiple levels") {
        REQUIRE_NOTHROW(manager.register_level("district", "Test District"));
        REQUIRE_NOTHROW(manager.register_level("province", "Test Province"));
        REQUIRE(manager.has_level("district"));
        REQUIRE(manager.has_level("province"));
        
        auto levels = manager.get_level_names();
        REQUIRE(levels.size() == 2);
        REQUIRE(levels[0] == "district");
        REQUIRE(levels[1] == "province");
    }

    SECTION("Prevent duplicate registration") {
        REQUIRE_NOTHROW(manager.register_level("district"));
        REQUIRE_THROWS_AS(manager.register_level("district"), std::runtime_error);
    }

    TearDown();
}

TEST_CASE_METHOD(BasicOperationsTest, "Basic Boundary Setup", "[AdminLevel][Basic]") {
    SetUp();

    SECTION("Setup district boundary") {
        // Create a simple district raster with known values matching our 3x3 grid
        std::vector<std::vector<int>> district_values = {
            {1, 1, 2},
            {1, 2, 2},
            {3, 3, 3}
        };
        create_custom_raster("test_district.asc", district_values);
        
        // Verify the raster file was created
        {
            std::ifstream test_file("test_district.asc");
            REQUIRE(test_file.good());
        }
        
        // Register and verify district level
        int district_id = manager.register_level("district");
        REQUIRE(district_id == 0);  // First registration should have ID 0
        REQUIRE(manager.has_level("district"));
        REQUIRE(manager.has_district());

        // Load and verify raster before moving
        AscFile* raw_raster = AscFileManager::read("test_district.asc");
        REQUIRE(raw_raster != nullptr);
        REQUIRE(raw_raster->NROWS == 3);
        REQUIRE(raw_raster->NCOLS == 3);
        
        // Verify location database is properly set up
        REQUIRE(Model::CONFIG != nullptr);
        REQUIRE(Model::CONFIG->number_of_locations() == 9);
        REQUIRE(Model::CONFIG->location_db().size() == 9);
        
        // Verify first location has valid coordinates
        REQUIRE(Model::CONFIG->location_db()[0].coordinate != nullptr);
        
        // Setup boundary with verified raster
        auto raster = std::unique_ptr<AscFile>(raw_raster);
        manager.setup_boundary("district", std::move(raster));
        
        // Verify the unit count and mappings
        REQUIRE(manager.get_unit_count("district") == 3);  // Districts 1, 2, and 3
        
        // Verify some specific location-to-district mappings
        REQUIRE(manager.get_admin_unit(0, "district") == 1);  // Top-left location should be in district 1
        REQUIRE(manager.get_admin_unit(2, "district") == 2);  // Top-right location should be in district 2
        REQUIRE(manager.get_admin_unit(8, "district") == 3);  // Bottom-right location should be in district 3
    }

    SECTION("Setup multiple boundaries safely") {
        // Register both levels first
        REQUIRE_NOTHROW(manager.register_level("district"));
        REQUIRE_NOTHROW(manager.register_level("province"));
        
        // Create district raster
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
        
        // Setup boundaries one at a time, verifying each step
        auto district_raster = std::unique_ptr<AscFile>(AscFileManager::read("test_district.asc"));
        REQUIRE(district_raster != nullptr);
        REQUIRE_NOTHROW(manager.setup_boundary("district", std::move(district_raster)));
        
        auto province_raster = std::unique_ptr<AscFile>(AscFileManager::read("test_province.asc"));
        REQUIRE(province_raster != nullptr);
        REQUIRE_NOTHROW(manager.setup_boundary("province", std::move(province_raster)));
        
        // Verify final state
        REQUIRE(manager.get_unit_count("district") == 3);
        REQUIRE(manager.get_unit_count("province") == 2);
    }

    TearDown();
}
