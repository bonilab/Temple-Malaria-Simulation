/*
 * SeasonalPattern.cpp
 *
 * Implement the pattern-based seasonal model.
 */
#include "Helpers/TimeHelpers.h"
#include "SeasonalInfo.h"

SeasonalPattern* SeasonalPattern::build(const YAML::Node &node) {
  // Prepare the object to be returned
  auto value = new SeasonalPattern();

  value->initialize(node);

  return value;
}

void SeasonalPattern::read(const std::string &filename) {
  std::ifstream in(filename);
  if (!in.good()) {
    throw std::runtime_error("Error opening the rainfall data file: " + filename);
  }

  std::string line;
  // Skip header
  std::getline(in, line);

  // Read each district's data
  while (std::getline(in, line)) {
    std::stringstream ss(line);
    std::string token;
    
    // Read district ID
    std::getline(ss, token, ',');
    int district_id = std::stoi(token);
    
    // Ensure vector is large enough
    if (district_id >= district_adjustments.size()) {
      district_adjustments.resize(district_id + 1);
    }

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
          fmt::format("Expected {} factors for district {}, got {}", 
                     period, district_id, factors.size()));
    }

    // If monthly data, expand to daily
    if (is_monthly) {
      DoubleVector daily_factors;
      daily_factors.reserve(365);
      
      // For each month
      for (int month = 0; month < 12; month++) {
        // Calculate number of days for this month
        int days_in_month = (month == 1) ? 28 : (month == 3 || month == 5 || month == 8 || month == 10) ? 30 : 31;
        
        // Repeat the monthly value for each day
        for (int day = 0; day < days_in_month; day++) {
          daily_factors.push_back(factors[month]);
        }
      }
      district_adjustments[district_id] = daily_factors;
    } else {
      district_adjustments[district_id] = factors;
    }
  }
}

double SeasonalPattern::get_seasonal_factor(const date::sys_days &today,
                                           const int &location) {
  int district = get_district_for_location(location);
  int doy = TimeHelpers::day_of_year(today);
  
  // Adjust for leap year
  doy = (doy == 366) ? doy-2 : doy - 1;

  return district_adjustments[district][doy];
}
