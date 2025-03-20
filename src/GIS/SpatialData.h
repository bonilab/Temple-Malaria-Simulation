/*
 * SpatialData.h
 *
 * Definitions of the thread-safe singleton pattern spatial class which manages
 * the spatial aspects of the model from a high level.
 */
#ifndef SPATIALDATA_H
#define SPATIALDATA_H

#include <yaml-cpp/yaml.h>

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <map>

#include "AscFile.h"
#include "Core/PropertyMacro.h"
#include "AdminLevelManager.h"

class SpatialData {
public:
  enum SpatialFileType {
    // Only use the data to define the model's location listing
    Locations,

    // Population data
    Population,

    // Transmission intensity, linked to the Entomological Inoculation Rates
    // (EIR)
    Beta,

    // District location
    Districts,

    // Travel time data
    Travel,

    // Eco-climatic zones that are used for seasonal variation
    Ecoclimatic,

    // Probability of treatment, under 5
    PrTreatmentUnder5,

    // Probability of treatment, over 5
    PrTreatmentOver5,

    // Number of sequential items in the type
    Count
  };

  struct RasterInformation {
    // Flag to indicate the value has not been set yet
    static const int NOT_SET = -1;

    // The number of columns in the raster
    int number_columns = NOT_SET;

    // The number of rows in the raster
    int number_rows = NOT_SET;

    // The lower-left X coordinate of the raster
    double x_lower_left_corner = NOT_SET;

    // The lower-left Y coordinate of the raster
    double y_lower_left_corner = NOT_SET;

    // The size of the cell, typically in meters
    double cellsize = NOT_SET;

    double no_data_value = NOT_SET;

    // Validate if the raster information matches another instance
    bool matches(const RasterInformation &other) const {
      return number_columns == other.number_columns
             && number_rows == other.number_rows
             && x_lower_left_corner == other.x_lower_left_corner
             && y_lower_left_corner == other.y_lower_left_corner
             && cellsize == other.cellsize
             && no_data_value == other.no_data_value;
    }

    // Check if the raster information has been initialized
    bool is_initialized() const {
      return number_columns != NOT_SET && number_rows != NOT_SET
             && x_lower_left_corner != NOT_SET && y_lower_left_corner != NOT_SET
             && cellsize != NOT_SET
             && no_data_value != NOT_SET;
    }
  };

  

  // private:
  const std::string BETA_RASTER = "beta_raster";
  const std::string DISTRICT_RASTER = "district_raster";
  const std::string LOCATION_RASTER = "location_raster";
  const std::string POPULATION_RASTER = "population_raster";
  const std::string TRAVEL_RASTER = "travel_raster";
  const std::string ECOCLIMATIC_RASTER = "ecoclimatic_raster";
  const std::string TREATMENT_RATE_UNDER5 = "pr_treatment_under5";
  const std::string TREATMENT_RATE_OVER5 = "pr_treatment_over5";

  // Add constant for the new admin boundaries configuration section
  const std::string ADMIN_BOUNDARIES = "administrative_boundaries";

  // The size of the cells in the raster, the units shouldn't matter, but this
  // was written when we were using 5x5 km cells
  float cell_size = 0;

  // Add raster_info as a data member
  RasterInformation raster_info;


  // true if any raster file has been loaded, false otherwise
  bool using_raster = false;

  // Add AdminLevelManager as a member
  std::unique_ptr<AdminLevelManager> admin_manager_{ new AdminLevelManager()};

  // Constructor
  SpatialData();

  // Deconstructor
  ~SpatialData();

  /**
   * @brief Parses spatial configuration from YAML and initializes the spatial
   * system, all reaster files must be defined in raster_db node for the check_catalog to work
   *
   * @param node YAML configuration node containing spatial settings
   * @return true if parsing was successful
   * @throws std::runtime_error if required configuration is missing or invalid
   */
  bool parse(const YAML::Node &node);

  // Check the loaded spatial catalog for errors, returns true if there are
  // errors
  bool check_catalog(std::string &errors);

  /**
   * @brief Generates location database from a reference raster file
   * @param raster The raster file to use as a reference for location generation
   *
   * This function creates location entries for each valid (non-NODATA) cell in
   * the raster. Each location is assigned:
   * - A unique sequential ID
   * - Row and column coordinates from the raster
   * - Initial elevation of 0
   *
   * @throws std::runtime_error if no valid raster files are available
   * @throws std::runtime_error if no valid locations are found in the raster
   */
  void generate_locations(AscFile* raster);

  // Load the given raster file into the spatial catalog and assign the given
  // label
  void load(const std::string &filename, SpatialFileType type);

  // Load all the spatial data from the node
  void load_files(const YAML::Node &node);

  // copy the raster to the location_db; works with betas and probability of
  // treatment
  void copy_raster_to_location_db(SpatialFileType type);

  // Perform any clean-up operations after parsing the YAML file is complete
  void parse_complete();

public:
  // Not supported by singleton.
  SpatialData(SpatialData const &) = delete;

  // Not supported by singleton.
  void operator=(SpatialData const &) = delete;

  // Get a reference to the spatial object.
  static SpatialData &get_instance() {
    static SpatialData instance;
    return instance;
  }

  // Return the raster header or the default structure if no raster are loaded
  RasterInformation get_raster_header();

  // Generate the Euclidean distances for the location_db
  void generate_distances() const;


  AdminLevelManager* get_admin_level_manager() const{
    return admin_manager_.get();
  }

  /**
   * @brief Retrieves the administrative unit ID for a given location
   * @param location The location ID
   * @param level_name The administrative level name (e.g., "district")
   * @return The administrative unit ID for the given location
   * @throws std::out_of_range if location is invalid
   * @throws std::runtime_error if admin level is not initialized
   */
  int get_admin_unit(const std::string& level_name, int location) const {
    return admin_manager_->get_admin_unit(level_name, location);
  }

  /**
   * @brief Retrieves the administrative unit ID for a given location
   * @param level_id The administrative level ID
   * @param location The location ID
   * @return The administrative unit ID for the given location
   * @throws std::out_of_range if location is invalid
   * @throws std::runtime_error if admin level is not initialized
   */
  int get_admin_unit(int level_id, int location) const {
    return admin_manager_->get_admin_unit(level_id, location);
  }

  int get_admin_level_id(const std::string& level_name) const {
    return admin_manager_->get_admin_level_id(level_name);
  }

  /**
   * @brief Returns locations in the specified administrative unit
   * @param unit_id The administrative unit ID
   * @param level_name The administrative level name (e.g., "district")
   * @return Const reference to vector of location IDs in the administrative unit
   * @throws std::runtime_error if admin level is not initialized
   */
  const std::vector<int>& get_locations_in_unit(const std::string& level_name, int unit_id) const {
    return admin_manager_->get_locations_in_unit(level_name, unit_id);
  }

  /**
   * @brief Returns the number of units in the specified administrative level
   * @param level_id The administrative level ID
   * @return The number of units in the administrative level
   * @throws std::runtime_error if admin level is not initialized
   */
  int get_unit_count(int level_id) const {
    return admin_manager_->get_unit_count(level_id);
  }

  /**
   * @brief Returns the number of units in the specified administrative level
   * @param level_name The administrative level name (e.g., "district")
   * @return The number of units in the administrative level
   * @throws std::runtime_error if admin level is not initialized
   */
  int get_unit_count(const std::string& level_name) const {
    if (admin_manager_ == nullptr) {
      return -1;
    }
    return admin_manager_->get_unit_count(level_name);
  }

  const BoundaryData* get_boundary(const std::string& level_name) const {
    if (admin_manager_ == nullptr) {
      return nullptr;
    }
    return admin_manager_->get_boundary(level_name);
  }

  // Get a reference to the AscFile raster, may be a nullptr
  AscFile* get_raster(SpatialFileType type) { return data[type].get(); }

  /**
   * @brief Initializes and configures administrative boundaries for the simulation.
   *
   * This function is designed to run once all necessary input data and raster
   * files have been read and processed. It sets up the AdminLevelManager and 
   * registers any administrative levels (like districts) found in the raster data.
   * The administrative boundaries are essential for spatial queries and operations
   * in the simulation, allowing locations to be grouped by administrative units.
   *
   * @note This function should be called after all input and raster data have
   * been fully processed but before the simulation begins to ensure that all
   * administrative boundaries are accurately configured.
   *
   * @pre Raster files and input data must be loaded and processed, including
   * any administrative boundary rasters (like district boundaries).
   *
   * @post The AdminLevelManager is initialized with all relevant administrative levels,
   * and administrative boundary data is configured for use in spatial operations.
   */
  void initialize_admin_boundaries();

  // Reset the singleton instance for testing
  void reset() {
    // Reset each unique_ptr individually
    for (auto &ptr : data) { ptr.reset(); }
  }

  // Add method to validate raster information
  bool validate_raster_info(const RasterInformation &new_info,
                            std::string &errors);

  /**
   * @brief Returns a list of all available administrative levels
   * @return Vector of administrative level names
   */
  const std::vector<std::string>& get_admin_levels() const {
    if (admin_manager_ == nullptr) {
      static const std::vector<std::string> empty_vector;
      return empty_vector;
    }
    return admin_manager_->get_level_names();
  }

  /**
   * @brief Checks if an administrative level exists
   * @param level_name The administrative level name to check
   * @return true if the level exists, false otherwise
   */
  bool has_admin_level(const std::string& level_name) const {
    if (admin_manager_ == nullptr) {
      return false;
    }
    return admin_manager_->has_level(level_name);
  }

  /**
   * @brief Gets all units in an administrative level
   * @param level_name The administrative level name
   * @return Vector of unit IDs for the requested level
   * @throws std::runtime_error if admin level does not exist
   */
  const std::pair<int,int> get_admin_units(const std::string& level_name) const {
    if (admin_manager_ == nullptr) {
      // return an invalid pair
      return {-1,-1};
    }
    return admin_manager_->get_units(level_name);
  }

private:
  /**
   * @brief Loads age distribution data from YAML configuration
   * @throws std::runtime_error if age distribution data is invalid
   */
  void load_age_distribution(const YAML::Node &node);

  /**
   * @brief Loads treatment data from YAML configuration if not provided by
   * raster
   * @throws std::runtime_error if treatment data is invalid
   */
  void load_treatment_data(const YAML::Node &node);

  /**
   * @brief Loads beta and population data from YAML if not provided by raster
   * @throws std::runtime_error if required data is missing or invalid
   */
  void load_location_data(const YAML::Node &node);


    /**
   * @brief Retrieves the district ID from the district raster file for a given location.
   *
   * This is an internal method used during initialization to build the district lookup table.
   * It performs coordinate-based lookups in the district raster file.
   *
   * @param location The location ID for which the district ID is requested.
   * @return The district ID from the raster file for the given location.
   * @throws std::out_of_range if location or coordinates are invalid
   * @throws std::runtime_error if district data is not loaded or coordinates are null
   */
  int get_district_from_raster(int location);

  
  /**
  * @brief This property holds a pre-populated map from location to district ID
  * as defined in the GIS raster file.
  *
  * The vector contains district IDs where each element represents a specific
  * location, and the value at each index corresponds to the actual district ID
  * from the raster file. The district IDs can be either 0-based or 1-based,
  * depending on how they are defined in the input GIS raster file. This mapping
  * is essential for quickly determining the district of any given location
  * within the simulation. It is assumed that the mapping is set up during the
  * initialization phase (in SpatialData::parse()) and remains constant
  * throughout the simulation, facilitating efficient spatial queries and
  * analyses.
  */
  std::vector<int> location_to_district;

  // Maps district IDs to their corresponding location IDs
  // Index is 0 to max_district_id+1 to handle both 0-based and 1-based IDs
  std::vector<std::vector<int>> district_to_locations;

  // Initialize array with nullptr
  std::array<std::unique_ptr<AscFile>, SpatialFileType::Count> data{};

  // Map of admin level names to their corresponding raster paths
  std::map<std::string, std::string> admin_rasters;

  // Helper method to parse administrative boundaries from YAML
  void load_admin_boundaries(const YAML::Node &node);
};

#endif
