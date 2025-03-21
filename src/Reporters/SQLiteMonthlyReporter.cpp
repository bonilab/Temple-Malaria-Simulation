#include "SQLiteMonthlyReporter.h"

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
void SQLiteMonthlyReporter::initialize(int jobNumber,
                                        const std::string &path) {
  // Inform the user of the reporter type
  if (enable_cell_level_reporting) {
    VLOG(1) << "Using SQLiteDbReporter with aggregation at cell/pixel level AND multiple admin levels.";
  } else {
    VLOG(1) << "Using SQLiteDbReporter with aggregation at multiple admin levels.";
  }

  SQLiteDbReporter::initialize(jobNumber, path);

  int admin_level_count = SpatialData::get_instance().get_admin_level_manager()->get_level_names().size();
  monthly_site_data_by_level.resize(admin_level_count);
  monthly_genome_data_by_level.resize(admin_level_count);
}

void SQLiteMonthlyReporter::count_infections_for_location(int location, int level_id) {
  auto unit_id = SpatialData::get_instance().get_admin_unit(level_id, location);
  auto &ageClasses = Model::CONFIG->age_structure();
  auto* index =
      Model::POPULATION->get_person_index<PersonIndexByLocationStateAgeClass>();

  for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
    for (unsigned int ac = 0; ac < ageClasses.size(); ac++) {
      for (auto &person : index->vPerson()[location][hs][ac]) {
        // Is the individual infected by at least one parasite?
        if (person->all_clonal_parasite_populations()->parasites()->empty()) {
          continue;
        }

        monthly_site_data_by_level[level_id].infections_by_unit[unit_id]++;
      }
    }
  }
}

void SQLiteMonthlyReporter::collect_site_data_for_location(int location, int level_id) {
  // Get admin unit for this location at this level
  auto unit_id = SpatialData::get_instance().get_admin_unit(level_id, location);
  
  auto &ageClasses = Model::CONFIG->age_structure();

  count_infections_for_location(location, level_id);

  auto locationPopulation = static_cast<int>(Model::POPULATION->size(location));
  // Collect the simple data
  monthly_site_data_by_level[level_id].population[unit_id] +=
      static_cast<int>(locationPopulation);

  monthly_site_data_by_level[level_id].clinical_episodes[unit_id] +=
      Model::MAIN_DATA_COLLECTOR
          ->monthly_number_of_clinical_episode_by_location()[location];

  monthly_site_data_by_level[level_id].treatments[unit_id] +=
      Model::MAIN_DATA_COLLECTOR
          ->monthly_number_of_treatment_by_location()[location];
  monthly_site_data_by_level[level_id].treatment_failures[unit_id] +=
      Model::MAIN_DATA_COLLECTOR
          ->monthly_treatment_failure_by_location()[location];
  monthly_site_data_by_level[level_id].nontreatment[unit_id] +=
      Model::MAIN_DATA_COLLECTOR->monthly_nontreatment_by_location()[location];

  for (auto ndx = 0; ndx < ageClasses.size(); ndx++) {
    // Collect the treatment by age class, following the 0-59 month convention
    // for under-5
    if (ageClasses[ndx] < 5) {
      monthly_site_data_by_level[level_id].treatments_under5[unit_id] +=
          Model::MAIN_DATA_COLLECTOR
              ->monthly_number_of_treatment_by_location_age_class()[location]
                                                                   [ndx];
    } else {
      monthly_site_data_by_level[level_id].treatments_over5[unit_id] +=
          Model::MAIN_DATA_COLLECTOR
              ->monthly_number_of_treatment_by_location_age_class()[location]
                                                                   [ndx];
    }

    // collect the clinical episodes by age class
    monthly_site_data_by_level[level_id].clinical_episodes_by_age_class[unit_id][ndx] +=
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
    monthly_site_data_by_level[level_id].eir[unit_id] += (eirLocation * locationPopulation);
    monthly_site_data_by_level[level_id].pfpr_under5[unit_id] +=
        (Model::MAIN_DATA_COLLECTOR->get_blood_slide_prevalence(location, 0, 5)
         * locationPopulation);
    monthly_site_data_by_level[level_id].pfpr2to10[unit_id] +=
        (Model::MAIN_DATA_COLLECTOR->get_blood_slide_prevalence(location, 2, 10)
         * locationPopulation);
    monthly_site_data_by_level[level_id].pfpr_all[unit_id] +=
        (Model::MAIN_DATA_COLLECTOR
             ->blood_slide_prevalence_by_location()[location]
         * locationPopulation);
  }
}

void SQLiteMonthlyReporter::calculate_and_build_up_site_data_insert_values(
    int monthId, int level_id) {
    
  // Get the boundary for this admin level
  const auto* boundary = SpatialData::get_instance().get_admin_level_manager()->get_boundary(
      SpatialData::get_instance().get_admin_level_manager()->get_level_names()[level_id]);
  
  auto min_unit_id = boundary->min_unit_id;
  auto max_unit_id = boundary->max_unit_id;
  
  insert_values.clear();

  for (auto unit_id = min_unit_id; unit_id <= max_unit_id; unit_id++) {
    // Skip units with no population
    if (monthly_site_data_by_level[level_id].population[unit_id] == 0) continue;
    
    double calculatedEir = (monthly_site_data_by_level[level_id].eir[unit_id] != 0)
                               ? (monthly_site_data_by_level[level_id].eir[unit_id]
                                  / monthly_site_data_by_level[level_id].population[unit_id])
                               : 0;
    double calculatedPfprUnder5 =
        (monthly_site_data_by_level[level_id].pfpr_under5[unit_id] != 0)
            ? (monthly_site_data_by_level[level_id].pfpr_under5[unit_id]
               / monthly_site_data_by_level[level_id].population[unit_id])
                  * 100.0
            : 0;
    double calculatedPfpr2to10 =
        (monthly_site_data_by_level[level_id].pfpr2to10[unit_id] != 0)
            ? (monthly_site_data_by_level[level_id].pfpr2to10[unit_id]
               / monthly_site_data_by_level[level_id].population[unit_id])
                  * 100.0
            : 0;
    double calculatedPfprAll = (monthly_site_data_by_level[level_id].pfpr_all[unit_id] != 0)
                                   ? (monthly_site_data_by_level[level_id].pfpr_all[unit_id]
                                      / monthly_site_data_by_level[level_id].population[unit_id])
                                         * 100.0
                                   : 0;

    std::string singleRow = fmt::format(
        "({}, {}, {}, {}", monthId, unit_id,
        monthly_site_data_by_level[level_id].population[unit_id],
        monthly_site_data_by_level[level_id].clinical_episodes[unit_id]);

    // Append clinical episodes by age class
    for (const auto &episodes :
         monthly_site_data_by_level[level_id].clinical_episodes_by_age_class[unit_id]) {
      singleRow += fmt::format(", {}", episodes);
    }

    singleRow += fmt::format(", {}, {}, {}, {}, {}, {}, {}, {}, {}, {})",
                             monthly_site_data_by_level[level_id].treatments[unit_id],
                             calculatedEir, calculatedPfprUnder5,
                             calculatedPfpr2to10, calculatedPfprAll,
                             monthly_site_data_by_level[level_id].infections_by_unit[unit_id],
                             monthly_site_data_by_level[level_id].treatment_failures[unit_id],
                             monthly_site_data_by_level[level_id].nontreatment[unit_id],
                             monthly_site_data_by_level[level_id].treatments_under5[unit_id],
                             monthly_site_data_by_level[level_id].treatments_over5[unit_id]);

    insert_values.push_back(singleRow);
  }
}

// Collect and store monthly site data
// Aggregates data related to various site metrics and stores them in the
// database
void SQLiteMonthlyReporter::monthly_report_site_data(int monthId) {
  TransactionGuard transaction{db.get()};
  
  // Check if cell-level reporting is enabled
  if (enable_cell_level_reporting) {
    monthly_report_cell_site_data(monthId);
  }
  
  // Get admin levels count
  int admin_level_count = SpatialData::get_instance().get_admin_level_manager()->get_level_names().size();
  
  // For each admin level
  for (int level_id = 0; level_id < admin_level_count; level_id++) {
    // Get the boundary data for this admin level
    const auto* boundary = SpatialData::get_instance().get_admin_level_manager()->get_boundary(
        SpatialData::get_instance().get_admin_level_manager()->get_level_names()[level_id]);
    
    // Calculate vector size for this admin level
    int vectorSize = boundary->max_unit_id + 1;
    auto &ageClasses = Model::CONFIG->age_structure();
    
    // Reset data structures for this admin level
    reset_site_data_structures(level_id, vectorSize, ageClasses.size());
    
    // Collect data for this admin level
    for (auto location = 0; location < Model::CONFIG->number_of_locations(); location++) {
      auto locationPopulation = static_cast<int>(Model::POPULATION->size(location));
      if (locationPopulation == 0) continue;
      
      collect_site_data_for_location(location, level_id);
    }
    
    // Calculate and insert data for this admin level
    calculate_and_build_up_site_data_insert_values(monthId, level_id);
    insert_monthly_site_data(level_id, insert_values);
  }
}

void SQLiteMonthlyReporter::collect_genome_data_for_location(size_t location, int level_id) {
  auto unit_id = SpatialData::get_instance().get_admin_unit(level_id, location);
  auto* index =
      Model::POPULATION->get_person_index<PersonIndexByLocationStateAgeClass>();
  auto ageClasses = index->vPerson()[0][0].size();

  for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
    // Iterate over all the age classes
    for (unsigned int ac = 0; ac < ageClasses; ac++) {
      // Iterate over all the genotypes
      auto peopleInAgeClass = index->vPerson()[location][hs][ac];
      for (auto &person : peopleInAgeClass) {
        collect_genome_data_for_a_person(person, unit_id, level_id);
      }
    }
  }
}

void SQLiteMonthlyReporter::reset_site_data_structures(int level_id, int vectorSize,
                                                        size_t numAgeClasses) {
  // reset the data structures
  monthly_site_data_by_level[level_id].eir.assign(vectorSize, 0);
  monthly_site_data_by_level[level_id].pfpr_under5.assign(vectorSize, 0);
  monthly_site_data_by_level[level_id].pfpr2to10.assign(vectorSize, 0);
  monthly_site_data_by_level[level_id].pfpr_all.assign(vectorSize, 0);
  monthly_site_data_by_level[level_id].population.assign(vectorSize, 0);
  monthly_site_data_by_level[level_id].clinical_episodes.assign(vectorSize, 0);
  monthly_site_data_by_level[level_id].clinical_episodes_by_age_class.assign(
      vectorSize, std::vector<int>(numAgeClasses, 0));
  monthly_site_data_by_level[level_id].treatments.assign(vectorSize, 0);
  monthly_site_data_by_level[level_id].treatment_failures.assign(vectorSize, 0);
  monthly_site_data_by_level[level_id].nontreatment.assign(vectorSize, 0);
  monthly_site_data_by_level[level_id].treatments_under5.assign(vectorSize, 0);
  monthly_site_data_by_level[level_id].treatments_over5.assign(vectorSize, 0);
  monthly_site_data_by_level[level_id].infections_by_unit.assign(vectorSize, 0);
}

void SQLiteMonthlyReporter::reset_genome_data_structures(int level_id, int vectorSize,
                                                          size_t numGenotypes) {
  // reset the data structures
  monthly_genome_data_by_level[level_id].occurrences.assign(vectorSize,
                                         std::vector<int>(numGenotypes, 0));
  monthly_genome_data_by_level[level_id].clinical_occurrences.assign(
      vectorSize, std::vector<int>(numGenotypes, 0));
  monthly_genome_data_by_level[level_id].occurrences_0_5.assign(vectorSize,
                                             std::vector<int>(numGenotypes, 0));
  monthly_genome_data_by_level[level_id].occurrences_2_10.assign(
      vectorSize, std::vector<int>(numGenotypes, 0));
  monthly_genome_data_by_level[level_id].weighted_occurrences.assign(
      vectorSize, std::vector<double>(numGenotypes, 0));
      
}

void SQLiteMonthlyReporter::collect_genome_data_for_a_person(Person* person,
                                                             int unit_id, 
                                                             int level_id) {
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
    monthly_genome_data_by_level[level_id].occurrences[unit_id][genotypeId]++;
    monthly_genome_data_by_level[level_id].occurrences_0_5[unit_id][genotypeId] += (age <= 5) ? 1 : 0;
    monthly_genome_data_by_level[level_id].occurrences_2_10[unit_id][genotypeId] +=
        (age >= 2 && age <= 10) ? 1 : 0;
    individual[genotypeId]++;

    // Count a clinical occurrence if the individual has clinical
    // symptoms
    monthly_genome_data_by_level[level_id].clinical_occurrences[unit_id][genotypeId] += clinical;
  }

  // Update the weighted occurrences and reset the individual count
  for (unsigned int ndx = 0; ndx < numGenotypes; ndx++) {
    if (individual[ndx] == 0) { continue; }
    monthly_genome_data_by_level[level_id].weighted_occurrences[unit_id][ndx] +=
        (individual[ndx] / static_cast<double>(numClones));
  }
}

void SQLiteMonthlyReporter::build_up_genome_data_insert_values(int monthId, int level_id) {
  auto numGenotypes = Model::CONFIG->number_of_parasite_types();
  
  // Get the boundary for this admin level
  const auto* boundary = SpatialData::get_instance().get_admin_level_manager()->get_boundary(
      SpatialData::get_instance().get_admin_level_manager()->get_level_names()[level_id]);
  
  auto min_unit_id = boundary->min_unit_id;
  auto max_unit_id = boundary->max_unit_id;

  insert_values.clear();
  
  // Iterate over the admin units and append the query
  for (auto unit_id = min_unit_id; unit_id <= max_unit_id; unit_id++) {
    // Skip if there are no infections in this unit
    if (monthly_site_data_by_level[level_id].infections_by_unit[unit_id] == 0) { continue; }

    for (auto genotype = 0; genotype < numGenotypes; genotype++) {
      if (monthly_genome_data_by_level[level_id].weighted_occurrences[unit_id][genotype] == 0) {
        continue;
      }
      std::string singleRow = fmt::format(
          "({}, {}, {}, {}, {}, {}, {}, {})", monthId, unit_id,
          genotype, monthly_genome_data_by_level[level_id].occurrences[unit_id][genotype],
          monthly_genome_data_by_level[level_id].clinical_occurrences[unit_id][genotype],
          monthly_genome_data_by_level[level_id].occurrences_0_5[unit_id][genotype],
          monthly_genome_data_by_level[level_id].occurrences_2_10[unit_id][genotype],
          monthly_genome_data_by_level[level_id].weighted_occurrences[unit_id][genotype]);

      insert_values.push_back(singleRow);
    }
  }
}

void SQLiteMonthlyReporter::monthly_report_genome_data(int monthId) {
  TransactionGuard transaction{db.get()};

  // Check if cell-level reporting is enabled
  if (enable_cell_level_reporting) {
    monthly_report_cell_genome_data(monthId);
  }

  // Get admin levels count
  int admin_level_count = SpatialData::get_instance().get_admin_level_manager()->get_level_names().size();
  
  // For each admin level
  for (int level_id = 0; level_id < admin_level_count; level_id++) {
    // Get the boundary data for this admin level
    const auto* boundary = SpatialData::get_instance().get_admin_level_manager()->get_boundary(
        SpatialData::get_instance().get_admin_level_manager()->get_level_names()[level_id]);
    
    // Calculate vector size for this admin level
    int vectorSize = boundary->max_unit_id + 1;
    auto numGenotypes = Model::CONFIG->number_of_parasite_types();
    
    reset_genome_data_structures(level_id, vectorSize, numGenotypes);
    
    auto* index = Model::POPULATION->get_person_index<PersonIndexByLocationStateAgeClass>();
    
    // Iterate over all locations
    for (auto location = 0; location < index->vPerson().size(); location++) {
      collect_genome_data_for_location(location, level_id);
    }
    
    build_up_genome_data_insert_values(monthId, level_id);
    
    if (insert_values.empty()) {
      LOG(INFO) << "No genotypes recorded in the simulation at timestep, "
                << Model::SCHEDULER->current_time();
      continue;
    }
    
    insert_monthly_genome_data(level_id, insert_values);
  }
}

// New method for cell-level site data reporting
void SQLiteMonthlyReporter::monthly_report_cell_site_data(int monthId) {
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
    auto &ageClasses = Model::CONFIG->age_structure();
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

    singleRow +=
        fmt::format(", {}, {}, {}, {}, {}, {}, {}, {}, {}, {})",
                    Model::MAIN_DATA_COLLECTOR
                        ->monthly_number_of_treatment_by_location()[location],
                    eir, pfprUnder5, pfpr2to10, pfprAll, infectedIndividuals,
                    Model::MAIN_DATA_COLLECTOR
                        ->monthly_treatment_failure_by_location()[location],
                    Model::MAIN_DATA_COLLECTOR
                        ->monthly_nontreatment_by_location()[location],
                    treatmentsUnder5, treatmentsOver5);

    values.push_back(singleRow);
  }

  insert_monthly_site_data(CELL_LEVEL_ID, values);
}

// New method for cell-level genome data reporting
void SQLiteMonthlyReporter::monthly_report_cell_genome_data(int monthId) {
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
  insert_monthly_genome_data(CELL_LEVEL_ID, insertValues);
}

