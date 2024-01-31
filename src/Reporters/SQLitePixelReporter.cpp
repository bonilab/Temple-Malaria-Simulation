/*
 * SQLiteDbReporter.cpp
 *
 * Implementation of the SQLiteDbReporter class.
 */
#include "SQLitePixelReporter.h"

#include <fmt/format.h>

#include "Core/Config/Config.h"
#include "Core/Random.h"
#include "Helpers/StringHelpers.h"
#include "MDC/ModelDataCollector.h"
#include "Model.h"
#include "Population/Population.h"
#include "Population/Properties/PersonIndexByLocationStateAgeClass.h"
#include "easylogging++.h"

void SQLitePixelReporter::initialize(int job_number, std::string path) {

  // Inform the user of the reporter type is pixel / cell level
  VLOG(1)
      << "Using SQLiteDbReporterwith aggregation at the pixel / cell level.";
  SQLiteDbReporter::initialize(job_number, path);

  auto locations = Model::CONFIG->number_of_locations();
}

// Iterate over all the sites and prepare the query for the site specific genome
// data
//
// WARNING This function updates the monthlysitedata with the total count of
// infected individuals, so it MUST
//         be invoked after monthly_site_data to ensure the data is inserted
//         correctly.
void SQLitePixelReporter::monthly_genome_data(int id) {
  // Prepare the data structures
  auto genotypes = Model::CONFIG->number_of_parasite_types();
  std::vector<int> individual(genotypes, 0);

  // Cache some values
  auto *index =
      Model::POPULATION->get_person_index<PersonIndexByLocationStateAgeClass>();
  auto age_classes = index->vPerson()[0][0].size();

  std::vector<std::string> insertValues;

  // Iterate over all the possible locations
  for (auto location = 0; location < Model::CONFIG->number_of_locations();
       location++) {
    std::vector<int> occurrences(
        genotypes, 0); // discrete count of occurrences of the parasite genotype
    std::vector<int> clinicalOccurrences(
        genotypes,
        0); // discrete count of clinical occurrences of the parasite genotype
    std::vector<int> occurrencesZeroToFive(
        genotypes, 0); // discrete count of the occurrences of the parasite
                       // genotype in individuals 0 - 5
    std::vector<int> occurrencesTwoToTen(
        genotypes, 0); // discrete count of the occurrences of the parasite
                       // genotype in individuals 2 - 10
    std::vector<double> weightedOccurrences(
        genotypes, 0.0); // weighted occurrences of the genotype
    int infectedIndividuals =
        0; // discrete count of infected individuals in the location

    // Iterate over all the possible states
    for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
      // Iterate over all the age classes
      for (unsigned int ac = 0; ac < age_classes; ac++) {
        // Iterate over all the genotypes
        auto age_class = index->vPerson()[location][hs][ac];
        for (auto &person : age_class) {

          // Get the person, press on if they are not infected (i.e., no
          // parasites)
          auto parasites =
              person->all_clonal_parasite_populations()->parasites();
          auto size = parasites->size();
          if (size == 0) {
            continue;
          }

          // Note the age and clinical status of the person
          auto age = person->age();
          auto clinical =
              (int)(person->host_state() == Person::HostStates::CLINICAL);

          // Update count of infected individuals
          infectedIndividuals += 1;

          // Count the genotypes present in the individuals
          for (unsigned int ndx = 0; ndx < size; ndx++) {
            auto parasite_population = (*parasites)[ndx];
            auto genotype_id = parasite_population->genotype()->genotype_id();
            occurrences[genotype_id]++;
            occurrencesZeroToFive[genotype_id] += (age <= 5);
            occurrencesTwoToTen[genotype_id] += (age >= 2 && age <= 10);
            individual[genotype_id]++;

            // Count a clinical occurrence if the individual has clinical
            // symptoms
            clinicalOccurrences[genotype_id] += clinical;
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
        if (weightedOccurrences[genotype] == 0) {
          continue;
        }

        std::string singleRow = fmt::format(
            "({}, {}, {}, {}, {}, {}, {}, {})", id, location, genotype,
            occurrences[genotype], clinicalOccurrences[genotype],
            occurrencesZeroToFive[genotype], occurrencesTwoToTen[genotype],
            weightedOccurrences[genotype]);

        insertValues.push_back(singleRow);
      }
    }

    std::string updateQuery = fmt::format(UPDATE_INFECTED_INDIVIDUALS,
                                          infectedIndividuals, id, location);
    db->execute(updateQuery);
  }

  std::string insertQuery =
      INSERT_GENOTYPE_PREFIX + StringHelpers::join(insertValues, ",") + ";";
  db->execute(insertQuery);
}

// Iterate over all the sites and prepare the query for the site specific data
void SQLitePixelReporter::monthly_site_data(int id) {
  /* std::cout << "monthly_site_data" << std::endl; */
  auto age_classes = Model::CONFIG->age_structure();

  // collect data
  std::vector<std::string> values;

  for (auto location = 0; location < Model::CONFIG->number_of_locations();
       location++) {
    // Check the population, if there is nobody there, press on
    if (Model::POPULATION->size(location) == 0) {
      continue;
    }

    // Determine the EIR and PfPR values
    auto eir =
        Model::DATA_COLLECTOR->EIR_by_location_year()[location].empty()
            ? 0
            : Model::DATA_COLLECTOR->EIR_by_location_year()[location].back();
    auto pfpr_under5 =
        Model::DATA_COLLECTOR->get_blood_slide_prevalence(location, 0, 5) *
        100.0;
    auto pfpr_2to10 =
        Model::DATA_COLLECTOR->get_blood_slide_prevalence(location, 2, 10) *
        100.0;
    auto pfpr_all =
        Model::DATA_COLLECTOR->blood_slide_prevalence_by_location()[location] *
        100.0;

    // Collect the treatment by age class, following the 0-59 month convention
    // for under-5
    auto treatments_under5 = 0;
    auto treatments_over5 = 0;
    for (auto ndx = 0; ndx < age_classes.size(); ndx++) {
      if (age_classes[ndx] < 5) {
        treatments_under5 +=
            Model::DATA_COLLECTOR
                ->monthly_number_of_treatment_by_location_age_class()[location]
                                                                     [ndx];
      } else {
        treatments_over5 +=
            Model::DATA_COLLECTOR
                ->monthly_number_of_treatment_by_location_age_class()[location]
                                                                     [ndx];
      }
    }

    std::string singleRow = fmt::format(
        "({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {})", id, location,
        Model::POPULATION->size(location),
        Model::DATA_COLLECTOR
            ->monthly_number_of_clinical_episode_by_location()[location],
        Model::DATA_COLLECTOR
            ->monthly_number_of_treatment_by_location()[location],
        eir, pfpr_under5, pfpr_2to10, pfpr_all,
        Model::DATA_COLLECTOR
            ->monthly_treatment_failure_by_location()[location],
        Model::DATA_COLLECTOR->monthly_nontreatment_by_location()[location],
        treatments_under5, treatments_over5);
    values.push_back(singleRow);
  }

  std::string query =
      INSERT_SITE_PREFIX + StringHelpers::join(values, ",") + ";";
  db->execute(query);
}

// Update the monthly count of infected individuals.
void SQLitePixelReporter::monthly_infected_individuals(int id) {
  /* std::cout << "monthly_infected_individuals" << std::endl; */
  // Cache some values
  auto *index =
      Model::POPULATION->get_person_index<PersonIndexByLocationStateAgeClass>();
  auto age_classes = index->vPerson()[0][0].size();

  // Iterate overall of the possible locations
  for (auto location = 0; location < Model::CONFIG->number_of_locations();
       location++) {
    int infected_individuals = 0;
    // Iterate over all the possible states
    for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
      // Iterate over all the age classes
      for (unsigned int ac = 0; ac < age_classes; ac++) {
        // Iterate over all the genotypes
        auto age_class = index->vPerson()[location][hs][ac];
        for (auto &person : age_class) {

          // Update the count if the individual is infected
          if (!person->all_clonal_parasite_populations()
                   ->parasites()
                   ->empty()) {
            infected_individuals++;
          }
        }
      }
    }

    std::string updateQuery = fmt::format(UPDATE_INFECTED_INDIVIDUALS,
                                          infected_individuals, id, location);
    db->execute(updateQuery);
  }
}
