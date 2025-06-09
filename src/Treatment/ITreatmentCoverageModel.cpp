/*
 * ITreatmentCoverageModel.cpp
 *
 * Implement the functions for the treatment coverage model abstract class.
 */
#include "ITreatmentCoverageModel.h"

#include <yaml-cpp/yaml.h>

#include "Core/Config/Config.h"
#include "InflatedTCM.hxx"
#include "LinearTCM.h"
#include "SteadyTCM.hxx"
#include "easylogging++.h"


// previous approach (before change to base treatment) (Uncomment if needed)
/*
double ITreatmentCoverageModel::get_probability_to_be_treated(
    const int &location, const int &age) {
  assert(location >= 0);  // Location should not be less than zero
  assert(static_cast<std::size_t>(location) < p_treatment_less_than_5.size()
         && static_cast<std::size_t>(location)
                < p_treatment_more_than_5
                      .size());  // Location is greater than the array size

  // This is abased upon the Malaria Indicator Survey, so we presume 0 - 59
  // months for under-5
  return (age < 5) ? p_treatment_less_than_5[location]
                   : p_treatment_more_than_5[location];
}
*/

// New code for age specific treatment coverage (ASTC) (May 2025)
double ITreatmentCoverageModel::get_probability_to_be_treated(
    const int &location, const int &age, std::vector<int> treatment_age_classes, std::vector<double>treatment_adjustments) {

  assert(location >= 0);  // Location should not be less than zero
  assert(static_cast<std::size_t>(location) < p_treatment_base.size());  // Location is greater than the array size



  double base_treatment = p_treatment_base.at(location);

for (size_t i=0; i< treatment_age_classes.size(); i++) {
  double weight;
  if(age <= treatment_age_classes[i]) {
    weight = treatment_adjustments[i];

    return base_treatment * weight;  // Return the adjusted treatment probability
  }
  }

  throw std::runtime_error(
    "Treatment Coverage for age: " + std::to_string(age) +
    " and location: " + std::to_string(location) +
    " cannot be determined"
);

 }

ITreatmentCoverageModel* ITreatmentCoverageModel::build_steady_tcm(
    const YAML::Node &node, Config* config) {
  auto* result = new SteadyTCM();

  const auto starting_date = node["day"].as<date::year_month_day>();
  result->starting_time_ =
      (date::sys_days{starting_date} - date::sys_days{config->starting_date()})
          .count();

  read_p_treatment(node["p_treatment_for_less_than_5_by_location"],
                   result->p_treatment_less_than_5,
                   config->number_of_locations());
  read_p_treatment(node["p_treatment_for_more_than_5_by_location"],
                   result->p_treatment_more_than_5,
                   config->number_of_locations());

  return result;
}

void ITreatmentCoverageModel::read_p_treatment(
    const YAML::Node &node, std::vector<double> &p_treatments,
    const std::size_t number_of_locations) {
  if (!node) return;
  for (std::size_t loc = 0; loc < number_of_locations; loc++) {
    std::size_t input_loc = node.size() < number_of_locations ? 0 : loc;
    p_treatments.push_back(node[input_loc].as<float>());
  }
}

ITreatmentCoverageModel* ITreatmentCoverageModel::build_inflated_tcm(
    const YAML::Node &node, Config* config) {
  auto* result = new InflatedTCM();
  const auto starting_date = node["day"].as<date::year_month_day>();
  result->starting_time_ =
      (date::sys_days{starting_date} - date::sys_days{config->starting_date()})
          .count();

  const auto annual_inflation_rate = node["annual_inflation_rate"].as<double>();
  result->monthly_inflation_rate = annual_inflation_rate / 12;

  read_p_treatment(node["p_treatment_for_less_than_5_by_location"],
                   result->p_treatment_less_than_5,
                   config->number_of_locations());
  read_p_treatment(node["p_treatment_for_more_than_5_by_location"],
                   result->p_treatment_more_than_5,
                   config->number_of_locations());

  return result;
}

ITreatmentCoverageModel* ITreatmentCoverageModel::build_linear_tcm(
    const YAML::Node &node, Config* config) {
  auto* result = new LinearTCM();
  const auto starting_date = node["from_day"].as<date::year_month_day>();
  const auto to_date = node["to_day"].as<date::year_month_day>();
  result->starting_time_ =
      (date::sys_days{starting_date} - date::sys_days{config->starting_date()})
          .count();
  result->end_time =
      (date::sys_days{to_date} - date::sys_days{config->starting_date()})
          .count();

  read_p_treatment(node["p_treatment_for_less_than_5_by_location_from"],
                   result->p_treatment_less_than_5,
                   config->number_of_locations());
  read_p_treatment(node["p_treatment_for_more_than_5_by_location_from"],
                   result->p_treatment_more_than_5,
                   config->number_of_locations());

  read_p_treatment(node["p_treatment_for_less_than_5_by_location_to"],
                   result->p_treatment_less_than_5_to,
                   config->number_of_locations());
  read_p_treatment(node["p_treatment_for_less_than_5_by_location_to"],
                   result->p_treatment_more_than_5_to,
                   config->number_of_locations());

  return result;
}

ITreatmentCoverageModel* ITreatmentCoverageModel::build(const YAML::Node &node,
                                                        Config* config) {
  const auto type = node["type"].as<std::string>();
  if (type == "SteadyTCM") { return build_steady_tcm(node, config); }
  if (type == "InflatedTCM") { return build_inflated_tcm(node, config); }
  if (type == "LinearTCM") { return build_linear_tcm(node, config); }
  throw std::runtime_error("Unknown treatment coverage model: " + type);
}
