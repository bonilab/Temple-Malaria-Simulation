#include <catch2/catch_test_macros.hpp>
#include "GIS/SpatialData.h"
#include "TestHelpers.h"
#include <yaml-cpp/yaml.h>
#include "Core/Config/Config.h"
#include "Model.h"
#include "easylogging++.h"

class ErrorHandlingTest : public SpatialDataTestHelper {
protected:
    void create_mismatched_raster(const std::string& filename) {
        std::ofstream file(filename);
        file << "ncols 4\n";  // Different dimensions
        file << "nrows 4\n";
        file << "xllcorner 0.0\n";
        file << "yllcorner 0.0\n";
        file << "cellsize 1.0\n";
        file << "NODATA_value -9999\n";
        file << "1 1 1 1\n";
        file << "1 1 1 1\n";
        file << "1 1 1 1\n";
        file << "1 1 1 1\n";
        file.close();
    }

    void create_invalid_district_raster(const std::string& filename) {
        std::ofstream file(filename);
        file << "ncols 3\nnrows 3\nxllcorner 0.0\nyllcorner 0.0\ncellsize 1.0\nNODATA_value -9999\n";
        file << "1 1 3\n";  // Gap in district numbers
        file << "1 3 3\n";
        file << "-9999 3 3\n";
        file.close();
    }
};

TEST_CASE_METHOD(ErrorHandlingTest, "Error Handling for File Loading", "[GIS][Error]") {
    // Initialize Model::CONFIG but don't do full setup
    if (Model::CONFIG == nullptr) {
        Model::CONFIG = new Config();
    }

    SECTION("Missing files") {
        auto& spatial_data = SpatialData::get_instance();
        spatial_data.reset();  // Reset before test
        
        auto node = YAML::Node();  // Create minimal node
        node["district_raster"] = "nonexistent.asc";
        REQUIRE_THROWS_AS(spatial_data.parse(node), std::runtime_error);
    }

    SECTION("Invalid district numbering") {
        auto& spatial_data = SpatialData::get_instance();
        spatial_data.reset();  // Reset before test
        
        // Create both required files
        create_invalid_district_raster("test_invalid.asc");
        create_population_raster("test_population.asc");
        
        auto node = createBasicNode();
        node["district_raster"] = "test_invalid.asc";
        REQUIRE_THROWS_AS(spatial_data.parse(node), std::invalid_argument);
        
        // Clean up both files
        std::remove("test_invalid.asc");
        std::remove("test_population.asc");
    }

    SECTION("Mismatched raster dimensions") {
        auto& spatial_data = SpatialData::get_instance();
        spatial_data.reset();  // Reset before test
        
        // Create both required files
        create_district_raster("test_district.asc");
        create_mismatched_raster("test_mismatch.asc");
        
        auto node = createBasicNode();
        node["population_raster"] = "test_mismatch.asc";
        REQUIRE_THROWS_AS(spatial_data.parse(node), std::runtime_error);
        
        // Clean up both files
        std::remove("test_district.asc");
        std::remove("test_mismatch.asc");
    }

    // Cleanup
    if (Model::CONFIG != nullptr) {
        delete Model::CONFIG;
        Model::CONFIG = nullptr;
    }
}

TEST_CASE_METHOD(ErrorHandlingTest, "Error Handling for Data Access", "[GIS][Error]") {
    SetUp();  // Use normal setup for testing data access

    auto& spatial_data = SpatialData::get_instance();

    SECTION("Invalid location access") {
        REQUIRE_THROWS_AS(spatial_data.get_district(999), std::out_of_range);
        // Add other boundary tests as needed:
        // REQUIRE_THROWS_AS(spatial_data.get_population(999), std::out_of_range);
        // REQUIRE_THROWS_AS(spatial_data.get_cell_coordinates(999), std::out_of_range);
    }

    TearDown();
} 