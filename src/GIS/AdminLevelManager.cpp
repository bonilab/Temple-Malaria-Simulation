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
  
    return id;
}

void AdminLevelManager::set_boundary(int level_id, const BoundaryData& in_boundary) {
    if (level_id < 0 || level_id >= static_cast<int>(boundaries.size())) {
        throw std::out_of_range("Invalid level ID: " + std::to_string(level_id));
    }


    auto& boundary = boundaries[level_id];
    boundary.location_to_unit = std::move(in_boundary.location_to_unit);
    boundary.unit_to_locations = std::move(in_boundary.unit_to_locations);
    boundary.min_unit_id = in_boundary.min_unit_id;
    boundary.max_unit_id = in_boundary.max_unit_id;
    boundary.unit_count = in_boundary.unit_count;

    auto name = id_to_name[level_id];
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

void AdminLevelManager::setup_boundary(const std::string& name, AscFile* raster) {
    
    auto it = name_to_id.find(name);
    if (it == name_to_id.end()) {
        throw std::runtime_error("Administrative level '" + name + "' not registered");
    }
    // Validate raster
    if (!raster) {
        throw std::runtime_error("Null raster provided for '" + name + "'");
    }


    auto result = populate_lookup(raster);

    // Set up boundary data
    set_boundary(it->second, result);
}

int AdminLevelManager::get_admin_level_id(const std::string& level_name) const {
    auto it = name_to_id.find(level_name);
    if (it == name_to_id.end()) {
        LOG(ERROR) << "Administrative level '" + level_name + "' not found";
        throw std::runtime_error("Administrative level '" + level_name + "' not found");
    }
    return it->second;
}

int AdminLevelManager::get_admin_unit(const std::string& level_name, int location) const {
    auto it = name_to_id.find(level_name);
    if (it == name_to_id.end()) {
        throw std::runtime_error("get_admin_unit: Administrative level '" + level_name + "' not found");
    }

    return get_admin_unit(it->second, location);
}

int AdminLevelManager::get_admin_unit(int level_id, int location) const {
    if (level_id < 0 || level_id >= static_cast<int>(boundaries.size())) {
        throw std::out_of_range("Invalid level ID: " + std::to_string(level_id));
    }
    if (location < 0 || location >= static_cast<int>(boundaries[level_id].location_to_unit.size())) {
        throw std::out_of_range("Invalid location ID: " + std::to_string(location));
    }
    return boundaries[level_id].location_to_unit[location];
}

const std::vector<int>& AdminLevelManager::get_locations_in_unit(const std::string& level_name, int unit_id) const {
    auto it = name_to_id.find(level_name);
    if (it == name_to_id.end()) {
        throw std::runtime_error("get_locations_in_unit: Administrative level '" + level_name + "' not found");
    }
    return get_locations_in_unit(it->second, unit_id);
}

const std::vector<int>& AdminLevelManager::get_locations_in_unit(int level_id, int unit_id) const {
    if (level_id < 0 || level_id >= static_cast<int>(boundaries.size())) {
        throw std::out_of_range("Invalid level ID: " + std::to_string(level_id));
    }
    if (unit_id < 0 || unit_id >= static_cast<int>(boundaries[level_id].unit_to_locations.size())) {
        throw std::out_of_range("Invalid unit ID: " + std::to_string(unit_id));
    }
    return boundaries[level_id].unit_to_locations[unit_id];
}

const std::pair<int,int> AdminLevelManager::get_units(const std::string& level_name) const {
    auto it = name_to_id.find(level_name);
    if (it == name_to_id.end()) {
        throw std::runtime_error("get_units: Administrative level '" + level_name + "' not found");
    }
    return {boundaries[it->second].min_unit_id, boundaries[it->second].max_unit_id};
}

const BoundaryData* AdminLevelManager::get_boundary(
    const std::string& name) const {
    auto it = name_to_id.find(name);
    return it != name_to_id.end() ? &boundaries[it->second] : nullptr;
}

int AdminLevelManager::get_unit_count(const std::string& level_name) const {
    auto it = name_to_id.find(level_name);
    if (it == name_to_id.end()) {
        throw std::runtime_error("get_unit_count: Administrative level '" + level_name + "' not found");
    }
    return get_unit_count(it->second);
}

int AdminLevelManager::get_unit_count(int level_id) const {
    if (level_id < 0 || level_id >= static_cast<int>(boundaries.size())) {
        VLOG(1) << "Invalid level ID: " << level_id << " for " << id_to_name[level_id];
        VLOG(1) << "Boundaries size: " << boundaries.size();
        throw std::out_of_range("Invalid level ID: " + std::to_string(level_id));
    }
    return boundaries[level_id].unit_count;
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

void AdminLevelManager::validate() const {
    // Validate each boundary in id_to_name
    for (int i = 0; i < id_to_name.size(); i++) {
        if (boundaries[i].unit_count == 0) {
            throw std::runtime_error("Administrative level '" + id_to_name[i] + "' has no units");
        }
    }

    // all admin levels must have the same dimensions for location_to_unit
    for (int i = 0; i < boundaries.size(); i++) {
        if (boundaries[i].location_to_unit.size() != boundaries[0].location_to_unit.size()) {
            throw std::runtime_error("All admin levels must have the same dimensions for location_to_unit");
        }
        // each unit_to_locations must have the size of max_unit_id + 1
        if (boundaries[i].unit_to_locations.size() != boundaries[i].max_unit_id + 1) {
            throw std::runtime_error("unit_to_locations must have the size of max_unit_id + 1");
        }

        // each unit_count should be max_unit_id - min_unit_id + 1
        if (boundaries[i].unit_count != boundaries[i].max_unit_id - boundaries[i].min_unit_id + 1) {
            throw std::runtime_error("unit_count should be max_unit_id - min_unit_id + 1");
        }
    }
}