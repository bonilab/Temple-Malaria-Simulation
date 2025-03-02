/*
 * SpatialData.h
 *
 * Definitions of the thread-safe singleton pattern spatial class which manages
 * the spatial aspects of the model from a high level.
 */
#ifndef SPATIALDATA_H
#define SPATIALDATA_H

#include <yaml-cpp/yaml.h>

#include <string>
#include <vector>
#include <array>
#include <memory>

#include "AscFile.h"
#include "Core/PropertyMacro.h"

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
  };

  /**
   * @brief This property holds a pre-populated map from location to district
   * using a 0-based index.
   *
   * The vector contains indices where each element represents a specific
   * location, and the value at each index corresponds to the district that
   * location belongs to. This mapping is essential for quickly determining the
   * district of any given location within the simulation. It is assumed that
   * the mapping is set up during the initialization phase (in
   * SpatialData::parse()) and remains constant throughout the
   * simulation, facilitating efficient spatial queries and analyses.
   */
  PROPERTY_REF(std::vector<int>, district_lookup)

// private:
  const std::string BETA_RASTER = "beta_raster";
  const std::string DISTRICT_RASTER = "district_raster";
  const std::string LOCATION_RASTER = "location_raster";
  const std::string POPULATION_RASTER = "population_raster";
  const std::string TRAVEL_RASTER = "travel_raster";
  const std::string ECOCLIMATIC_RASTER = "ecoclimatic_raster";
  const std::string TREATMENT_RATE_UNDER5 = "pr_treatment_under5";
  const std::string TREATMENT_RATE_OVER5 = "pr_treatment_over5";

  // Initialize array with nullptr
  std::array<std::unique_ptr<AscFile>, SpatialFileType::Count> data{};

  // Flag to indicate if data has been loaded since the last time it was checked
  bool dirty = false;

  // The size of the cells in the raster, the units shouldn't matter, but this
  // was written when we were using 5x5 km cells
  float cell_size = 0;

  // First district index, default -1, lazy initialization to actual value
  int first_district = -1;

  // Count of district loaded in the map, default zero, lazy initialization to
  // actual value
  int district_count = -1;

  // Constructor
  SpatialData();

  // Deconstructor
  ~SpatialData();

  // Check the loaded spatial catalog for errors, returns true if there are
  // errors
  bool check_catalog(std::string &errors);

  // Generate the locations for the location_db
  void generate_locations();

  // Load the given raster file into the spatial catalog and assign the given
  // label
  void load(const std::string &filename, SpatialFileType type);

  // Load all the spatial data from the node
  void load_files(const YAML::Node &node);

  // Load the raster indicated into the location_db; works with betas and
  // probability of treatment
  void load_raster(SpatialFileType type);

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

  // Return true if any raster file has been loaded, false otherwise
  bool has_raster();

  // Return true if a raster file has been loaded, false otherwise
  bool has_raster(SpatialFileType type) { return data[type] != nullptr; }

  // Generate the Euclidean distances for the location_db
  void generate_distances() const;

  /**
   * @brief Retrieves the district ID as defined in raster files, corresponding
   * to a given location ID.
   *
   * This function maps a location ID to its corresponding district ID, as
   * defined within the raster files used by the simulation. This mapping is
   * crucial for spatial analyses and operations that require understanding the
   * geographical district a specific location belongs to.
   *
   * @param location The location ID for which the district ID is requested.
   * @return The district ID matching the location ID, as defined in the raster
   * files.
   */
  int get_raster_district(int location);

  /**
   * @brief Retrieves the 0-based district ID in the simulation, corresponding
   * to a given location ID.
   *
   * This function translates a location ID into a simulation-internal, 0-based
   * district ID. It is used primarily for internal simulation operations where
   * districts are handled with 0-based indexing.
   *
   * @param location The location ID for which the 0-based district ID is
   * requested.
   * @return The 0-based district ID corresponding to the given location ID.
   */
  int get_district(int location);

  /**
   * @brief Returns the adjusted district index matching the definition in
   * raster files, for external storage.
   *
   * This function adjusts an internal simulation district index to match the
   * district indexing as defined in the raster files. The adjusted index is
   * suitable for external references, such as database entries, ensuring
   * consistency between the simulation's internal data representation and
   * external data stores.
   *
   * @param simulationDistrict The internal simulation district index that needs
   * adjustment for external use.
   * @return The adjusted district index that matches the raster file
   * definitions, suitable for external references.
   */
  int adjust_simulation_district_to_raster_index(int simulationDistrict) {
    // Assuming get_first_district() returns the necessary adjustment. Adjust as
    // per your actual logic.
    return simulationDistrict + get_first_district();
  }

  // Get the count of districts loaded, or -1 if they have not been loaded
  int get_district_count();

  // Get the locations that are within the given district, throws an error if
  // not districts are loaded
  std::vector<int> get_district_locations(int district);

  // Returns the index of the first district.
  // Note that the index may be one (ArcGIS default) or zero; however, a
  // delayed error is generated if the value is not one of the two.
  int get_first_district();

  // Get a reference to the AscFile raster, may be a nullptr
  AscFile* get_raster(SpatialFileType type) { return data[type].get(); }

  // Parse the YAML node provided to extract all the relevant information for
  // the simulation
  bool parse(const YAML::Node &node);

  /**
   * @brief Populates dependent data structures after input data and raster
   * files have been processed.
   *
   * This function is designed to run once all necessary input data and raster
   * files have been read and processed. It performs the crucial step of
   * populating several dependent data structures that are essential for the
   * simulation's operation. These include caching the total count of districts,
   * determining the first district index based on the spatial data, and
   * creating the district lookup table. The function ensures that these
   * components are correctly initialized before the simulation proceeds,
   * guaranteeing that spatial queries and operations can be conducted
   * efficiently.
   *
   * @note This function should be called after all input and raster data have
   * been fully processed but before the simulation begins to ensure that all
   * dependent data structures are accurately populated. Failure to call this
   * function in the correct sequence may result in uninitialized or incorrect
   * data being used in the simulation, leading to potential errors or
   * inaccurate results.
   *
   * @pre Raster files and input data must be loaded and processed. This
   * includes loading spatial data such as district boundaries and population
   * distributions from raster files.
   *
   * @post The total district count is cached, the first district index is
   * determined, and the district lookup table is created and populated. These
   * actions prepare the system for efficient spatial operations and
   * simulations.
   */
  void populate_dependent_data();

  // Refresh the data from the model (i.e., Location DB) to the spatial data
  void refresh();

  // Write the current spatial data to the filename and path indicated, output
  // will be an ASC file
  void write(const std::string &filename, SpatialFileType type);

  // Reset the singleton instance for testing
  void reset() {
    // Reset each unique_ptr individually
    for (auto& ptr : data) {
      ptr.reset();
    }
    first_district = 0;
    district_count = 0;
  }
};

#endif
