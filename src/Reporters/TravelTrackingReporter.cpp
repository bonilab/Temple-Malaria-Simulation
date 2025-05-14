#include "GIS/SpatialData.h"
#ifdef ENABLE_TRAVEL_TRACKING
#include "Core/Config/Config.h"
#include "Model.h"
#include "Population/Population.h"
#include "Population/Properties/PersonIndexAll.h"
#include "TravelTrackingReporter.h"

void TravelTrackingReporter::initialize(int job_number,
                                        const std::string &path) {
  output_file.open(fmt::format("{}travel_tracking_{}.csv", path, job_number));
}

void TravelTrackingReporter::before_run() {}

void TravelTrackingReporter::begin_time_step() {}

void TravelTrackingReporter::monthly_report() {}
/* Column Name	Data Type	Description */
/* district_id	Integer	Unique identifier for the district. */
/* population_size	Integer	Total number of people residing in the district. */
/* percentage_traveled_last_30_days	Float	Percentage of district population
 * that initiated any trip in the last 30 days. */
/* percentage_traveled_last_60_days	Float	Percentage of district population
 * that initiated any trip in the last 60 days. */
/* percentage_traveled_last_90_days	Float	Percentage of district population
 * that initiated any trip in the last 90 days. */
/* percentage_cross_district_last_30_days	Float	Percentage of district
 * population that initiated a trip outside their district in the last 30 days.
 */
/* percentage_cross_district_last_60_days	Float	Percentage of district
 * population that initiated a trip outside their district in the last 60 days.
 */
/* percentage_cross_district_last_90_days	Float	Percentage of district
 * population that initiated a trip outside their district in the last 90 days.
 */
void TravelTrackingReporter::after_run() {
  // output the travel tracking data
  // csv columns: district_id, population_size,
  // percentage_traveled_last_30_days, percentage_traveled_last_60_days
  // percentage_traveled_last_90_days
  // percentage_cross_district_last_30_days,
  // percentage_cross_district_last_60_days,
  // percentage_cross_district_last_90_days

  const auto* boundary = SpatialData::get_instance().get_boundary("district");
  const auto vector_size = boundary->max_unit_id + 1;
  const auto min_district = boundary->min_unit_id;
  const auto max_district = boundary->max_unit_id;

  std::vector<int> population(vector_size, 0);
  std::vector<int> traveled_last_30_days(vector_size, 0);
  std::vector<int> traveled_last_60_days(vector_size, 0);
  std::vector<int> traveled_last_90_days(vector_size, 0);

  std::vector<int> cross_district_last_30_days(vector_size, 0);
  std::vector<int> cross_district_last_60_days(vector_size, 0);
  std::vector<int> cross_district_last_90_days(vector_size, 0);

  auto current_time = Model::SCHEDULER->current_time();
  auto* all_person_index =
      Model::POPULATION->get_person_index<PersonIndexAll>();
  for (auto* person : all_person_index->vPerson()) {
    auto district = SpatialData::get_instance().get_admin_unit(
        "district", person->location());
    population[district]++;

    if (person->day_that_last_trip_was_initiated() > current_time - 30) {
      traveled_last_30_days[district]++;
    }
    if (person->day_that_last_trip_was_initiated() > current_time - 60) {
      traveled_last_60_days[district]++;
    }
    if (person->day_that_last_trip_was_initiated() > current_time - 90) {
      traveled_last_90_days[district]++;
    }
    if (person->day_that_last_trip_outside_district_was_initiated()
        > current_time - 30) {
      cross_district_last_30_days[district]++;
    }
    if (person->day_that_last_trip_outside_district_was_initiated()
        > current_time - 60) {
      cross_district_last_60_days[district]++;
    }
    if (person->day_that_last_trip_outside_district_was_initiated()
        > current_time - 90) {
      cross_district_last_90_days[district]++;
    }
  }

  // output
  /// output header
  output_file << fmt::format(
      "{},{},{},{},{},{},{},{}\n", "district_id", "population_size",
      "percentage_traveled_last_30_days", "percentage_traveled_last_60_days",
      "percentage_traveled_last_90_days",
      "percentage_cross_district_last_30_days",
      "percentage_cross_district_last_60_days",
      "percentage_cross_district_last_90_days");

  for (auto district = min_district; district <= max_district; district++) {
    output_file << fmt::format(
        "{},{},{},{},{},{},{},{}\n", district, population[district],
        population[district] == 0
            ? 0
            : static_cast<float>(traveled_last_30_days[district])
                  / static_cast<float>(population[district]),
        population[district] == 0
            ? 0
            : static_cast<float>(traveled_last_60_days[district])
                  / static_cast<float>(population[district]),
        population[district] == 0
            ? 0
            : static_cast<float>(traveled_last_90_days[district])
                  / static_cast<float>(population[district]),
        population[district] == 0
            ? 0
            : static_cast<float>(cross_district_last_30_days[district])
                  / static_cast<float>(population[district]),
        population[district] == 0
            ? 0
            : static_cast<float>(cross_district_last_60_days[district])
                  / static_cast<float>(population[district]),
        population[district] == 0
            ? 0
            : static_cast<float>(cross_district_last_90_days[district])
                  / static_cast<float>(population[district]));
  }

  // close the file
  if (output_file.is_open()) { output_file.close(); }
}

#endif  // ENABLE_TRAVEL_TRACKING
