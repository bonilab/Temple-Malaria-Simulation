#ifndef SPATIAL_DATA_TEST_HELPERS_H
#define SPATIAL_DATA_TEST_HELPERS_H

#include <fstream>
#include <string>
#include <yaml-cpp/yaml.h>
#include "Core/Config/Config.h"
#include "Model.h"
#include "GIS/SpatialData.h"

class SpatialDataTestHelper {
protected:
    // Define admin levels to test with, district is always included by default
    std::vector<std::string> additional_admin_levels;
    
    void SetUp() {
        // Create test files
        create_district_raster("test_district.asc", false);  // Use 1-based districts
        create_population_raster("test_population.asc");
        
        // Create additional admin level rasters if needed
        for (const auto& level : additional_admin_levels) {
            create_admin_level_raster("test_" + level + ".asc", false);
        }
        
        // Initialize Model::CONFIG
        if (Model::CONFIG == nullptr) {
            Model::CONFIG = new Config();
        }

        Model::CONFIG->number_of_locations() = 0;

        // Initialize spatial data
        auto& spatial_data = SpatialData::get_instance();
        auto node = createBasicNode();
        spatial_data.parse(node);
    }

    YAML::Node createBasicNode() {
        YAML::Node node;
        
        // Always use the administrative_boundaries format
        YAML::Node admin_boundaries;
        
        // Always add district first for backward compatibility
        YAML::Node district;
        district["name"] = "district";
        district["raster"] = "test_district.asc";
        admin_boundaries.push_back(district);
        
        // Add any additional admin levels
        for (const auto& level : additional_admin_levels) {
            YAML::Node admin_level;
            admin_level["name"] = level;
            admin_level["raster"] = "test_" + level + ".asc";
            admin_boundaries.push_back(admin_level);
        }
        
        node["administrative_boundaries"] = admin_boundaries;
        
        // Rest of the configuration remains the same
        node["population_raster"] = "test_population.asc";
        node["cell_size"] = 1.0;
        
        // Create age distribution for all locations
        YAML::Node age_distributions;
        std::vector<double> dist = {0.2, 0.3, 0.5};
        for (int i = 0; i < 8; i++) {
            YAML::Node age_dist;
            for (const auto& val : dist) {
                age_dist.push_back(val);
            }
            age_distributions.push_back(age_dist);
        }
        node["age_distribution_by_location"] = age_distributions;
        
        // Add treatment probabilities for all locations
        YAML::Node treatment_under5;
        YAML::Node treatment_over5;
        for (int i = 0; i < 8; i++) {
            treatment_under5.push_back(0.5);
            treatment_over5.push_back(0.5);
        }
        node["p_treatment_for_less_than_5_by_location"] = treatment_under5;
        node["p_treatment_for_more_than_5_by_location"] = treatment_over5;
        
        // Add beta and population size for all locations
        YAML::Node betas;
        YAML::Node populations;
        for (int i = 0; i < 8; i++) {
            betas.push_back(0.5);
            populations.push_back(100);
        }
        node["beta_by_location"] = betas;
        node["population_size_by_location"] = populations;
        
        return node;
    }

    void TearDown() {
        cleanup_files();
        SpatialData::get_instance().reset();
        Model::CONFIG->number_of_locations() = 0;
        Model::CONFIG->location_db().clear();
        
        if (Model::CONFIG != nullptr) {
            delete Model::CONFIG;
            Model::CONFIG = nullptr;
        }
        
        // Reset test configuration
        additional_admin_levels.clear();
    }

    static void create_district_raster(const std::string& filename, bool zero_based = true) {
        std::ofstream file(filename);
        // Basic ASC header
        file << "ncols 3\n";
        file << "nrows 3\n";
        file << "xllcorner 0.0\n";
        file << "yllcorner 0.0\n";
        file << "cellsize 1.0\n";
        file << "NODATA_value -9999\n";
        // Data: 3x3 grid with districts 0,1 or 1,2 based on zero_based flag
        int base = zero_based ? 0 : 1;
        file << (base) << " " << (base) << " " << (base+1) << "\n";
        file << (base) << " " << (base+1) << " " << (base+1) << "\n";
        file << "-9999 " << (base+1) << " " << (base+1) << "\n";
        file.close();
    }

    // Helper to create an admin level raster with different boundaries than district
    static void create_admin_level_raster(const std::string& filename, bool zero_based = true) {
        std::ofstream file(filename);
        // Basic ASC header
        file << "ncols 3\n";
        file << "nrows 3\n";
        file << "xllcorner 0.0\n";
        file << "yllcorner 0.0\n";
        file << "cellsize 1.0\n";
        file << "NODATA_value -9999\n";
        // Create a different pattern for this admin level
        int base = zero_based ? 0 : 1;
        file << (base) << " " << (base+1) << " " << (base+1) << "\n";
        file << (base) << " " << (base) << " " << (base+2) << "\n";
        file << "-9999 " << (base+2) << " " << (base+2) << "\n";
        file.close();
    }

    static void create_population_raster(const std::string& filename) {
        std::ofstream file(filename);
        file << "ncols 3\n";
        file << "nrows 3\n";
        file << "xllcorner 0.0\n";
        file << "yllcorner 0.0\n";
        file << "cellsize 1.0\n";
        file << "NODATA_value -9999\n";
        file << "100 150 200\n";
        file << "120 180 220\n";
        file << "-9999 160 240\n";
        file.close();
    }

    static void cleanup_files() {
        std::remove("test_district.asc");
        std::remove("test_population.asc");
        std::remove("test_invalid.asc");
        
        // Remove any additional admin level files that were created
        for (const auto& level : {"province", "region", "country", "zone"}) {
            std::string filename = "test_" + std::string(level) + ".asc";
            std::remove(filename.c_str());
        }
    }
    
    // Helper method to add an additional admin level for testing
    void add_admin_level(const std::string& level_name) {
        additional_admin_levels.push_back(level_name);
    }
};

// Specialized class for testing multi-admin level functionality
class MultiAdminLevelTest : public SpatialDataTestHelper {
protected:
    void SetUp() {
        // Add default additional admin levels for multi-level tests
        add_admin_level("province");
        add_admin_level("region");
        
        // Call parent SetUp
        SpatialDataTestHelper::SetUp();
    }
};

#endif 