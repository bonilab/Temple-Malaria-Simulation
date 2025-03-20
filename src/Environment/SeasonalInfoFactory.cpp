#include "SeasonalInfoFactory.h"
#include "SeasonalDisabled.h"
#include "SeasonalEquation.h"
#include "SeasonalPattern.h"
#include "GIS/SpatialData.h"
#include <easylogging++.h>
#include <fmt/format.h>
#include <algorithm>

ISeasonalInfo* SeasonalInfoFactory::build(const YAML::Node &node, Config* config) {
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
    return SeasonalPattern::build(node, &SpatialData::get_instance());
  }
  throw std::runtime_error(fmt::format("Unknown seasonal mode {}", mode));
} 