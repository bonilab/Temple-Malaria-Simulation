/*
 * SeasonalPattern.cpp
 *
 * Implement the pattern-based seasonal model.
 */
#include "SeasonalPattern.h"

#include <fmt/format.h>

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <map>

#include "GIS/SpatialData.h"
#include "Helpers/TimeHelpers.h"
#include "easylogging++.h"

SeasonalPattern* SeasonalPattern::build(const YAML::Node &node) {
  auto* result = new SeasonalPattern();
  result->initialize(node);
  return result;
}

void SeasonalPattern::initialize(const YAML::Node &node) {
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
  read(filename);
}

int SeasonalPattern::get_district_for_location(int location) const {
  return SpatialData::get_instance().get_district(location);
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

  int min_district_id = std::numeric_limits<int>::max();
  int max_district_id = std::numeric_limits<int>::min();
  
  // Temporary storage for data
  std::map<int, DoubleVector> temp_adjustments;

  // Read each district's data
  while (std::getline(in, line)) {
    std::stringstream ss(line);
    std::string token;

    // Read district ID
    std::getline(ss, token, ',');
    int district_id = std::stoi(token);

    min_district_id = std::min(min_district_id, district_id);
    max_district_id = std::max(max_district_id, district_id);

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
          fmt::format("Expected {} factors for district {}, got {}", period,
                      district_id, factors.size()));
    }

    // Store in temporary map with original ID
    temp_adjustments[district_id] = factors;
  }

  // Determine if input is 0-based or 1-based
  bool is_one_based = (min_district_id == 1);
  bool is_zero_based = (min_district_id == 0);
  
  if (!is_one_based && !is_zero_based) {
    throw std::runtime_error(
        fmt::format("District IDs must start at 0 or 1, but found minimum ID: {}", 
                   min_district_id));
  }

  // Calculate actual district count
  int actual_district_count = max_district_id - min_district_id + 1;

  // Validate against SpatialData if available
  if (SpatialData::get_instance().get_district_count() != -1) {
    if (actual_district_count != SpatialData::get_instance().get_district_count()) {
      throw std::runtime_error(
          fmt::format("Expected {} districts, got {}",
                     SpatialData::get_instance().get_district_count(),
                     actual_district_count));
    }
  }

  // Convert to 0-based and store in final vector
  district_adjustments.clear();
  district_adjustments.resize(actual_district_count);
  for (const auto& [file_id, factors] : temp_adjustments) {
    int zero_based_id = is_one_based ? file_id - 1 : file_id;
    district_adjustments[zero_based_id] = factors;
  }

  LOG(INFO) << fmt::format("Loaded {} districts from {} ({}-based indexing)", 
                          actual_district_count, 
                          filename, 
                          is_one_based ? "1" : "0");
}

double SeasonalPattern::get_seasonal_factor(const date::sys_days &today,
                                            const int &location) {
  int district = get_district_for_location(location);

  int doy = TimeHelpers::day_of_year(today);

  // Get the month (0-11)
  auto ymd = date::year_month_day{today};
  int month = static_cast<unsigned>(ymd.month()) - 1;

  // For monthly data, use the month directly
  if (is_monthly) {
    return district_adjustments[district][month];
  } else {
    // For daily data, use the day of year (0-364)
    doy = (doy == 366) ? 364 : doy - 1;
    return district_adjustments[district][doy];
  }
}
