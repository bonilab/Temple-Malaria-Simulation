#include "Environment/SeasonalPattern.h"
#include "Helpers/TimeHelpers.h"
#include "SeasonalPatternFixture.h"
#include "GIS/SpatialData.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <yaml-cpp/yaml.h>
#include <fmt/format.h>
#include <fstream>

using Catch::Matchers::WithinRel;

// Test helper class that overrides district mapping
class TestSeasonalPattern : public SeasonalPattern {
public:
    static TestSeasonalPattern* build(const YAML::Node &node, SpatialData* spatial_data) {
        auto value = new TestSeasonalPattern();
        value->initialize(node, spatial_data);
        return value;
    }

    static SpatialData* create_fake_spatial_data() {
        std::cout << "Creating fake spatial data" << std::endl;
        auto fake_spatial_data = new SpatialData();
        fake_spatial_data->get_admin_level_manager()->register_level("district");
        auto boundary = BoundaryData();
        boundary.location_to_unit = {1, 2};
        boundary.unit_to_locations = {{1, {1}}, {2, {2}}};
        boundary.min_unit_id = 1;
        boundary.max_unit_id = 2;
        boundary.unit_count = 2;
        fake_spatial_data->get_admin_level_manager()->set_boundary(0, boundary);
        return fake_spatial_data;
    }
protected:
    // Override district mapping for testing
    int get_admin_unit_for_location(int location) const override {
        if (location == 999){
            throw std::out_of_range("Location is out of range");
        }
        return location;  // Simple mapping: location a -> district a
    }
};

TEST_CASE_METHOD(SeasonalPatternFixture, "SeasonalPattern", "[Environment]") {
    SetUp();  // Create test data files
    
    // Helper function to create dates
    auto make_date = [](int year, int month, int day) {
        return date::sys_days{date::year{year}/month/day};
    };

    SECTION("Can create SeasonalPattern with monthly data") {
        auto node = YAML::Load(fmt::format(R"(
          enable: true
          mode: "PATTERN"
          pattern:
            admin_level: "district"
            filename: "{}"
            period: 12
        )", monthly_file));

        auto fake_spatial_data = std::unique_ptr<SpatialData>(TestSeasonalPattern::create_fake_spatial_data());
        auto pattern = TestSeasonalPattern::build(node, fake_spatial_data.get());
        REQUIRE(pattern != nullptr);
        REQUIRE(pattern->get_is_monthly());
        REQUIRE(pattern->get_period() == 12);
    }

    SECTION("Can create SeasonalPattern with daily data") {
        auto node = YAML::Load(fmt::format(R"(
          enable: true
          mode: "PATTERN"
          pattern:
            admin_level: "district"
            filename: "{}"
            period: 365
        )", daily_file));

        auto fake_spatial_data = std::unique_ptr<SpatialData>(TestSeasonalPattern::create_fake_spatial_data());
        auto pattern = TestSeasonalPattern::build(node, fake_spatial_data.get());
        REQUIRE(pattern != nullptr);
        REQUIRE(!pattern->get_is_monthly());
        REQUIRE(pattern->get_period() == 365);
    }

    SECTION("Throws error on invalid period") {
        auto node = YAML::Load(fmt::format(R"(
          enable: true
          mode: "PATTERN"
          pattern:
            admin_level: "district"
            filename: "{}"
            period: 100
        )", "test_pattern.csv"));

        auto fake_spatial_data = std::unique_ptr<SpatialData>(TestSeasonalPattern::create_fake_spatial_data());
        REQUIRE_THROWS_AS(TestSeasonalPattern::build(node, fake_spatial_data.get()), std::invalid_argument);
    }

    SECTION("Can read monthly pattern data") {
        auto node = YAML::Load(fmt::format(R"(
          enable: true
          mode: "PATTERN"
          pattern:
            admin_level: "district"
            filename: "{}"
            period: 12
        )", monthly_file));

        auto fake_spatial_data = std::unique_ptr<SpatialData>(TestSeasonalPattern::create_fake_spatial_data());
        auto pattern = TestSeasonalPattern::build(node, fake_spatial_data.get());
        REQUIRE(pattern != nullptr);
        
        // Test pattern values for location 1
        REQUIRE_THAT(pattern->get_seasonal_factor(make_date(2000, 1, 15), 1), WithinRel(0.5, 0.00001));  // January
        REQUIRE_THAT(pattern->get_seasonal_factor(make_date(2000, 7, 15), 1), WithinRel(1.1, 0.00001)); // July
        REQUIRE_THAT(pattern->get_seasonal_factor(make_date(2000, 12, 15), 1), WithinRel(0.6, 0.00001)); // December

        // Test pattern values for location 2
        REQUIRE_THAT(pattern->get_seasonal_factor(make_date(2000, 1, 15), 2), WithinRel(0.6, 0.00001));  // January
        REQUIRE_THAT(pattern->get_seasonal_factor(make_date(2000, 7, 15), 2), WithinRel(1.2, 0.00001)); // July
        REQUIRE_THAT(pattern->get_seasonal_factor(make_date(2000, 12, 15), 2), WithinRel(0.7, 0.00001)); // December
    }

    SECTION("Can read daily pattern data") {
        auto node = YAML::Load(fmt::format(R"(
          enable: true
          mode: "PATTERN"
          pattern:
            admin_level: "district"
            filename: "{}"
            period: 365
        )", daily_file));

        auto fake_spatial_data = std::unique_ptr<SpatialData>(TestSeasonalPattern::create_fake_spatial_data());
        auto pattern = TestSeasonalPattern::build(node, fake_spatial_data.get());
        REQUIRE(pattern != nullptr);
        
        // Test pattern values
        REQUIRE_THAT(pattern->get_seasonal_factor(make_date(2000, 1, 1),1), WithinRel(0.501, 0.00001));
        REQUIRE_THAT(pattern->get_seasonal_factor(make_date(2000, 6, 28), 1), WithinRel(0.68, 0.00001));  // ~day 180
        REQUIRE_THAT(pattern->get_seasonal_factor(make_date(2000, 12, 31), 1), WithinRel(0.865, 0.00001));
    }

    SECTION("Handles leap year correctly") {
        auto node = YAML::Load(fmt::format(R"(
          enable: true
          mode: "PATTERN"
          pattern:
            admin_level: "district"
            filename: "{}"
            period: 365
        )", daily_file));

        auto fake_spatial_data = std::unique_ptr<SpatialData>(TestSeasonalPattern::create_fake_spatial_data());
        auto pattern = TestSeasonalPattern::build(node, fake_spatial_data.get());
        
        // Dec 30th (day 365) in leap year should use same value as Dec 31st (day 366)
        auto dec30 = make_date(2000, 12, 30);
        auto dec31 = make_date(2000, 12, 31);
        REQUIRE_THAT(pattern->get_seasonal_factor(dec30, 1), 
                    WithinRel(pattern->get_seasonal_factor(dec31, 1), 0.00001));
    }

    SECTION("Handles missing district data") {
        auto node = YAML::Load(fmt::format(R"(
          enable: true
          mode: "PATTERN"
          pattern:
            admin_level: "district"
            filename: "{}"
            period: 12
        )", monthly_file));

        auto fake_spatial_data = std::unique_ptr<SpatialData>(TestSeasonalPattern::create_fake_spatial_data());
        auto pattern = TestSeasonalPattern::build(node, fake_spatial_data.get());
        
        // Try to access non-existent district
        REQUIRE_THROWS_AS(pattern->get_seasonal_factor(make_date(2000, 1, 1), 999), 
                         std::out_of_range);
    }

    SECTION("Handles malformed CSV file") {
        // Create malformed CSV file
        std::ofstream bad_file("test_bad_pattern.csv");
        bad_file << "district_id,jan,feb,mar\n";  // Missing columns
        bad_file << "1,0.5,0.6\n";  // Missing values
        bad_file.close();

        auto node = YAML::Load(R"(
          enable: true
          mode: "PATTERN"
          pattern:
            admin_level: "district"
            filename: "test_bad_pattern.csv"
            period: 12
        )");

        auto fake_spatial_data = std::unique_ptr<SpatialData>(TestSeasonalPattern::create_fake_spatial_data());
        REQUIRE_THROWS(TestSeasonalPattern::build(node, fake_spatial_data.get()));
        std::remove("test_bad_pattern.csv");
    }

    SECTION("Handles negative values in CSV") {
        // Create CSV with negative values
        std::ofstream bad_file("test_negative_pattern.csv");
        bad_file << "district_id,jan,feb,mar,apr,may,jun,jul,aug,sep,oct,nov,dec\n";
        bad_file << "1,0.5,-0.6,0.7,0.8,0.9,1.0,1.1,1.0,0.9,0.8,0.7,0.6\n";
        bad_file.close();

        auto node = YAML::Load(R"(
          enable: true
          mode: "PATTERN"
          pattern:
            admin_level: "district"
            filename: "test_negative_pattern.csv"
            period: 12
        )");

        auto fake_spatial_data = std::unique_ptr<SpatialData>(TestSeasonalPattern::create_fake_spatial_data());
        REQUIRE_THROWS_AS(TestSeasonalPattern::build(node, fake_spatial_data.get()), std::runtime_error);
        std::remove("test_negative_pattern.csv");
    }

    SECTION("Handles non-existent file") {
        auto node = YAML::Load(R"(
          enable: true
          mode: "PATTERN"
          pattern:
            admin_level: "district"
            filename: "non_existent_file.csv"
            period: 12
        )");

        auto fake_spatial_data = std::unique_ptr<SpatialData>(TestSeasonalPattern::create_fake_spatial_data());
        REQUIRE_THROWS(TestSeasonalPattern::build(node, fake_spatial_data.get()));
    }

    SECTION("Validates monthly data expansion") {
        auto node = YAML::Load(fmt::format(R"(
          enable: true
          mode: "PATTERN"
          pattern:
            admin_level: "district"
            filename: "{}"
            period: 12
        )", monthly_file));

        auto fake_spatial_data = std::unique_ptr<SpatialData>(TestSeasonalPattern::create_fake_spatial_data());
        auto pattern = TestSeasonalPattern::build(node, fake_spatial_data.get());
        
        // Check February has 28 days
        auto feb1 = make_date(2000, 2, 1);
        auto feb28 = make_date(2000, 2, 28);
        REQUIRE_THAT(pattern->get_seasonal_factor(feb1, 1), 
                    WithinRel(pattern->get_seasonal_factor(feb28, 1), 0.00001));

        // Check months with 31 days
        auto jan1 = make_date(2000, 1, 1);
        auto jan31 = make_date(2000, 1, 31);
        REQUIRE_THAT(pattern->get_seasonal_factor(jan1, 1), 
                    WithinRel(pattern->get_seasonal_factor(jan31, 1), 0.00001));
    }

    TearDown();  // Clean up test data files
} 