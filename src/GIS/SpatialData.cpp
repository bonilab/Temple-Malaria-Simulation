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
  if (!has_raster()) { return true; }

  for (const auto &raster : data) {
    if (!raster) { continue; }

    auto ref_raster_info = RasterInformation();
    ref_raster_info.number_columns = raster->NCOLS;
    ref_raster_info.number_rows = raster->NROWS;
    ref_raster_info.x_lower_left_corner = raster->XLLCORNER;
    ref_raster_info.y_lower_left_corner = raster->YLLCORNER;
    ref_raster_info.cellsize = raster->CELLSIZE;

    if (!validate_raster_info(ref_raster_info, errors)) {
      errors = fmt::format("Header mismatch: {}", errors);
      LOG(ERROR) << errors;
      return true;
    }
  }

  return false;
}

void SpatialData::generate_distances() const {
  auto &db = Model::CONFIG->location_db();
  auto &distances = Model::CONFIG->spatial_distance_matrix();

  auto locations = db.size();
  distances.resize(static_cast<unsigned long>(locations));
  for (std::size_t from = 0; from < locations; from++) {
    distances[from].resize(static_cast<unsigned long long int>(locations));
    for (std::size_t to = 0; to < locations; to++) {
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

  VLOG(1) << "Updated Euclidean distances using raster data";
}

void SpatialData::generate_locations() {
  // Find first available raster file to use as reference
  AscFile* reference = nullptr;
  SpatialFileType reference_type = SpatialFileType::Count;

  for (int type = 0; type < SpatialFileType::Count; ++type) {
    if (data[type]) {
      reference = data[type].get();
      reference_type = static_cast<SpatialFileType>(type);
      break;
    }
  }

  // Validate we found a reference raster
  if (!reference) {
    throw std::runtime_error(
        "No spatial raster files available to generate locations");
  }

  // VLOG(2) << fmt::format("Using {} raster as reference for location
  // generation",
  //                       reference_type);

  // Pre-allocate the location database
  auto &db = Model::CONFIG->location_db();
  db.clear();

  // Calculate maximum possible size (all cells valid)
  const size_t max_size = static_cast<size_t>(reference->NROWS)
                          * static_cast<size_t>(reference->NCOLS);
  db.reserve(max_size);

  // Generate locations for valid cells
  int location_id = 0;
  const float no_data = reference->NODATA_VALUE;

  for (int row = 0; row < reference->NROWS; ++row) {
    for (int col = 0; col < reference->NCOLS; ++col) {
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
        fmt::format("No valid locations found in reference raster (type: {})",
                    static_cast<int>(reference_type)));
  }

  VLOG(1) << fmt::format("Generated {} locations from ({:.1f}% of cells)",
                         location_count, 100.0 * location_count / max_size);
}

// NOTE: this function return distrct_id in NON_ZERO based index given a
// location
int SpatialData::get_raster_district(int location) {
  // Throw an error if there are no districts
  if (data[SpatialFileType::Districts] == nullptr) {
    throw std::runtime_error(
        fmt::format("{} called without district data loaded", __FUNCTION__));
  }

  // Get the coordinate of the location
  auto &coordinate = Model::CONFIG->location_db()[location].coordinate;

  // Use the x, y to get the district id
  auto district =
      (int)data[SpatialFileType::Districts]
          ->data[(int)coordinate->latitude][(int)coordinate->longitude];
  return district;
}

int SpatialData::get_district(int location) {
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

  // If it's a NODATA value, return it without adjusting
  if (district == data[SpatialFileType::Districts]->NODATA_VALUE) {
    return district;
  }

  return district - first_district;
}

int SpatialData::get_district_count() { return district_count; }

std::vector<int> SpatialData::get_district_locations(int district) {
  // Throw an error if there are no districts
  if (data[SpatialFileType::Districts] == nullptr) {
    throw std::runtime_error(
        fmt::format("{} called without district data loaded", __FUNCTION__));
  }

  // Prepare our data
  std::vector<int> locations;
  AscFile* reference = data[SpatialFileType::Districts].get();  // Use .get()

  // Scan the district raster and use it to generate the location ids, the logic
  // here is the same as the generation of the location ids in
  // generate_locations
  auto id = -1;
  for (auto ndx = 0; ndx < reference->NROWS; ndx++) {
    for (auto ndy = 0; ndy < reference->NCOLS; ndy++) {
      if (reference->data[ndx][ndy] == reference->NODATA_VALUE) { continue; }
      id++;
      if ((int)reference->data[ndx][ndy] == district) {
        locations.emplace_back(id);
      }
    }
  }

  // Return the results
  return locations;
}

int SpatialData::get_first_district() { return first_district; }

SpatialData::RasterInformation SpatialData::get_raster_header() {
  return raster_info;
}

bool SpatialData::has_raster() {
  return std::any_of(data.begin(), data.end(),
                     [](const auto &raster) { return raster != nullptr; });
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
  if (node[LOCATION_RASTER]) {
    load(node[LOCATION_RASTER].as<std::string>(),
         SpatialData::SpatialFileType::Locations);
  }
  if (node[BETA_RASTER]) {
    load(node[BETA_RASTER].as<std::string>(),
         SpatialData::SpatialFileType::Beta);
  }
  if (node[POPULATION_RASTER]) {
    load(node[POPULATION_RASTER].as<std::string>(),
         SpatialData::SpatialFileType::Population);
  }
  if (node[DISTRICT_RASTER]) {
    load(node[DISTRICT_RASTER].as<std::string>(),
         SpatialData::SpatialFileType::Districts);
  }
  if (node[TRAVEL_RASTER]) {
    load(node[TRAVEL_RASTER].as<std::string>(),
         SpatialData::SpatialFileType::Travel);
  }
  if (node[ECOCLIMATIC_RASTER]) {
    load(node[ECOCLIMATIC_RASTER].as<std::string>(),
         SpatialData::SpatialFileType::Ecoclimatic);
  }
  if (node[TREATMENT_RATE_UNDER5]) {
    load(node[TREATMENT_RATE_UNDER5].as<std::string>(),
         SpatialData::SpatialFileType::PrTreatmentUnder5);
  }
  if (node[TREATMENT_RATE_OVER5]) {
    load(node[TREATMENT_RATE_OVER5].as<std::string>(),
         SpatialData::SpatialFileType::PrTreatmentOver5);
  }

  // Check to make sure our data is OK
  std::string errors;
  if (check_catalog(errors)) {
    throw std::runtime_error(errors);
  }
}

bool SpatialData::parse(const YAML::Node &node) {
  // Validate required configuration
  if (!node["cell_size"]) {
    throw std::runtime_error("Missing required 'cell_size' configuration");
  }
  cell_size = node["cell_size"].as<float>();

  // Load and validate raster files
  load_files(node);

  // We have data, and we know that it should be located in the same geographic
  // space, so now we can now refresh the location_db
  if (Model::CONFIG->number_of_locations() == 0) {
    LOG(INFO) << "Location database is empty, generating locations using first "
                 "available raster.";
    generate_locations();
  }

  // Load the age distribution data from the YAML file (not provided by raster)
  load_age_distribution(node);

  // Load location-specific data from raster or YAML
  load_location_data(node);
  load_treatment_data(node);

  // Finalize setup, populate district lookup and reset rasters
  populate_dependent_data();
  parse_complete();
  return true;
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
  // populate the district lookup
  if (!data[SpatialFileType::Districts]) {
    district_lookup_.clear();
    district_count = -1;
    first_district = -1;
    return;
  }

  // Get a reference to the districts raster for cleaner code
  AscFile* districts_raster = data[SpatialFileType::Districts].get();

  int min_district_id = std::numeric_limits<int>::max();
  int max_district_id = std::numeric_limits<int>::min();

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

  // Sort the districts to check indexing
  std::vector<int> districts(unique_districts.begin(), unique_districts.end());
  std::sort(districts.begin(), districts.end());

  // Determine if we're using 0-based or 1-based indexing
  if (districts.front() == 0) {
    first_district = 0;
    LOG(INFO) << "File suggests zero-based district numbering.";
  } else if (districts.front() == 1) {
    first_district = 1;
    LOG(INFO) << "File suggests one-based district numbering.";
  } else {
    LOG(ERROR) << "Index of first district must be zero or one, found "
               << districts.front();
    throw std::invalid_argument(
        "District raster must be zero-based or one-based.");
  }

  // Log information about the districts
  LOG(INFO) << fmt::format(
      "Districts loaded with {} districts (IDs from {} to {})", district_count,
      districts.front(), districts.back());

  // district_lookup must be populated after populate the first_district and
  // district_count
  district_lookup_.clear();
  for (auto loc = 0; loc < Model::CONFIG->number_of_locations(); loc++) {
    district_lookup_.emplace_back(
        SpatialData::get_instance().get_district(loc));
  }
  LOG(INFO) << fmt::format("District_lookup loaded with {} pixels",
                           district_lookup_.size());
}

void SpatialData::parse_complete() {
  // Simply reset unique_ptrs instead of manual delete
  data[SpatialFileType::Beta].reset();
  data[SpatialFileType::Population].reset();
  data[SpatialFileType::PrTreatmentUnder5].reset();
  data[SpatialFileType::PrTreatmentOver5].reset();
}

void SpatialData::write(const std::string &filename, SpatialFileType type) {
  // Check to make sure there is something to write
  if (!data[type]) {  // Check if unique_ptr contains object
    throw std::runtime_error(
        fmt::format("No data for spatial file type {}, write file {}",
                    static_cast<uint32_t>(type), filename));
  }

  // Write the data
  AscFileManager::write(data[type].get(), filename);
}
