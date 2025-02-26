/*
 * SeasonalInfo.h
 *
 * Define the various seasonal information methods that are supported by
 */
#ifndef SEASONALINFO_H
#define SEASONALINFO_H

#include <date/date.h>
#include <easylogging++.h>
#include <fmt/format.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>

#include "Core/TypeDef.h"
#include "GIS/SpatialData.h"

class Config;

class ISeasonalInfo {
public:
  // Return the seasonal factor for the given day and location, based upon the
  // loaded configuration.
  virtual double get_seasonal_factor(const date::sys_days &today,
                                     const int &location) {
    throw std::runtime_error("Runtime call to virtual function");
  }
};

class SeasonalDisabled : public ISeasonalInfo {
public:
  double get_seasonal_factor(const date::sys_days &today,
                             const int &location) override {
    return 1.0;
  }
};

class SeasonalEquation : public ISeasonalInfo {
private:
  DoubleVector base;
  DoubleVector A;
  DoubleVector B;
  DoubleVector phi;

  // The reference values contain the inputs from the YAML so the
  // UpdateEcozoneEvent can change the ecozone (i.e., seasonal information)
  // during model execution
  DoubleVector reference_base;
  DoubleVector reference_A;
  DoubleVector reference_B;
  DoubleVector reference_phi;

  void set_from_raster(const YAML::Node &node);
  void set_seasonal_period(const YAML::Node &node, unsigned long index);

public:
  static SeasonalEquation* build(const YAML::Node &node, Config* config);
  double get_seasonal_factor(const date::sys_days &today,
                             const int &location) override;
  void update_seasonality(int from, int to);
};

class SeasonalPattern : public ISeasonalInfo {
protected:
  // Vector of vectors: [district][day/month]
  std::vector<DoubleVector> district_adjustments;
  int period;  // Either 365 for daily or 12 for monthly
  bool is_monthly;  // Flag to indicate if we're using monthly data

  // Make this virtual so we can override it in tests
  virtual int get_district_for_location(int location) const {
    return SpatialData::get_instance().get_district(location);
  }

  void read(const std::string &filename);

  // Protected method for initialization logic
  void initialize(const YAML::Node &node) {
    auto settings = node["pattern"];
    // Validate settings
    if (settings["filename"].IsNull()) {
      throw std::invalid_argument("The seasonal pattern filename parameter is missing.");
    }
    if (settings["period"].IsNull()) {
      throw std::invalid_argument("The seasonal pattern period parameter is missing.");
    }

    // Set period (12 or 365)
    period = settings["period"].as<int>();
    if (period != 12 && period != 365) {
      throw std::invalid_argument("Period must be either 12 (monthly) or 365 (daily)");
    }
    is_monthly = (period == 12);

    // Read the district-specific adjustments
    auto filename = settings["filename"].as<std::string>();
    read(filename);
  }

public:
  static SeasonalPattern* build(const YAML::Node &node);
  double get_seasonal_factor(const date::sys_days &today,
                           const int &location) override;

  // Getter methods for testing
  bool get_is_monthly() const { return is_monthly; }
  int get_period() const { return period; }
};

class SeasonalInfoFactory {
public:
  static ISeasonalInfo* build(const YAML::Node &node, Config* config) {
    // If seasonality has been disabled then don't bother parsing the rest
    auto enabled = node["enable"].as<bool>();
    if (!enabled) {
      VLOG(1) << "Seasonal information disabled.";
      return new SeasonalDisabled();
    }

    // Check to make sure the mode node exists
    try {
      if (node["mode"].IsNull()) {
        throw std::invalid_argument("Seasonal information mode is not found.");
      }
    } catch (YAML::InvalidNode &ex) {
      throw std::invalid_argument(
          "Seasonal information mode node is not found.");
    }

    // Return the correct object for the named mode
    auto mode = node["mode"].as<std::string>();
    std::transform(mode.begin(), mode.end(), mode.begin(), ::toupper);
    if (mode == "EQUATION") {
      LOG(INFO) << "Using equation-based seasonal information.";
      return SeasonalEquation::build(node, config);
    }
    if (mode == "PATTERN") {
      LOG(INFO) << "Using pattern-based seasonal information.";
      return SeasonalPattern::build(node);
    }
    throw std::runtime_error(fmt::format("Unknown seasonal mode {}", mode));
  }
};

#endif
