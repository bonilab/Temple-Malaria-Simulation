#include "AdminLevelManager.h"
#include "Model.h"
#include "Core/Config/Config.h"
#include <set>
#include <stdexcept>

int AdminLevelManager::register_level(const std::string& name, const std::string& description) {
    // Check if name already exists
    if (has_level(name)) {
        throw std::runtime_error("Administrative level '" + name + "' already exists");
    }

    // Register the new level
    int id = id_to_name.size();
    id_to_name.push_back(name);
    name_to_id[name] = id;
    boundaries.emplace_back();
    boundaries[id].description = description;

    // Track if this is the district level
    if (name == "district") {
        has_district_ = true;
    }
  
    return id;
}

void AdminLevelManager::setup_boundary(const std::string& name, std::unique_ptr<AscFile> raster) {
    
    auto it = name_to_id.find(name);
    if (it == name_to_id.end()) {
        throw std::runtime_error("Administrative level '" + name + "' not registered");
    }
    // Validate raster
    if (!raster) {
        throw std::runtime_error("Null raster provided for '" + name + "'");
    }

    validate_raster(raster.get());

    // Verify level exists
    // Set up boundary data
    auto& boundary = boundaries[it->second];

    auto result = populate_lookup(raster.get());
    
    boundary.raster = std::move(raster);
    boundary.location_lookup = std::move(result.lookup);
    boundary.first_index = result.first_index;
    boundary.count = result.unit_count;

    VLOG(1) << "Initialized administrative level '" << name << "' with "
            << boundary.count << " units";
}

int AdminLevelManager::get_admin_unit(int location, const std::string& level_name) const {
    auto it = name_to_id.find(level_name);
    if (it == name_to_id.end()) {
        throw std::runtime_error("Administrative level '" + level_name + "' not found");
    }

    const auto& boundary = boundaries[it->second];
    if (location < 0 || location >= static_cast<int>(boundary.location_lookup.size())) {
        throw std::out_of_range("Invalid location index: " + std::to_string(location));
    }

    return boundary.location_lookup[location];
}

std::vector<int> AdminLevelManager::get_locations_in_unit(int unit_id, 
                                                        const std::string& level_name) const {
    auto it = name_to_id.find(level_name);
    if (it == name_to_id.end()) {
        throw std::runtime_error("Administrative level '" + level_name + "' not found");
    }

    const auto& boundary = boundaries[it->second];
    std::vector<int> locations;

    for (int loc = 0; loc < static_cast<int>(boundary.location_lookup.size()); loc++) {
        if (boundary.location_lookup[loc] == unit_id) {
            locations.push_back(loc);
        }
    }

    return locations;
}

const AdminLevelManager::BoundaryData* AdminLevelManager::get_boundary(
    const std::string& name) const {
    auto it = name_to_id.find(name);
    return it != name_to_id.end() ? &boundaries[it->second] : nullptr;
}

int AdminLevelManager::get_unit_count(const std::string& level_name) const {
    auto it = name_to_id.find(level_name);
    if (it == name_to_id.end()) {
        throw std::runtime_error("Administrative level '" + level_name + "' not found");
    }
    return boundaries[it->second].count;
}

void AdminLevelManager::validate() const {
    // Check if district level is configured when admin boundaries are used
    if (!id_to_name.empty() && !has_district_) {
        throw std::runtime_error("Administrative boundaries configured but missing required 'district' level");
    }

    // Validate each boundary
    for (size_t i = 0; i < boundaries.size(); i++) {
        const auto& boundary = boundaries[i];
        const auto& name = id_to_name[i];

        if (!boundary.raster) {
            throw std::runtime_error("No raster data loaded for '" + name + "'");
        }

        if (boundary.first_index != 0 && boundary.first_index != 1) {
            throw std::runtime_error("Invalid first_index for '" + name + "'");
        }

        if (boundary.count <= 0) {
            throw std::runtime_error("No units found in '" + name + "'");
        }
    }
}

LookupResult AdminLevelManager::populate_lookup(const AscFile* raster) {
    auto locations = Model::CONFIG->number_of_locations();
    LookupResult result;
    result.lookup.resize(locations);
    
    int min_unit_id = 1000000;
    int max_unit_id = -1;

    // For each location, determine its admin unit
    for (int loc = 0; loc < locations; loc++) {
        auto& coordinate = Model::CONFIG->location_db()[loc].coordinate;
        
        if (!coordinate) {
            throw std::runtime_error("Null coordinate for location " + std::to_string(loc));
        }

        int x = static_cast<int>(coordinate->latitude);
        int y = static_cast<int>(coordinate->longitude);

        if (x < 0 || x >= raster->NROWS || y < 0 || y >= raster->NCOLS) {
            throw std::runtime_error("Location " + std::to_string(loc) + 
                                   " coordinates out of bounds");
        }

        int unit_id = static_cast<int>(raster->data[x][y]);
        if (unit_id == raster->NODATA_VALUE) {
            throw std::runtime_error("Location " + std::to_string(loc) + 
                                   " falls in NODATA area");
        }

        result.lookup[loc] = unit_id;
        min_unit_id = std::min(min_unit_id, unit_id);
        max_unit_id = std::max(max_unit_id, unit_id);
    }

    if (min_unit_id != 0 && min_unit_id != 1) {
        throw std::runtime_error("Invalid raster indexing: must start at 0 or 1");
    }

    result.first_index = min_unit_id;
    result.unit_count = max_unit_id - min_unit_id + 1;
    return result;
}

void AdminLevelManager::validate_raster(const AscFile* raster) const {
    if (!raster) {
        throw std::runtime_error("Null raster provided");
    }

    if (raster->NROWS <= 0 || raster->NCOLS <= 0) {
        throw std::runtime_error("Invalid raster dimensions");
    }

    // Check if this is the first raster or verify dimensions match
    if (!boundaries.empty()) {
        // Find the first boundary with a valid raster
        const AscFile* first_raster = nullptr;
        for (const auto& boundary : boundaries) {
            if (boundary.raster) {
                first_raster = boundary.raster.get();
                break;
            }
        }

        // Compare dimensions if we found a previous raster
        if (first_raster) {
            if (first_raster->NROWS != raster->NROWS || first_raster->NCOLS != raster->NCOLS) {
                throw std::runtime_error("All raster files must have the same dimensions. " 
                    "Expected " + std::to_string(first_raster->NROWS) + "x" + 
                    std::to_string(first_raster->NCOLS) + ", got " + 
                    std::to_string(raster->NROWS) + "x" + std::to_string(raster->NCOLS));
            }
        }
    }
}
