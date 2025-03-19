#include "TestHelpers.h"

class LookupOperationsTest : public AdminLevelManagerTestHelper {
protected:
    void setup_test_boundaries() {
        // Create a simple district raster with known values
        std::vector<std::vector<int>> district_values = {
            {1, 1, 2},
            {1, 1, 2},
            {3, 3, 3}
        };
        create_custom_raster("test_district.asc", district_values);
        
        REQUIRE_NOTHROW(manager.register_level("district"));
        auto district_raster = std::unique_ptr<AscFile>(AscFileManager::read("test_district.asc"));
        REQUIRE_NOTHROW(manager.setup_boundary("district", district_raster.get()));
    }
};

TEST_CASE_METHOD(LookupOperationsTest, "Location to Admin Unit Mapping", "[AdminLevel][Lookup]") {
    SetUp();
    setup_test_boundaries();

    SECTION("Get admin unit for location") {
        // Test various locations based on the known raster pattern
        REQUIRE(manager.get_admin_unit("district", 0) == 1);  // First district
        REQUIRE(manager.get_admin_unit("district", 4) == 1);  // Second district
        REQUIRE(manager.get_admin_unit("district", 2) == 2);  // Second district
        REQUIRE(manager.get_admin_unit("district", 8) == 3);  // Third district
    }

    SECTION("Invalid location handling") {
        REQUIRE_THROWS_AS(manager.get_admin_unit("district", -1), std::out_of_range);
        REQUIRE_THROWS_AS(manager.get_admin_unit("district", 999), std::out_of_range);
    }

    TearDown();
}

TEST_CASE_METHOD(LookupOperationsTest, "Admin Unit to Locations Mapping", "[AdminLevel][Lookup]") {
    SetUp();
    setup_test_boundaries();

    SECTION("Get locations in admin unit") {
        auto locations_d1 = manager.get_locations_in_unit("district", 1);
        REQUIRE(locations_d1.size() == 4);  // District 1 has 4 cells
        
        auto locations_d2 = manager.get_locations_in_unit("district", 2);
        REQUIRE(locations_d2.size() == 2);  // District 2 has 2 cells
        
        auto locations_d3 = manager.get_locations_in_unit("district", 3);
        REQUIRE(locations_d3.size() == 3);  // District 3 has 3 cells
    }

    SECTION("Invalid admin unit handling") {
        REQUIRE_THROWS_AS(manager.get_locations_in_unit("district", 999), std::out_of_range);
    }

    TearDown();
}

TEST_CASE_METHOD(LookupOperationsTest, "Boundary Data Access", "[AdminLevel][Lookup]") {
    SetUp();
    setup_test_boundaries();

    SECTION("Get boundary data") {
        const auto* boundary = manager.get_boundary("district");
        REQUIRE(boundary != nullptr);
        REQUIRE(boundary->unit_count == 3);  // Three unique districts
    }

    SECTION("Get unit count") {
        REQUIRE(manager.get_unit_count("district") == 3);
    }

    SECTION("Invalid boundary access") {
        REQUIRE(manager.get_boundary("nonexistent") == nullptr);
        REQUIRE_THROWS_AS(manager.get_unit_count("nonexistent"), std::runtime_error);
    }

    TearDown();
}