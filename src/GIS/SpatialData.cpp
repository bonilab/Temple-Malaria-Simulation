/*
 * SpatialData.cpp
 *
 * Implementation of SpatialData functions.
 */
#include "SpatialData.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <cmath>
#include <stdexcept>

#include "Core/Config/Config.h"
#include "Model.h"
#include "easylogging++.h"

SpatialData::SpatialData() = default;  // Array is zero-initialized by default

SpatialData::~SpatialData() = default;  // Let unique_ptr handle cleanup

bool SpatialData::parse(const YAML::Node &node) {
  // Validate required configuration
  if (!node["cell_size"]) {
    throw std::runtime_error("Missing required 'cell_size' configuration");
  }
  cell_size = node["cell_size"].as<float>();

  // Load and validate raster files
  load_files(node);

  if (Model::CONFIG->number_of_locations() != 0) {
    throw std::runtime_error("Location database is not empty");
  }

   // find first raster that is not nullptr
  auto first_raster = std::find_if(data.begin(), data.end(), [](const auto& raster) { return raster != nullptr; });
  if (first_raster == data.end()) {
    throw std::runtime_error("No raster files found");
  }

  LOG(INFO) << "Location database is empty, generating locations using first "
                "available raster.";
  generate_locations(first_raster->get());
  generate_distances();

  // Load the age distribution data from the YAML file (not provided by raster)
  load_age_distribution(node);

  // Load location-specific data from raster or YAML
  load_location_data(node);
  load_treatment_data(node);

  // Finalize setup, populate location_to_district, district_to_locations
  initialize_admin_boundaries();

  // Reset rasters
  parse_complete();
  return true;
}

bool SpatialData::validate_raster_info(const RasterInformation &new_info,
                                       std::string &errors) {
  // If raster_info isn't initialized yet, store the new info
  if (!raster_info.is_initialized()) {
    raster_info = new_info;
    return true;
  }

  // Otherwise validate that the new info matches
  if (!raster_info.matches(new_info)) {
    // Use a vector to store error messages
    std::vector<std::string> error_messages;

    if (new_info.number_columns != raster_info.number_columns) {
      error_messages.push_back("mismatched number of columns");
    }
    if (new_info.number_rows != raster_info.number_rows) {
      error_messages.push_back("mismatched number of rows");
    }
    if (new_info.x_lower_left_corner != raster_info.x_lower_left_corner) {
      error_messages.push_back("mismatched x lower left corner");
    }
    if (new_info.y_lower_left_corner != raster_info.y_lower_left_corner) {
      error_messages.push_back("mismatched y lower left corner");
    }
    if (new_info.cellsize != raster_info.cellsize) {
      error_messages.push_back("mismatched cell size");
    }

    // Join all error messages with semicolons
    errors = fmt::format("{};", fmt::join(error_messages, ";"));
    return false;
  }
  return true;
}

bool SpatialData::check_catalog(std::string &errors) {
  if (!using_raster) { return true; }

  for (const auto &raster : data) {
    if (!raster) { continue; }

    auto ref_raster_info = RasterInformation();
    ref_raster_info.number_columns = raster->NCOLS;
    ref_raster_info.number_rows = raster->NROWS;
    ref_raster_info.x_lower_left_corner = raster->XLLCORNER;
    ref_raster_info.y_lower_left_corner = raster->YLLCORNER;
    ref_raster_info.cellsize = raster->CELLSIZE;
    ref_raster_info.no_data_value = raster->NODATA_VALUE;

    if (!validate_raster_info(ref_raster_info, errors)) {
      errors = fmt::format("Header mismatch: {}", errors);
      LOG(ERROR) << errors;
      return true;
    }
  }

  //check for all rasters have the same no_data cell locations
  AscFile* ref_raster = nullptr;
  for (const auto &raster : data) {
    if (!raster) { continue; }
    if (ref_raster == nullptr) {
      ref_raster = raster.get();
      continue;
    }

    for (int row = 0; row < raster->NROWS; row++) {
      for (int col = 0; col < raster->NCOLS; col++) {
        if (raster->data[row][col] == raster->NODATA_VALUE) {
          if (ref_raster->data[row][col] != raster->NODATA_VALUE) { 
            errors = fmt::format("NODATA_VALUE mismatch: {}", raster->NODATA_VALUE);
            LOG(ERROR) << errors;
            return true;
          }
        }
      }
    }
  }

  return false;
}

void SpatialData::generate_distances() const {
  auto &db = Model::CONFIG->location_db();
  auto &distances = Model::CONFIG->spatial_distance_matrix();

  auto number_of_locations = db.size();
  if (number_of_locations == 0) {
    throw std::runtime_error("No locations found in location database");
  }
  
  distances.resize(static_cast<unsigned long>(number_of_locations));
  for (std::size_t from = 0; from < number_of_locations; from++) {
    distances[from].resize(static_cast<unsigned long long int>(number_of_locations));
    for (std::size_t to = 0; to < number_of_locations; to++) {
      distances[from][to] =
          std::sqrt(std::pow(cell_size
                                 * (db[from].coordinate->latitude
                                    - db[to].coordinate->latitude),
                             2)
                    + std::pow(cell_size
                                   * (db[from].coordinate->longitude
                                      - db[to].coordinate->longitude),
                               2));

    }
  }

  LOG(INFO) << "Updated Euclidean distances using raster data";
}

void SpatialData::generate_locations(AscFile* reference) {

  // Validate we found a reference raster
  if (!reference) {
    throw std::runtime_error(
        "No spatial raster files available to generate locations");
  }

  // Using Raster Information to generate locations
  if (raster_info.is_initialized()) {
    VLOG(1) << "Using Raster Information to generate locations";
  } else {
    throw std::runtime_error("Raster Information is not initialized");
  }

  // Pre-allocate the location database
  auto &db = Model::CONFIG->location_db();
  db.clear();

  // Calculate maximum possible size (all cells valid)
  const size_t max_size = static_cast<size_t>(raster_info.number_rows)
                          * static_cast<size_t>(raster_info.number_columns);
  db.reserve(max_size);

  // Generate locations for valid cells
  int location_id = 0;
  const float no_data = raster_info.no_data_value;

  for (int row = 0; row < raster_info.number_rows; ++row) {
    for (int col = 0; col < raster_info.number_columns; ++col) {
      if (reference->data[row][col] == no_data) { continue; }
      db.emplace_back(location_id++, row, col, 0);
    }
  }

  // Reclaim excess memory
  db.shrink_to_fit();

  // Update and validate location count
  Model::CONFIG->number_of_locations.set_value();
  const auto location_count = Model::CONFIG->number_of_locations();

  if (location_count == 0) {
    throw std::runtime_error(
        fmt::format("No valid locations found in raster"));
  }
  auto no_data_count = max_size - location_count;
  LOG(INFO) << fmt::format("Generated {} locations from {} total cells, {} cells with no data",
                         location_count, max_size, no_data_count);
}

SpatialData::RasterInformation SpatialData::get_raster_header() {
  return raster_info;
}

void SpatialData::load(const std::string &filename, SpatialFileType type) {
  // No need to check and delete, unique_ptr handles it
  VLOG(1) << "Loading " << filename;
  data[type] = std::unique_ptr<AscFile>(AscFileManager::read(filename));
}

void SpatialData::copy_raster_to_location_db(SpatialFileType type) {
  // Verify that the raster data has been loaded
  if (!data[type]) {
    throw std::runtime_error(
        fmt::format("{} called without raster, type id: {}", __FUNCTION__,
                    static_cast<uint32_t>(type)));
  }

  // Get a reference to the raster for cleaner code
  AscFile* raster = data[type].get();

  // Get a reference to the location database
  auto &db = Model::CONFIG->location_db();
  auto count = Model::CONFIG->number_of_locations();

  // Scan the data and update the values
  auto id = -1;
  for (auto ndx = 0; ndx < raster->NROWS; ndx++) {
    for (auto ndy = 0; ndy < raster->NCOLS; ndy++) {
      if (raster->data[ndx][ndy] == raster->NODATA_VALUE) { continue; }
      id++;

      // Verify that we haven't exceeded our bounds
      if (id >= count) {
        throw std::runtime_error(
            fmt::format("{} found more locations than expected", __FUNCTION__));
      }

      // Update the appropriate value
      switch (type) {
        case SpatialFileType::Beta:
          db[id].beta = raster->data[ndx][ndy];
          break;
        case SpatialFileType::Population:
          db[id].population_size = static_cast<int>(raster->data[ndx][ndy]);
          break;
        case SpatialFileType::PrTreatmentUnder5:
          db[id].p_treatment_less_than_5 = raster->data[ndx][ndy];
          break;
        case SpatialFileType::PrTreatmentOver5:
          db[id].p_treatment_more_than_5 = raster->data[ndx][ndy];
          break;
        default:
          break;
      }
    }
  }
}

void SpatialData::load_files(const YAML::Node &node) {
  using_raster = false;  // Reset flag at start
  
  if (node[LOCATION_RASTER]) {
    load(node[LOCATION_RASTER].as<std::string>(),
         SpatialData::SpatialFileType::Locations);
    using_raster = true;
  }
  if (node[BETA_RASTER]) {
    load(node[BETA_RASTER].as<std::string>(),
         SpatialData::SpatialFileType::Beta);
    using_raster = true;
  }
  if (node[POPULATION_RASTER]) {
    load(node[POPULATION_RASTER].as<std::string>(),
         SpatialData::SpatialFileType::Population);
    using_raster = true;
  }
  if (node[TRAVEL_RASTER]) {
    load(node[TRAVEL_RASTER].as<std::string>(),
         SpatialData::SpatialFileType::Travel);
    using_raster = true;
  }
  if (node[ECOCLIMATIC_RASTER]) {
    load(node[ECOCLIMATIC_RASTER].as<std::string>(),
         SpatialData::SpatialFileType::Ecoclimatic);
    using_raster = true;
  }
  if (node[TREATMENT_RATE_UNDER5]) {
    load(node[TREATMENT_RATE_UNDER5].as<std::string>(),
         SpatialData::SpatialFileType::PrTreatmentUnder5);
    using_raster = true;
  }
  if (node[TREATMENT_RATE_OVER5]) {
    load(node[TREATMENT_RATE_OVER5].as<std::string>(),
         SpatialData::SpatialFileType::PrTreatmentOver5);
    using_raster = true;
  }

  if (node[DISTRICT_RASTER]) {
    load(node[DISTRICT_RASTER].as<std::string>(),
         SpatialData::SpatialFileType::Districts);
    using_raster = true;
  }

  // Add support for the new administrative_boundaries section
  if (node[ADMIN_BOUNDARIES]) {
    load_admin_boundaries(node);
  }

  // Check to make sure our data is OK
  std::string errors;
  if (check_catalog(errors)) {
    throw std::runtime_error(errors);
  }
}

void SpatialData::load_admin_boundaries(const YAML::Node &node) {
  if (!node[ADMIN_BOUNDARIES].IsSequence()) {
    throw std::runtime_error("administrative_boundaries must be a sequence");
  }
  
  admin_rasters.clear();
  
  // Process each administrative boundary
  for (const auto& admin_level : node[ADMIN_BOUNDARIES]) {
    if (!admin_level["name"] || !admin_level["raster"]) {
      throw std::runtime_error("Each administrative level must have a name and raster path");
    }
    
    std::string level_name = admin_level["name"].as<std::string>();
    std::string raster_path = admin_level["raster"].as<std::string>();
    
    // Store for processing later
    admin_rasters[level_name] = raster_path;
    LOG(INFO) << "Found admin level: " << level_name << " with raster: " << raster_path;
  }
  
  using_raster = true;
}

void SpatialData::load_age_distribution(const YAML::Node &node) {
  if (!node["age_distribution_by_location"]) {
    throw std::runtime_error("Missing required age distribution data");
  }

  auto &location_db = Model::CONFIG->location_db();
  auto number_of_locations = Model::CONFIG->number_of_locations();

  for (auto loc = 0ul; loc < number_of_locations; loc++) {
    auto input_loc =
        node["age_distribution_by_location"].size() < number_of_locations ? 0
                                                                          : loc;
    auto &age_dist = node["age_distribution_by_location"][input_loc];

    if (!age_dist.IsSequence()) {
      throw std::runtime_error("Age distribution must be a sequence");
    }

    location_db[loc].age_distribution.clear();
    for (const auto &value : age_dist) {
      location_db[loc].age_distribution.push_back(value.as<double>());
    }
  }
}

void SpatialData::load_treatment_data(const YAML::Node &node) {
  auto &location_db = Model::CONFIG->location_db();
  auto number_of_locations = Model::CONFIG->number_of_locations();

  // Only load from YAML if raster not provided
  if (data[SpatialFileType::PrTreatmentUnder5] != nullptr) {
    copy_raster_to_location_db(SpatialFileType::PrTreatmentUnder5);
  } else {
    if (!node["p_treatment_for_less_than_5_by_location"]) {
      throw std::runtime_error("Missing treatment rate data for under 5");
    }
    for (auto loc = 0ul; loc < number_of_locations; loc++) {
      auto input_loc = node["p_treatment_for_less_than_5_by_location"].size()
                               < number_of_locations
                           ? 0
                           : loc;
      location_db[loc].p_treatment_less_than_5 =
          node["p_treatment_for_less_than_5_by_location"][input_loc]
              .as<float>();
    }
  }

  if (data[SpatialFileType::PrTreatmentOver5] != nullptr) {
    copy_raster_to_location_db(SpatialFileType::PrTreatmentOver5);
  } else {
    if (!node["p_treatment_for_more_than_5_by_location"]) {
      throw std::runtime_error("Missing treatment rate data for over 5");
    }
    for (auto loc = 0ul; loc < number_of_locations; loc++) {
      auto input_loc = node["p_treatment_for_more_than_5_by_location"].size()
                               < number_of_locations
                           ? 0
                           : loc;
      location_db[loc].p_treatment_more_than_5 =
          node["p_treatment_for_more_than_5_by_location"][input_loc]
              .as<float>();
    }
  }
}

void SpatialData::load_location_data(const YAML::Node &node) {
  auto &location_db = Model::CONFIG->location_db();
  auto number_of_locations = Model::CONFIG->number_of_locations();

  if (data[SpatialFileType::Beta] != nullptr) {
    copy_raster_to_location_db(SpatialFileType::Beta);
  } else {
    if (!node["beta_by_location"]) {
      throw std::runtime_error("Missing beta data");
    }
    for (auto loc = 0ul; loc < number_of_locations; loc++) {
      auto input_loc =
          node["beta_by_location"].size() < number_of_locations ? 0 : loc;
      location_db[loc].beta = node["beta_by_location"][input_loc].as<float>();
    }
  }
  if (data[SpatialFileType::Population] != nullptr) {
    copy_raster_to_location_db(SpatialFileType::Population);
  } else {
    if (!node["population_size_by_location"]) {
      throw std::runtime_error("Missing population data");
    }
    for (auto loc = 0ul; loc < number_of_locations; loc++) {
      auto input_loc =
          node["population_size_by_location"].size() < number_of_locations
              ? 0
              : loc;
      location_db[loc].population_size =
          node["population_size_by_location"][input_loc].as<int>();
    }
  }
}

void SpatialData::initialize_admin_boundaries() {
    // Create a new AdminLevelManager
    admin_manager_ = std::make_unique<AdminLevelManager>();
    
    if (!using_raster) {
      return;
    }
    if (admin_rasters.empty()) {
      // there will be cases where we don't need to have any admin levels
      return;
    }
    
    // Now process all other admin levels
    for (const auto& [level_name, raster_path] : admin_rasters) {
        try {
            admin_manager_->register_level(level_name);
            
            // Load the raster
            auto raster = std::unique_ptr<AscFile>(AscFileManager::read(raster_path));
            admin_manager_->setup_boundary(level_name, raster.get());
            
            LOG(INFO) << "Initialized admin level: " << level_name;
        } catch (const std::exception& e) {
            LOG(ERROR) << "Failed to initialize admin level " << level_name << ": " << e.what();
            throw;
        }
    }
    
    // Validate the configuration
    try {
        admin_manager_->validate();
    } catch (const std::exception& e) {
        LOG(ERROR) << "AdminLevelManager validation failed: " << e.what();
        throw std::runtime_error(e.what());
    }

    LOG(INFO) << "Administrative boundaries initialized successfully";
} 

void SpatialData::parse_complete() {
  // Simply reset unique_ptrs instead of manual delete
  // some rasters are not reset because they are used by other components for initialization
  // i.e: SeasonalImmunity reporter requires the Ecoclimatic raster
  // SeasonalEquation reporter requires the Ecoclimatic raster
  // SpatialModel requires the Travel raster
  // ...
  
  data[SpatialFileType::Beta].reset();
  data[SpatialFileType::Population].reset();
  data[SpatialFileType::PrTreatmentUnder5].reset();
  data[SpatialFileType::PrTreatmentOver5].reset();
  
  // Note: We don't reset Districts raster as ownership might have been transferred
  // Similarly, we don't need to reset any admin rasters as they are stored in custom objects
  
  // Clean up the temporary admin rasters map as it's no longer needed
  admin_rasters.clear();
}