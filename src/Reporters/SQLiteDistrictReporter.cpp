#include "SQLiteDistrictReporter.h"

#include <fmt/printf.h>

#include "Core/Config/Config.h"
#include "GIS/SpatialData.h"
#include "MDC/MainDataCollector.h"
#include "Model.h"
#include "Population/Population.h"
#include "Population/Properties/PersonIndexByLocationStateAgeClass.h"
#include "easylogging++.h"

// Initialize the reporter
// Sets up the database and prepares it for data entry
void SQLiteDistrictReporter::initialize(int jobNumber,
                                        const std::string &path) {
  // Inform the user of the reporter type and make sure there are districts
  VLOG(1) << "Using SQLiteDbReporterwith aggregation at the district level.";
  if (!SpatialData::get_instance().has_raster(SpatialData::Districts)) {
    LOG(ERROR) << "District raster must be present when aggregating data at "
                  "the district level.";
    throw std::invalid_argument("No district raster present");
  }

  SQLiteDbReporter::initialize(jobNumber, path);
}

void SQLiteDistrictReporter::count_infections_for_location(int location) {
  auto &districtLookup = SpatialData::get_instance().district_lookup();
  auto &ageClasses = Model::CONFIG->age_structure();
  auto* index =
      Model::POPULATION->get_person_index<PersonIndexByLocationStateAgeClass>();

  // Calculate the correct index and update the count
  auto district = districtLookup[location];

  for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
    for (unsigned int ac = 0; ac < ageClasses.size(); ac++) {
      for (auto &person : index->vPerson()[location][hs][ac]) {
        // Is the individual infected by at least one parasite?
        if (person->all_clonal_parasite_populations()->parasites()->empty()) {
          continue;
        }

        monthly_site_data.infections_by_district[district]++;
      }
    }
  }
}

void SQLiteDistrictReporter::collect_site_data_for_location(int location) {
  auto &districtLookup = SpatialData::get_instance().district_lookup();
  auto district = districtLookup[location];
  auto &ageClasses = Model::CONFIG->age_structure();

  count_infections_for_location(location);

  auto locationPopulation = static_cast<int>(Model::POPULATION->size(location));
  // Collect the simple data
  monthly_site_data.population[district] +=
      static_cast<int>(locationPopulation);

  monthly_site_data.clinical_episodes[district] +=
      Model::MAIN_DATA_COLLECTOR
          ->monthly_number_of_clinical_episode_by_location()[location];

  monthly_site_data.treatments[district] +=
      Model::MAIN_DATA_COLLECTOR
          ->monthly_number_of_treatment_by_location()[location];
  monthly_site_data.recrudescence_treatments[district] +=
      Model::MAIN_DATA_COLLECTOR
          ->monthly_number_of_recrudescence_treatment_by_location()[location];
  monthly_site_data.treatment_failures[district] +=
      Model::MAIN_DATA_COLLECTOR
          ->monthly_treatment_failure_by_location()[location];
  monthly_site_data.nontreatment[district] +=
      Model::MAIN_DATA_COLLECTOR->monthly_nontreatment_by_location()[location];

  for (auto ndx = 0; ndx < ageClasses.size(); ndx++) {
    // Collect the treatment by age class, following the 0-59 month convention
    // for under-5
    if (ageClasses[ndx] < 5) {
      monthly_site_data.treatments_under5[district] +=
          Model::MAIN_DATA_COLLECTOR
              ->monthly_number_of_treatment_by_location_age_class()[location]
                                                                   [ndx];
    } else {
      monthly_site_data.treatments_over5[district] +=
          Model::MAIN_DATA_COLLECTOR
              ->monthly_number_of_treatment_by_location_age_class()[location]
                                                                   [ndx];
    }

    // collect the clinical episodes by age class
    monthly_site_data.clinical_episodes_by_age_class[district][ndx] +=
        Model::MAIN_DATA_COLLECTOR
            ->monthly_number_of_clinical_episode_by_location_age_class()
                [location][ndx];
  }

  // EIR and PfPR is a bit more complicated since it could be an invalid value
  // early in the simulation, and when aggregating at the district level the
  // weighted mean needs to be reported instead
  if (Model::MAIN_DATA_COLLECTOR->recording_data()) {
    auto eirLocation =
        Model::MAIN_DATA_COLLECTOR->EIR_by_location_year()[location].empty()
            ? 0
            : Model::MAIN_DATA_COLLECTOR->EIR_by_location_year()[location]
                  .back();
    monthly_site_data.eir[district] += (eirLocation * locationPopulation);
    monthly_site_data.pfpr_under5[district] +=
        (Model::MAIN_DATA_COLLECTOR->get_blood_slide_prevalence(location, 0, 5)
         * locationPopulation);
    monthly_site_data.pfpr2to10[district] +=
        (Model::MAIN_DATA_COLLECTOR->get_blood_slide_prevalence(location, 2, 10)
         * locationPopulation);
    monthly_site_data.pfpr_all[district] +=
        (Model::MAIN_DATA_COLLECTOR
             ->blood_slide_prevalence_by_location()[location]
         * locationPopulation);
  }
}

void SQLiteDistrictReporter::calculate_and_build_up_site_data_insert_values(
    int monthId) {
  auto numDistricts = SpatialData::get_instance().get_district_count();
  insert_values.clear();

  for (auto district = 0; district < numDistricts; district++) {
    double calculatedEir = (monthly_site_data.eir[district] != 0)
                               ? (monthly_site_data.eir[district]
                                  / monthly_site_data.population[district])
                               : 0;
    double calculatedPfprUnder5 =
        (monthly_site_data.pfpr_under5[district] != 0)
            ? (monthly_site_data.pfpr_under5[district]
               / monthly_site_data.population[district])
                  * 100.0
            : 0;
    double calculatedPfpr2to10 =
        (monthly_site_data.pfpr2to10[district] != 0)
            ? (monthly_site_data.pfpr2to10[district]
               / monthly_site_data.population[district])
                  * 100.0
            : 0;
    double calculatedPfprAll = (monthly_site_data.pfpr_all[district] != 0)
                                   ? (monthly_site_data.pfpr_all[district]
                                      / monthly_site_data.population[district])
                                         * 100.0
                                   : 0;

    std::string singleRow = fmt::format(
        "({}, {}, {}, {}", monthId,
        SpatialData::get_instance().adjust_simulation_district_to_raster_index(
            district),
        monthly_site_data.population[district],
        monthly_site_data.clinical_episodes[district]);

    // Append clinical episodes by age class
    for (const auto &episodes :
         monthly_site_data.clinical_episodes_by_age_class[district]) {
      singleRow += fmt::format(", {}", episodes);
    }

    singleRow += fmt::format(
        ", {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {})",
        monthly_site_data.treatments[district],
        monthly_site_data.recrudescence_treatments[district], calculatedEir,
        calculatedPfprUnder5, calculatedPfpr2to10, calculatedPfprAll,
        monthly_site_data.infections_by_district[district],
        monthly_site_data.treatment_failures[district],
        monthly_site_data.nontreatment[district],
        monthly_site_data.treatments_under5[district],
        monthly_site_data.treatments_over5[district]);

    insert_values.push_back(singleRow);
  }
}

// Collect and store monthly site data
// Aggregates data related to various site metrics and stores them in the
// database
void SQLiteDistrictReporter::monthly_report_site_data(int monthId) {
  TransactionGuard transaction{db.get()};
  auto numDistricts = SpatialData::get_instance().get_district_count();
  auto &ageClasses = Model::CONFIG->age_structure();

  // Prepare the data structures
  reset_site_data_structures(numDistricts, ageClasses.size());

  // Collect the data
  for (auto location = 0; location < Model::CONFIG->number_of_locations();
       location++) {
    // If the population is zero, press on
    auto locationPopulation =
        static_cast<int>(Model::POPULATION->size(location));
    if (locationPopulation == 0) { continue; }

    collect_site_data_for_location(location);
  }

  calculate_and_build_up_site_data_insert_values(monthId);

  insert_monthly_site_data(insert_values);
}

void SQLiteDistrictReporter::collect_genome_data_for_location(size_t location) {
  const auto &districtLookup = SpatialData::get_instance().district_lookup();
  const auto district = districtLookup[location];
  auto* index =
      Model::POPULATION->get_person_index<PersonIndexByLocationStateAgeClass>();
  auto ageClasses = index->vPerson()[0][0].size();

  for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
    // Iterate over all the age classes
    for (unsigned int ac = 0; ac < ageClasses; ac++) {
      // Iterate over all the genotypes
      auto peopleInAgeClass = index->vPerson()[location][hs][ac];
      for (auto &person : peopleInAgeClass) {
        collect_genome_data_for_a_person(person, district);
      }
    }
  }
}

void SQLiteDistrictReporter::reset_site_data_structures(int numDistricts,
                                                        size_t numAgeClasses) {
  // reset the data structures
  monthly_site_data.eir.assign(numDistricts, 0);
  monthly_site_data.pfpr_under5.assign(numDistricts, 0);
  monthly_site_data.pfpr2to10.assign(numDistricts, 0);
  monthly_site_data.pfpr_all.assign(numDistricts, 0);
  monthly_site_data.population.assign(numDistricts, 0);
  monthly_site_data.clinical_episodes.assign(numDistricts, 0);
  monthly_site_data.clinical_episodes_by_age_class.assign(
      numDistricts, std::vector<int>(numAgeClasses, 0));
  monthly_site_data.treatments.assign(numDistricts, 0);
  monthly_site_data.recrudescence_treatments.assign(numDistricts, 0);
  monthly_site_data.treatment_failures.assign(numDistricts, 0);
  monthly_site_data.nontreatment.assign(numDistricts, 0);
  monthly_site_data.treatments_under5.assign(numDistricts, 0);
  monthly_site_data.treatments_over5.assign(numDistricts, 0);
  monthly_site_data.infections_by_district.assign(numDistricts, 0);
}

void SQLiteDistrictReporter::reset_genome_data_structures(int numDistricts,
                                                          size_t numGenotypes) {
  // reset the data structures
  monthly_genome_data.occurrences.assign(numDistricts,
                                         std::vector<int>(numGenotypes, 0));
  monthly_genome_data.clinical_occurrences.assign(
      numDistricts, std::vector<int>(numGenotypes, 0));
  monthly_genome_data.occurrences_0_5.assign(numDistricts,
                                             std::vector<int>(numGenotypes, 0));
  monthly_genome_data.occurrences_2_10.assign(
      numDistricts, std::vector<int>(numGenotypes, 0));
  monthly_genome_data.weighted_occurrences.assign(
      numDistricts, std::vector<double>(numGenotypes, 0));
}
void SQLiteDistrictReporter::collect_genome_data_for_a_person(Person* person,
                                                              int site) {
  const auto numGenotypes = Model::CONFIG->number_of_parasite_types();
  auto individual = std::vector<int>(numGenotypes, 0);
  // Get the person, press on if they are not infected
  auto* parasites = person->all_clonal_parasite_populations()->parasites();
  auto numClones = parasites->size();
  if (numClones == 0) { return; }

  // Note the age and clinical status of the person
  auto age = person->age();
  auto clinical =
      static_cast<int>(person->host_state() == Person::HostStates::CLINICAL);

  // Count the genotypes present in the individual
  for (unsigned int ndx = 0; ndx < numClones; ndx++) {
    auto* parasitePopulation = (*parasites)[ndx];
    auto genotypeId = parasitePopulation->genotype()->genotype_id();
    monthly_genome_data.occurrences[site][genotypeId]++;
    monthly_genome_data.occurrences_0_5[site][genotypeId] += (age <= 5) ? 1 : 0;
    monthly_genome_data.occurrences_2_10[site][genotypeId] +=
        (age >= 2 && age <= 10) ? 1 : 0;
    individual[genotypeId]++;

    // Count a clinical occurrence if the individual has clinical
    // symptoms
    monthly_genome_data.clinical_occurrences[site][genotypeId] += clinical;
  }

  // Update the weighted occurrences and reset the individual count
  for (unsigned int ndx = 0; ndx < numGenotypes; ndx++) {
    if (individual[ndx] == 0) { continue; }
    monthly_genome_data.weighted_occurrences[site][ndx] +=
        (individual[ndx] / static_cast<double>(numClones));
  }
}

void SQLiteDistrictReporter::build_up_genome_data_insert_values(int monthId) {
  auto numGenotypes = Model::CONFIG->number_of_parasite_types();
  auto numDistricts = SpatialData::get_instance().get_district_count();

  insert_values.clear();
  // Iterate over the districts and append the query
  std::string insertGenotypes;
  std::string updateInfections;
  for (auto district = 0; district < numDistricts; district++) {
    if (monthly_site_data.infections_by_district[district] == 0) { continue; }

    for (auto genotype = 0; genotype < numGenotypes; genotype++) {
      if (monthly_genome_data.weighted_occurrences[district][genotype] == 0) {
        continue;
      }
      std::string singleRow = fmt::format(
          "({}, {}, {}, {}, {}, {}, {}, {})", monthId,
          SpatialData::get_instance()
              .adjust_simulation_district_to_raster_index(district),
          genotype, monthly_genome_data.occurrences[district][genotype],
          monthly_genome_data.clinical_occurrences[district][genotype],
          monthly_genome_data.occurrences_0_5[district][genotype],
          monthly_genome_data.occurrences_2_10[district][genotype],
          monthly_genome_data.weighted_occurrences[district][genotype]);

      insert_values.push_back(singleRow);
    }
  }
}

void SQLiteDistrictReporter::monthly_report_genome_data(int monthId) {
  TransactionGuard transaction{db.get()};

  // Cache some values
  auto numGenotypes = Model::CONFIG->number_of_parasite_types();
  auto numDistricts = SpatialData::get_instance().get_district_count();
  auto* index =
      Model::POPULATION->get_person_index<PersonIndexByLocationStateAgeClass>();

  reset_genome_data_structures(numDistricts, numGenotypes);

  // Iterate over all the possible states
  for (auto location = 0; location < index->vPerson().size(); location++) {
    collect_genome_data_for_location(location);
  }

  build_up_genome_data_insert_values(monthId);

  if (insert_values.empty()) {
    LOG(INFO) << "No genotypes recorded in the simulation at timestep, "
              << Model::SCHEDULER->current_time();
    return;
  }

  insert_monthly_genome_data(insert_values);
}

