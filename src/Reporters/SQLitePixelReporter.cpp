/*
 * SQLiteDbReporter.cpp
 *
 * Implementation of the SQLiteDbReporter class.
 */
#include "SQLitePixelReporter.h"

#include <fmt/format.h>

#include "Core/Config/Config.h"
#include "Core/Random.h"
#include "MDC/MainDataCollector.h"
#include "Model.h"
#include "Population/Population.h"
#include "Population/Properties/PersonIndexByLocationStateAgeClass.h"
#include "easylogging++.h"

void SQLitePixelReporter::initialize(int jobNumber, const std::string &path) {
  // Inform the user of the reporter type is pixel / cell level
  VLOG(1)
      << "Using SQLiteDbReporterwith aggregation at the pixel / cell level.";
  SQLiteDbReporter::initialize(jobNumber, path);
}

// Iterate over all the sites and prepare the query for the site specific data
void SQLitePixelReporter::monthly_report_site_data(int monthId) {
  TransactionGuard transaction{db.get()};
  /* std::cout << "monthly_site_data" << std::endl; */
  auto &ageClasses = Model::CONFIG->age_structure();

  // collect data
  std::vector<std::string> values;

  for (auto location = 0; location < Model::CONFIG->number_of_locations();
       location++) {
    // Check the population, if there is nobody there, press on
    if (Model::POPULATION->size(location) == 0) { continue; }

    // count the number of infected individuals by location
    auto* index = Model::POPULATION
                      ->get_person_index<PersonIndexByLocationStateAgeClass>();
    auto numAgeClasses = index->vPerson()[0][0].size();
    if (numAgeClasses != ageClasses.size()) {
      throw std::invalid_argument(
          "The number of age classes in the population does not match the "
          "number of age classes in the configuration.");
    }
    int infectedIndividuals = 0;
    // Iterate over all the possible states
    for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
      // Iterate over all the age classes
      for (unsigned int ac = 0; ac < numAgeClasses; ac++) {
        // Iterate over all the genotypes
        auto peopleInAgeClass = index->vPerson()[location][hs][ac];
        for (auto &person : peopleInAgeClass) {
          // Update the count if the individual is infected
          if (!person->all_clonal_parasite_populations()
                   ->parasites()
                   ->empty()) {
            infectedIndividuals++;
          }
        }
      }
    }

    // Determine the EIR and PfPR values
    auto eir =
        Model::MAIN_DATA_COLLECTOR->EIR_by_location_year()[location].empty()
            ? 0
            : Model::MAIN_DATA_COLLECTOR->EIR_by_location_year()[location]
                  .back();
    auto pfprUnder5 =
        Model::MAIN_DATA_COLLECTOR->get_blood_slide_prevalence(location, 0, 5)
        * 100.0;
    auto pfpr2to10 =
        Model::MAIN_DATA_COLLECTOR->get_blood_slide_prevalence(location, 2, 10)
        * 100.0;
    auto pfprAll = Model::MAIN_DATA_COLLECTOR
                       ->blood_slide_prevalence_by_location()[location]
                   * 100.0;

    // Collect the treatment by age class, following the 0-59 month convention
    // for under-5
    auto treatmentsUnder5 = 0;
    auto treatmentsOver5 = 0;
    for (auto ndx = 0; ndx < ageClasses.size(); ndx++) {
      if (ageClasses[ndx] < 5) {
        treatmentsUnder5 +=
            Model::MAIN_DATA_COLLECTOR
                ->monthly_number_of_treatment_by_location_age_class()[location]
                                                                     [ndx];
      } else {
        treatmentsOver5 +=
            Model::MAIN_DATA_COLLECTOR
                ->monthly_number_of_treatment_by_location_age_class()[location]
                                                                     [ndx];
      }
    }

    std::string singleRow = fmt::format(
        "({}, {}, {}, {}", monthId, location, Model::POPULATION->size(location),
        Model::MAIN_DATA_COLLECTOR
            ->monthly_number_of_clinical_episode_by_location()[location]);

    for (const auto &episodes :
         Model::MAIN_DATA_COLLECTOR
             ->monthly_number_of_clinical_episode_by_location_age_class()
                 [location]) {
      singleRow += fmt::format(", {}", episodes);
    }

    singleRow += fmt::format(
        ", {}, {}, {}, {}, {}, {}, {}, {}, {}, {},{})",
        Model::MAIN_DATA_COLLECTOR
            ->monthly_number_of_treatment_by_location()[location],
        Model::MAIN_DATA_COLLECTOR
            ->monthly_number_of_recrudescence_treatment_by_location()[location],
        eir, pfprUnder5, pfpr2to10, pfprAll, infectedIndividuals,
        Model::MAIN_DATA_COLLECTOR
            ->monthly_treatment_failure_by_location()[location],
        Model::MAIN_DATA_COLLECTOR
            ->monthly_nontreatment_by_location()[location],
        treatmentsUnder5, treatmentsOver5);

    values.push_back(singleRow);
  }

  insert_monthly_site_data(values);
}

// Iterate over all the sites and prepare the query for the site specific genome
// data
void SQLitePixelReporter::monthly_report_genome_data(int monthId) {
  TransactionGuard transaction{db.get()};

  // Prepare the data structures
  auto genotypes = Model::CONFIG->number_of_parasite_types();
  std::vector<int> individual(genotypes, 0);

  // Cache some values
  auto* index =
      Model::POPULATION->get_person_index<PersonIndexByLocationStateAgeClass>();
  auto numAgeClasses = index->vPerson()[0][0].size();

  std::vector<std::string> insertValues;

  // Iterate over all the possible locations
  for (auto location = 0; location < Model::CONFIG->number_of_locations();
       location++) {
    std::vector<int> occurrences(genotypes, 0);
    std::vector<int> clinicalOccurrences(genotypes, 0);
    std::vector<int> occurrencesZeroToFive(genotypes, 0);
    std::vector<int> occurrencesTwoToTen(genotypes, 0);
    std::vector<double> weightedOccurrences(genotypes, 0.0);
    int infectedIndividuals = 0;

    // Iterate over all the possible states
    for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
      // Iterate over all the age classes
      for (unsigned int ac = 0; ac < numAgeClasses; ac++) {
        // Iterate over all the genotypes
        auto peopleInAgeClass = index->vPerson()[location][hs][ac];
        for (auto &person : peopleInAgeClass) {
          // Get the person, press on if they are not infected (i.e., no
          // parasites)
          auto* parasites =
              person->all_clonal_parasite_populations()->parasites();
          auto size = parasites->size();
          if (size == 0) { continue; }

          // Note the age and clinical status of the person
          auto age = person->age();
          auto clinical = static_cast<int>(person->host_state()
                                           == Person::HostStates::CLINICAL);

          // Update count of infected individuals
          infectedIndividuals++;

          // Count the genotypes present in the individuals
          for (unsigned int ndx = 0; ndx < size; ndx++) {
            auto* parasitePopulation = (*parasites)[ndx];
            auto genotypeId = parasitePopulation->genotype()->genotype_id();
            occurrences[genotypeId]++;
            occurrencesZeroToFive[genotypeId] += (age <= 5) ? 1 : 0;
            occurrencesTwoToTen[genotypeId] += (age >= 2 && age <= 10) ? 1 : 0;
            individual[genotypeId]++;

            // Count a clinical occurrence if the individual has clinical
            // symptoms
            clinicalOccurrences[genotypeId] += clinical;
          }

          // Update the weighted occurrences and reset the individual count
          auto divisor = static_cast<double>(size);
          for (unsigned int ndx = 0; ndx < genotypes; ndx++) {
            weightedOccurrences[ndx] += (individual[ndx] / divisor);
            individual[ndx] = 0;
          }
        }
      }
    }

    // Prepare and append the query, pass if the genotype was not seen or no
    // infections were seen
    if (infectedIndividuals != 0) {
      for (auto genotype = 0; genotype < genotypes; genotype++) {
        if (weightedOccurrences[genotype] == 0) { continue; }

        std::string const singleRow = fmt::format(
            "({}, {}, {}, {}, {}, {}, {}, {})", monthId, location, genotype,
            occurrences[genotype], clinicalOccurrences[genotype],
            occurrencesZeroToFive[genotype], occurrencesTwoToTen[genotype],
            weightedOccurrences[genotype]);

        insertValues.push_back(singleRow);
      }
    }
  }

  if (insertValues.empty()) {
    LOG(INFO) << "No genotypes recorded in the simulation at timestep, "
              << Model::SCHEDULER->current_time();
    return;
  }
  insert_monthly_genome_data(insertValues);
}

