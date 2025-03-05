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

  // Finalize setup, populate location_to_district, district_to_locations, and reset rasters
  populate_dependent_data();
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

      // std::cout << "Distance between location " << from << " and location " << to << " is " << distances[from][to] << std::endl;
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

int SpatialData::get_district_from_raster(int location) {
  // Check if location is within bounds
  if (location < 0 || location >= Model::CONFIG->number_of_locations()) {
    throw std::out_of_range(fmt::format("{} called with invalid location: {}",
                                        __FUNCTION__, location));
  }

  // Throw an error if there are no districts
  if (data[SpatialFileType::Districts] == nullptr) {
    throw std::runtime_error(
        fmt::format("{} called without district data loaded", __FUNCTION__));
  }

  // Get the coordinate of the location
  auto &coordinate = Model::CONFIG->location_db()[location].coordinate;

  // Check if coordinate is valid
  if (coordinate == nullptr) {
    throw std::runtime_error(
        fmt::format("{} called with location {} having null coordinate",
                    __FUNCTION__, location));
  }

  // Check if coordinates are within raster bounds
  auto* districts = data[SpatialFileType::Districts].get();  // Use .get()
  if (coordinate->latitude < 0 || coordinate->latitude >= districts->NROWS
      || coordinate->longitude < 0
      || coordinate->longitude >= districts->NCOLS) {
    throw std::out_of_range(fmt::format(
        "{} called with location {} having out of bounds coordinates ({}, {})",
        __FUNCTION__, location, coordinate->latitude, coordinate->longitude));
  }

  // Use the x, y to get the district id
  auto district =
      static_cast<int>(data[SpatialFileType::Districts]->data[static_cast<int>(
          coordinate->latitude)][static_cast<int>(coordinate->longitude)]);

  return district;
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
  if (node[DISTRICT_RASTER]) {
    load(node[DISTRICT_RASTER].as<std::string>(),
         SpatialData::SpatialFileType::Districts);
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
  // Check to make sure our data is OK
  std::string errors;
  if (check_catalog(errors)) {
    throw std::runtime_error(errors);
  }
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

void SpatialData::populate_dependent_data() {
  // populate the location_to_district and district_to_locations
  if (!data[SpatialFileType::Districts]) {
    location_to_district.clear();
    district_to_locations.clear();
    district_count = -1;
    min_district_id = -1;
    max_district_id = -1;
    return;
  }

  // Get a reference to the districts raster for cleaner code
  AscFile* districts_raster = data[SpatialFileType::Districts].get();

  min_district_id = std::numeric_limits<int>::max();
  max_district_id = std::numeric_limits<int>::min();

  // Perform a consistency check on the districts
  std::set<int> unique_districts;  // Use a set to count unique districts
  for (auto ndx = 0; ndx < districts_raster->NROWS; ndx++) {
    for (auto ndy = 0; ndy < districts_raster->NCOLS; ndy++) {
      auto value = districts_raster->data[ndx][ndy];
      if (value == districts_raster->NODATA_VALUE) { continue; }
      auto district_id = static_cast<int>(value);
      unique_districts.insert(district_id);
      min_district_id = std::min(min_district_id, district_id);
      max_district_id = std::max(max_district_id, district_id);
    }
  }

  // Set district count to number of unique districts
  district_count = unique_districts.size();

  // check size of unique districts
  if (unique_districts.size() != max_district_id - min_district_id + 1) {
    throw std::invalid_argument(fmt::format(
        "Expected {} districts, got {} districts with ids from {} to {}",
        max_district_id - min_district_id + 1, unique_districts.size(),
        min_district_id, max_district_id));
  }

  // Determine if we're using 0-based or 1-based indexing
  if (min_district_id == 0) {
    LOG(INFO) << "File suggests zero-based district numbering.";
  } else if (min_district_id == 1) {
    LOG(INFO) << "File suggests one-based district numbering.";
  } else {
    LOG(ERROR) << "Index of first district must be zero or one, found "
               << min_district_id;
    throw std::invalid_argument(
        "District raster must be zero-based or one-based.");
  }

  // Log information about the districts
  LOG(INFO) << fmt::format(
      "Districts loaded with {} districts (IDs from {} to {})", district_count,
      min_district_id, max_district_id);

  // Update location_to_district and prepare district_to_locations
  location_to_district.clear();
  district_to_locations.clear();
  
  if (data[SpatialFileType::Districts]) {
    // Size the vectors appropriately
    district_to_locations.resize(max_district_id + 1);
    location_to_district.reserve(Model::CONFIG->number_of_locations());

    // Single pass through locations to populate both mappings
    for (auto loc = 0; loc < Model::CONFIG->number_of_locations(); loc++) {
      auto district = get_district_from_raster(loc);
      location_to_district.push_back(district);
      district_to_locations[district].push_back(loc);
    }

    LOG(INFO) << fmt::format("location_to_district loaded with {} pixels", 
                            location_to_district.size());
    LOG(INFO) << fmt::format("district_to_locations created with size of {} for {}-based districts", 
                            district_to_locations.size(), min_district_id);
  }
}

int SpatialData::get_district(int location) {
  if (location < 0 || location >= Model::CONFIG->number_of_locations()) {
    throw std::out_of_range(fmt::format("{} called with invalid location: {}",
                                        __FUNCTION__, location));
  }
  if (location_to_district.empty()) {
    throw std::runtime_error("location_to_district not initialized");
  }
  return location_to_district[location];
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
}

const std::vector<int>& SpatialData::get_district_locations(int district) const {
  if (district_to_locations.empty()) {
    throw std::runtime_error("District to locations mapping not initialized");
  }
  
  // Direct indexing without adjustment
  if (district >= district_to_locations.size()) {
    throw std::out_of_range(fmt::format("Invalid district ID: {}", district));
  }
  
  return district_to_locations[district];
}