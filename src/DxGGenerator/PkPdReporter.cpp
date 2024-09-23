/*
 * File:   PkPdReporter.cpp
 * Author: Merlin
 *
 * Created on October 29, 2014, 12:56 PM
 */

#include "PkPdReporter.h"

#include "Core/Config/Config.h"
#include "Core/Random.h"
#include "MDC/MainDataCollector.h"
#include "Model.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/Person.h"
#include "Population/Population.h"
#include "Population/Properties/PersonIndexAll.h"
#include "Population/SingleHostClonalParasitePopulations.h"

PkPdReporter::PkPdReporter() = default;

PkPdReporter::~PkPdReporter() = default;

void PkPdReporter::initialize(int /*job_number*/, const std::string &path) {
  prefix = path;
}

void PkPdReporter::before_run() {
  //    std::cout << Model::RANDOM->seed() << std::endl;
  parasitaemia_file.open(fmt::format("{}_parasitaemia.csv", prefix),
                         std::ios::out);
  parasitaemia_file << "time,individual,recrudescence,parasitaemia"
                    << std::endl;
}

void PkPdReporter::begin_time_step() {
  Model::MAIN_DATA_COLLECTOR->perform_population_statistic();

  // fake pfpr for recrudescence
  Model::MAIN_DATA_COLLECTOR->blood_slide_prevalence_by_location()[0] = 0.1;

  if (Model::SCHEDULER->current_time() % Model::CONFIG->report_frequency()
      == 0) {
    auto curent_time = Model::SCHEDULER->current_time();
    //
    // std::cout << Model::SCHEDULER->current_time() << "\t";
    // std::cout <<
    // Model::MAIN_DATA_COLLECTOR->number_of_positive_by_location()[0]
    //                  * 100
    //                  / static_cast<double>(
    //                      Model::POPULATION->popsize_by_location()[0])
    //           << "\t";
    //
    for (auto i = 0; i < Model::POPULATION->all_persons()->vPerson().size();
         i++) {
      auto* person = Model::POPULATION->all_persons()->vPerson()[i];
      auto recrudescence_state = person->recrudescence_status;
      auto parasitaemia = 0.0;
      if (person->all_clonal_parasite_populations()->parasites()->size() >= 1) {
        parasitaemia = person->all_clonal_parasite_populations()
                           ->parasites()
                           ->at(0)
                           ->last_update_log10_parasite_density();

      } else {
        parasitaemia =
            Model::CONFIG->parasite_density_level().log_parasite_density_cured;
      }

      parasitaemia_file << curent_time << "," << i << "," << recrudescence_state
                        << "," << parasitaemia << std::endl;
    }
  }
}

void PkPdReporter::monthly_report() {}

void PkPdReporter::after_run() {
  Model::MAIN_DATA_COLLECTOR->update_after_run();
  parasitaemia_file.close();

  //    std::cout << 1-Model::DATA_COLLECTOR->current_TF_by_location()[0] <<
  //    std::endl;
  // std::cout <<
  // Model::DATA_COLLECTOR->total_number_of_treatments_60_by_location()[0][1] <<
  // std::endl;
}
