#ifndef ADMIN_LEVEL_MANAGER_TEST_HELPERS_H
#define ADMIN_LEVEL_MANAGER_TEST_HELPERS_H

#include <catch2/catch_test_macros.hpp>
#include "GIS/AdminLevelManager.h"
#include "Core/Config/Config.h"
#include "Model.h"
#include "Core/Random.h"
#include "Population/Population.h"
#include "Spatial/Location.h"
#include <fstream>
#include <memory>

class AdminLevelManagerTestHelper {
protected:
    AdminLevelManager manager;
    
    void SetUp() {
        // Set up any necessary test environment
        cleanup_files();
        
        if (Model::CONFIG == nullptr) {
            Model::CONFIG = new Config();
            
            // Set up minimal location database
            Model::CONFIG->number_of_locations() = 9;  // 3x3 grid
            auto& db = Model::CONFIG->location_db();
            db.clear();
            
            // Create locations with proper initialization
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    int loc_id = i * 3 + j;
                    float latitude = static_cast<float>(i);
                    float longitude = static_cast<float>(j);
                    // Initialize with id, lat, long, and a default population size of 1000
                    db.emplace_back(loc_id, latitude, longitude, 1000);
                }
            }
        }

        // Initialize other Model singletons if needed
        if (Model::RANDOM == nullptr) {
            // Model::RANDOM = new Random();
        }
        if (Model::POPULATION == nullptr) {
            // Model::POPULATION = new Population(Model::CONFIG);
        }
    }
    
    void TearDown() {
        cleanup_files();
        
        if (Model::POPULATION != nullptr) {
            delete Model::POPULATION;
            Model::POPULATION = nullptr;
        }
        if (Model::RANDOM != nullptr) {
            delete Model::RANDOM;
            Model::RANDOM = nullptr;
        }
        if (Model::CONFIG != nullptr) {
            delete Model::CONFIG;
            Model::CONFIG = nullptr;
        }
    }

    // Create a test raster file with specified dimensions and values
    static void create_test_raster(const std::string& filename, 
                                 bool zero_based = true,
                                 int rows = 3, 
                                 int cols = 3) {
        std::ofstream file(filename);
        file << "ncols " << cols << "\n";
        file << "nrows " << rows << "\n";
        file << "xllcorner 0.0\n";
        file << "yllcorner 0.0\n";
        file << "cellsize 1.0\n";
        file << "NODATA_value -9999\n";
        
        // Write a simple pattern: each row has increasing values
        int start_value = zero_based ? 0 : 1;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (j > 0) file << " ";
                file << (start_value + (i * cols + j) % 3);  // Ensure only 3 unique values
            }
            file << "\n";
        }
        file.close();
    }

    // Create a test raster with specific values
    static void create_custom_raster(const std::string& filename, 
                                   const std::vector<std::vector<int>>& values) {
        std::ofstream file(filename);
        file << "ncols " << values[0].size() << "\n";
        file << "nrows " << values.size() << "\n";
        file << "xllcorner 0.0\n";
        file << "yllcorner 0.0\n";
        file << "cellsize 1.0\n";
        file << "NODATA_value -9999\n";
        
        for (const auto& row : values) {
            for (size_t j = 0; j < row.size(); j++) {
                if (j > 0) file << " ";
                file << row[j];
            }
            file << "\n";
        }
        file.close();
    }

    static void cleanup_files() {
        std::remove("test_district.asc");
        std::remove("test_province.asc");
        std::remove("test_invalid.asc");
    }
};

#endif // ADMIN_LEVEL_MANAGER_TEST_HELPERS_H
