#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>


#include "GIS/SpatialData.h"
#include "TestHelpers.h"

class AdminBoundaryFixture : public MultiAdminLevelTest {
public:
    AdminBoundaryFixture() {
        SetUp();
    }
    
    ~AdminBoundaryFixture() {
        TearDown();
    }
};

class CustomAdminLevelFixture : public SpatialDataTestHelper {
public:
    CustomAdminLevelFixture() {
        // Add custom admin levels
        add_admin_level("zone");
        add_admin_level("country");
        SetUp();
    }
    
    ~CustomAdminLevelFixture() {
        TearDown();
    }
};

class DistrictOnlyFixture : public SpatialDataTestHelper {
public:
    DistrictOnlyFixture() {
        SetUp();
    }
    
    ~DistrictOnlyFixture() {
        TearDown();
    }
};

TEST_CASE_METHOD(AdminBoundaryFixture, "Admin levels are properly initialized", "[admin_boundaries]") {
    auto& spatial_data = SpatialData::get_instance();
    
    // Check that all admin levels are available
    auto admin_levels = spatial_data.get_admin_levels();
    REQUIRE(admin_levels.size() == 3); // district, province, region
    
    // Check that specific levels exist
    CHECK(spatial_data.has_admin_level("district"));
    CHECK(spatial_data.has_admin_level("province"));
    CHECK(spatial_data.has_admin_level("region"));
    CHECK_FALSE(spatial_data.has_admin_level("nonexistent"));
}

TEST_CASE_METHOD(AdminBoundaryFixture, "Admin unit queries work correctly", "[admin_boundaries]") {
    auto& spatial_data = SpatialData::get_instance();
    
    // Get all units for each level
    auto district_units = spatial_data.get_admin_units("district");
    auto province_units = spatial_data.get_admin_units("province");
    auto region_units = spatial_data.get_admin_units("region");
    
    // We should have 2 districts, 3 provinces (based on our test data)
    // return is pair of (min_unit_id, max_unit_id)
    CHECK(district_units == std::make_pair(1, 2));
    CHECK(province_units == std::make_pair(1, 3));
    CHECK(region_units == std::make_pair(1, 3));
    // Get unit counts directly
    CHECK(spatial_data.get_unit_count("district") == 2);
    CHECK(spatial_data.get_unit_count("province") == 3);
    CHECK(spatial_data.get_unit_count("region") == 3);
}

TEST_CASE_METHOD(AdminBoundaryFixture, "Location to admin unit mapping works correctly", "[admin_boundaries]") {
    auto& spatial_data = SpatialData::get_instance();
    
    // Our test setup has 8 locations
    REQUIRE(Model::CONFIG->number_of_locations() == 8);
    
    // Check district for each location (using our test raster pattern)
    CHECK(spatial_data.get_admin_unit("district", 0) == 1);
    CHECK(spatial_data.get_admin_unit("district", 1) == 1);
    CHECK(spatial_data.get_admin_unit("district", 2) == 2);
    CHECK(spatial_data.get_admin_unit("district", 3) == 1);
    CHECK(spatial_data.get_admin_unit("district", 4) == 2);
    CHECK(spatial_data.get_admin_unit("district", 5) == 2);
    CHECK(spatial_data.get_admin_unit("district", 6) == 2);
    CHECK(spatial_data.get_admin_unit("district", 7) == 2);
    
    // Check province for each location (using our test raster pattern which is different)
    CHECK(spatial_data.get_admin_unit("province", 0) == 1);
    CHECK(spatial_data.get_admin_unit("province", 1) == 2);
    CHECK(spatial_data.get_admin_unit("province", 2) == 2);
    CHECK(spatial_data.get_admin_unit("province", 3) == 1);
    CHECK(spatial_data.get_admin_unit("province", 4) == 1);
    CHECK(spatial_data.get_admin_unit("province", 5) == 3);
    CHECK(spatial_data.get_admin_unit("province", 6) == 3);
    CHECK(spatial_data.get_admin_unit("province", 7) == 3);
}

TEST_CASE_METHOD(AdminBoundaryFixture, "Admin unit to locations mapping works correctly", "[admin_boundaries]") {
    auto& spatial_data = SpatialData::get_instance();
    
    // Check locations in each district
    auto district1_locations = spatial_data.get_locations_in_unit("district", 1);
    auto district2_locations = spatial_data.get_locations_in_unit("district", 2);
    
    // Based on our test raster
    CHECK_THAT(district1_locations, Catch::Matchers::UnorderedEquals(std::vector<int>{0, 1, 3}));
    CHECK_THAT(district2_locations, Catch::Matchers::UnorderedEquals(std::vector<int>{2, 4, 5, 6, 7}));
    
    // Check locations in each province
    auto province1_locations = spatial_data.get_locations_in_unit("province", 1);
    auto province2_locations = spatial_data.get_locations_in_unit("province", 2);
    auto province3_locations = spatial_data.get_locations_in_unit("province", 3);
    
    // Based on our test raster for province
    CHECK_THAT(province1_locations, Catch::Matchers::UnorderedEquals(std::vector<int>{0, 3, 4}));
    CHECK_THAT(province2_locations, Catch::Matchers::UnorderedEquals(std::vector<int>{1, 2}));
    CHECK_THAT(province3_locations, Catch::Matchers::UnorderedEquals(std::vector<int>{5, 6, 7}));
}

TEST_CASE_METHOD(AdminBoundaryFixture, "Error handling works correctly", "[admin_boundaries]") {
    auto& spatial_data = SpatialData::get_instance();
    
    // Invalid admin level
    CHECK_THROWS_AS(spatial_data.get_admin_unit("nonexistent", 0), std::runtime_error);
    CHECK_THROWS_AS(spatial_data.get_locations_in_unit("nonexistent", 1), std::runtime_error);
    
    // Invalid unit ID
    CHECK_THROWS_AS(spatial_data.get_locations_in_unit("district", 999), std::out_of_range);
    
    // Invalid location ID
    CHECK_THROWS_AS(spatial_data.get_admin_unit("district", 999), std::out_of_range);
}

TEST_CASE_METHOD(CustomAdminLevelFixture, "Custom admin levels work correctly", "[admin_boundaries]") {
    auto& spatial_data = SpatialData::get_instance();
    
    // Check that all admin levels are available
    auto admin_levels = spatial_data.get_admin_levels();
    REQUIRE(admin_levels.size() == 3); // district, zone, country
    
    // Check that specific levels exist
    CHECK(spatial_data.has_admin_level("district"));
    CHECK(spatial_data.has_admin_level("zone"));
    CHECK(spatial_data.has_admin_level("country"));
}

TEST_CASE_METHOD(AdminBoundaryFixture, "Boundary data access works correctly", "[admin_boundaries]") {
    auto& spatial_data = SpatialData::get_instance();
    
    // Get boundary data for each level
    const auto* district_boundary = spatial_data.get_boundary("district");
    const auto* province_boundary = spatial_data.get_boundary("province");
    const auto* region_boundary = spatial_data.get_boundary("region");
    
    // Check that boundary data is not null
    REQUIRE(district_boundary != nullptr);
    REQUIRE(province_boundary != nullptr);
    REQUIRE(region_boundary != nullptr);
    
    // Check boundary properties
    CHECK(district_boundary->min_unit_id == 1); // 1-based IDs in our test
    CHECK(district_boundary->max_unit_id == 2);
    CHECK(district_boundary->unit_count == 2);
    
    CHECK(province_boundary->min_unit_id == 1);
    CHECK(province_boundary->max_unit_id == 3);
    CHECK(province_boundary->unit_count == 3);
    
    // Check that location_to_unit has the right size (number of locations)
    CHECK(district_boundary->location_to_unit.size() == 8);
    CHECK(province_boundary->location_to_unit.size() == 8);
    
    // Check that unit_to_locations has the right size (max_unit_id + 1)
    CHECK(district_boundary->unit_to_locations.size() == 3); // 0-indexed plus max_id=2
    CHECK(province_boundary->unit_to_locations.size() == 4); // 0-indexed plus max_id=3
}

TEST_CASE_METHOD(DistrictOnlyFixture, "District only mode works correctly for backward compatibility", "[admin_boundaries]") {
    auto& spatial_data = SpatialData::get_instance();
    
    // Check that only district level is available
    auto admin_levels = spatial_data.get_admin_levels();
    REQUIRE(admin_levels.size() == 1); // district only
    CHECK(admin_levels[0] == "district");
    
    // Check that district level is accessible
    CHECK(spatial_data.has_admin_level("district"));
    CHECK(spatial_data.get_unit_count("district") == 2);
    
    // Check district for a few locations
    CHECK(spatial_data.get_admin_unit("district", 0) == 1);
    CHECK(spatial_data.get_admin_unit("district", 2) == 2);
    
    // Check locations in district 2
    auto district2_locations = spatial_data.get_locations_in_unit("district", 2);
    CHECK_THAT(district2_locations, Catch::Matchers::UnorderedEquals(std::vector<int>{2, 4, 5, 6, 7}));
} 