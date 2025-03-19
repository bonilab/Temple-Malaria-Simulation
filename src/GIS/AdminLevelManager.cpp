#include "AdminLevelManager.h"
#include "Model.h"
#include "Core/Config/Config.h"
#include <set>
#include <stdexcept>
#include <unordered_map>

int AdminLevelManager::register_level(const std::string& name) {
    // Check if name already exists
    if (has_level(name)) {
        throw std::runtime_error("Administrative level '" + name + "' already exists");
    }

    // Register the new level
    int id = id_to_name.size();
    id_to_name.push_back(name);
    name_to_id[name] = id;
    boundaries.emplace_back();

    // Track if this is the district level
    if (name == "district") {
        has_district_ = true;
    }
  
    return id;
}

void AdminLevelManager::setup_boundary(const std::string& name, AscFile* raster) {
    
    auto it = name_to_id.find(name);
    if (it == name_to_id.end()) {
        throw std::runtime_error("Administrative level '" + name + "' not registered");
    }
    // Validate raster
    if (!raster) {
        throw std::runtime_error("Null raster provided for '" + name + "'");
    }

    // Set up boundary data
    auto& boundary = boundaries[it->second];

    auto result = populate_lookup(raster);
    
    boundary.location_to_unit = std::move(result.location_to_unit);
    boundary.unit_to_locations = std::move(result.unit_to_locations);
    boundary.min_unit_id = result.min_unit_id;
    boundary.max_unit_id = result.max_unit_id;
    boundary.unit_count = result.unit_count;

    // Validate the boundary data
    if (boundary.unit_count == 0) {
        throw std::runtime_error("Administrative level '" + name + "' has no units");
    }

    if (boundary.min_unit_id !=0 && boundary.min_unit_id != 1) {
        throw std::runtime_error("Administrative level '" + name + "' has invalid minimum unit ID of 0 or 1");
    }

    if (boundary.unit_count != boundary.max_unit_id - boundary.min_unit_id + 1) {
        throw std::runtime_error("Administrative level '" + name + "' has invalid unit ID range");
    }

    VLOG(1) << "Initialized administrative level '" << name << "' with "
            << boundary.unit_count << " units";
}

int AdminLevelManager::get_admin_unit(const std::string& level_name, int location) const {
    auto it = name_to_id.find(level_name);
    if (it == name_to_id.end()) {
        throw std::runtime_error("Administrative level '" + level_name + "' not found");
    }

    const auto& boundary = boundaries[it->second];
    if (location < 0 || location >= static_cast<int>(boundary.location_to_unit.size())) {
        throw std::out_of_range("Invalid location index: " + std::to_string(location));
    }

    return boundary.location_to_unit[location];
}

const std::vector<int>& AdminLevelManager::get_locations_in_unit(const std::string& level_name, int unit_id) const {
    auto it = name_to_id.find(level_name);
    if (it == name_to_id.end()) {
        throw std::runtime_error("Administrative level '" + level_name + "' not found");
    }

    const auto& boundary = boundaries[it->second];

    if (unit_id < 0 || unit_id >= static_cast<int>(boundary.unit_to_locations.size())) {
        throw std::out_of_range("Invalid unit ID: " + std::to_string(unit_id));
    }

    return boundary.unit_to_locations[unit_id];
}

const BoundaryData* AdminLevelManager::get_boundary(
    const std::string& name) const {
    auto it = name_to_id.find(name);
    return it != name_to_id.end() ? &boundaries[it->second] : nullptr;
}

int AdminLevelManager::get_unit_count(const std::string& level_name) const {
    auto it = name_to_id.find(level_name);
    if (it == name_to_id.end()) {
        throw std::runtime_error("Administrative level '" + level_name + "' not found");
    }
    return boundaries[it->second].unit_count;
}

void AdminLevelManager::validate() const {
    // Check if district level is configured when admin boundaries are used
    if (!id_to_name.empty() && !has_district_) {
        throw std::runtime_error("Administrative boundaries configured but missing required 'district' level");
    }

    // Validate each boundary in id_to_name
    for (int i = 0; i < id_to_name.size(); i++) {
        if (boundaries[i].unit_count == 0) {
            throw std::runtime_error("Administrative level '" + id_to_name[i] + "' has no units");
        }
    }

    // Check if all boundaries have the same dimensions
    if (boundaries.size() > 1) {
        for (int i = 1; i < boundaries.size(); i++) {
            if (boundaries[i].unit_count != boundaries[0].unit_count) {
                throw std::runtime_error("All boundaries must have the same dimensions.");
            }
        }
    }
}

BoundaryData AdminLevelManager::populate_lookup(const AscFile* raster) {
    BoundaryData result;

    auto min_unit_id = std::numeric_limits<int>::max();
    auto max_unit_id = std::numeric_limits<int>::min();

    // Perform a consistency check on the districts
    std::set<int> unique_unit_id;

    auto location_count = 0;
    for (auto ndx = 0; ndx < raster->NROWS; ndx++) {
        for (auto ndy = 0; ndy < raster->NCOLS; ndy++) {
            auto value = raster->data[ndx][ndy];
            if (value == raster->NODATA_VALUE) { continue; }
            auto unit_id = static_cast<int>(value);
            unique_unit_id.insert(unit_id);
            min_unit_id = std::min(min_unit_id, unit_id);
            max_unit_id = std::max(max_unit_id, unit_id);
            location_count++;
        }
    } 
    
    if (unique_unit_id.size() != max_unit_id - min_unit_id + 1) {
        throw std::runtime_error("Invalid unit ID range in raster");
    }

    result.location_to_unit.clear();
    result.location_to_unit.resize(location_count);
    result.unit_to_locations.clear();
    result.unit_to_locations.resize(max_unit_id + 1); // handle both 0 and 1 based indexing

    auto location_id =0;
    for (auto ndx = 0; ndx < raster->NROWS; ndx++) {
        for (auto ndy = 0; ndy < raster->NCOLS; ndy++) {
            auto value = raster->data[ndx][ndy];
            if (value == raster->NODATA_VALUE) { continue; }
            auto unit_id = static_cast<int>(value);
            result.location_to_unit[location_id] = unit_id;
            result.unit_to_locations[unit_id].push_back(location_id);
            location_id++;
        }
    }
    
    result.min_unit_id = min_unit_id;
    result.max_unit_id = max_unit_id;
    result.unit_count = unique_unit_id.size();

    return result;
}

void AdminLevelManager::validate_raster(const AscFile* raster) const {
    if (!raster) {
        throw std::runtime_error("Null raster provided");
    }

    if (raster->NROWS <= 0 || raster->NCOLS <= 0) {
        throw std::runtime_error("Invalid raster dimensions");
    }
    // TODO: check if the raster is a valid admin level raster

}
