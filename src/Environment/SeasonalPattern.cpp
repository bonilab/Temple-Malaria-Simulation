/*
 * SeasonalPattern.cpp
 *
 * Implement the pattern-based seasonal model.
 */
#include "SeasonalPattern.h"

#include <fmt/format.h>

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

#include "GIS/SpatialData.h"
#include "Helpers/TimeHelpers.h"
#include "easylogging++.h"

SeasonalPattern* SeasonalPattern::build(const YAML::Node &node, SpatialData* spatial_data) {
  auto* result = new SeasonalPattern();
  result->initialize(node, spatial_data);
  return result;
}

void SeasonalPattern::initialize(const YAML::Node &node, SpatialData* spatial_data) {
  VLOG(1) << "Initializing SeasonalPattern";
  auto settings = node["pattern"];
  // Validate settings
  if (settings["filename"].IsNull()) {
    throw std::invalid_argument(
        "The seasonal pattern filename parameter is missing.");
  }
  if (settings["period"].IsNull()) {
    throw std::invalid_argument(
        "The seasonal pattern period parameter is missing.");
  }

  // Set period (12 or 365)
  period = settings["period"].as<int>();
  if (period != 12 && period != 365) {
    throw std::invalid_argument(
        "Period must be either 12 (monthly) or 365 (daily)");
  }
  is_monthly = (period == 12);

  // Read the district-specific adjustments
  auto filename = settings["filename"].as<std::string>();

  admin_level = settings["admin_level"].as<std::string>();
  if (admin_level == "") {
    throw std::invalid_argument("The admin level parameter is missing.");
  }

  admin_level_id = spatial_data->get_admin_level_id(admin_level);
  if (admin_level_id == -1) {
    throw std::invalid_argument("The admin level parameter is invalid.");
  }

  read(filename);

  // Validate against SpatialData if available
  if (spatial_data->get_unit_count(admin_level_id) > 0) {
    auto boundary = spatial_data->get_admin_level_manager()->get_boundary(admin_level);
    if (admin_unit_adjustments.size()
        != boundary->max_unit_id + 1) {
      throw std::runtime_error(
          fmt::format("Expected {} {}s, got {}", 
                      boundary->max_unit_id + 1,
                      admin_level,
                      admin_unit_adjustments.size()));
    }
  }
}


int SeasonalPattern::get_admin_unit_for_location(int location) const {
  if (SpatialData::get_instance().get_unit_count(admin_level_id) <= 0) {
    return min_admin_unit_id;
  }

  return SpatialData::get_instance().get_admin_unit(admin_level_id, location);
}

void SeasonalPattern::read(const std::string &filename) {
  std::ifstream in(filename);
  if (!in.good()) {
    throw std::runtime_error("Error opening the rainfall data file: "
                             + filename);
  }

  std::string line;
  // Skip header
  std::getline(in, line);

  min_admin_unit_id = std::numeric_limits<int>::max();
  max_admin_unit_id = std::numeric_limits<int>::min();

  // Temporary storage for data
  std::map<int, DoubleVector> temp_adjustments;

  // Read each district's data
  while (std::getline(in, line)) {
    std::stringstream ss(line);
    std::string token;

    // Read district ID
    std::getline(ss, token, ',');
    int admin_unit_id = std::stoi(token);

    min_admin_unit_id = std::min(min_admin_unit_id, admin_unit_id);
    max_admin_unit_id = std::max(max_admin_unit_id, admin_unit_id);

    // Read seasonal factors
    DoubleVector factors;
    while (std::getline(ss, token, ',')) {
      double factor = std::stod(token);
      if (factor < 0.0) {
        throw std::runtime_error(
            fmt::format("Rainfall factor less than zero: {0}", factor));
      }
      factors.push_back(factor);
    }

    // Validate number of factors
    if (factors.size() != period) {
      throw std::runtime_error(
          fmt::format("Expected {} factors for admin unit {}, got {}", period,
                      admin_unit_id, factors.size()));
    }

    // Store in temporary map with original ID
    temp_adjustments[admin_unit_id] = factors;
  }

  // Determine if input is 0-based or 1-based
  bool is_one_based = (min_admin_unit_id == 1);
  bool is_zero_based = (min_admin_unit_id == 0);

  if (!is_one_based && !is_zero_based) {
    throw std::runtime_error(fmt::format(
        "Admin unit IDs must start at 0 or 1, but found minimum ID: {}",
        min_admin_unit_id));
  }

  // Calculate actual admin unit count
  int actual_admin_unit_count = max_admin_unit_id - min_admin_unit_id + 1;

  // Size the vector to accommodate direct indexing (size = count for 0-based, count+1 for 1-based)
  admin_unit_adjustments.clear();
  admin_unit_adjustments.resize(min_admin_unit_id == 0 ? actual_admin_unit_count : actual_admin_unit_count + 1);
  
  // Store factors using original admin unit IDs directly
  for (const auto &[file_id, factors] : temp_adjustments) {
    admin_unit_adjustments[file_id] = factors;
  }

  LOG(INFO) << fmt::format("Loaded {} admin units from {} ({}-based indexing)",
                           actual_admin_unit_count, filename,
                           min_admin_unit_id);
}

double SeasonalPattern::get_seasonal_factor(const date::sys_days &today,
                                          const int &location) {
  int admin_unit = get_admin_unit_for_location(location);
  
  int doy = TimeHelpers::day_of_year(today);

  // Get the month (0-11)
  auto ymd = date::year_month_day{today};
  int month = static_cast<unsigned>(ymd.month()) - 1;

  // For monthly data, use the month directly
  if (is_monthly) {
    return admin_unit_adjustments[admin_unit][month];
  } else {
    // For daily data, use the day of year (0-364)
    doy = (doy == 366) ? 364 : doy - 1;
    return admin_unit_adjustments[admin_unit][doy];
  }
}
