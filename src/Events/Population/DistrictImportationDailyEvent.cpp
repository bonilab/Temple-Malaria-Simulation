#include "DistrictImportationDailyEvent.h"

#include "Core/Config/Config.h"
#include "Core/Random.h"
#include "Core/Scheduler.h"
#include "Model.h"
#include "Population/Population.h"
#include "Population/Properties/PersonIndexByLocationStateAgeClass.h"

DistrictImportationDailyEvent::DistrictImportationDailyEvent(
    int district, int locus, int mutantAllele, double dailyRate, int startDay)
    : district_(district),
      locus_(locus),
      mutant_allele_(mutantAllele),
      daily_rate_(dailyRate) {
  time = startDay;
}

void DistrictImportationDailyEvent::schedule_event(Scheduler* scheduler,
                                                   int district, int locus,
                                                   int mutantAllele,
                                                   double dailyRate,
                                                   int startDay) {
  if (scheduler != nullptr) {
    auto* event = new DistrictImportationDailyEvent(
        district, locus, mutantAllele, dailyRate, startDay);
    event->dispatcher = nullptr;
    event->time = startDay;
    scheduler->schedule_population_event(event);
  }
}

void DistrictImportationDailyEvent::execute() {
  // std::cout << date::year_month_day{ Model::SCHEDULER->calendar_date } <<
  // ":import periodically event" << std::endl;
  // schedule importation for the next day
  schedule_event(Model::SCHEDULER, district_, locus_, mutant_allele_,
                 daily_rate_, Model::SCHEDULER->current_time() + 1);

  auto number_of_importation_cases = Model::RANDOM->random_poisson(daily_rate_);

  if (number_of_importation_cases == 0) { return; }


  const auto &locations = SpatialData::get_instance().get_locations_in_unit("district", district_);

  auto* pi =
      Model::POPULATION->get_person_index<PersonIndexByLocationStateAgeClass>();

  std::vector<double> infected_cases_by_location(locations.size(), 0);

  for (auto i = 0; i < locations.size(); i++) {
    auto location = locations[i];

    for (auto ac = 0; ac < Model::CONFIG->number_of_age_classes(); ac++) {
      // only select state clinical or asymptomatic
      infected_cases_by_location[i] +=
          pi->vPerson()[location][Person::ASYMPTOMATIC][ac].size()
          + pi->vPerson()[location][Person::CLINICAL][ac].size();
    }
  }
  // use multinomial distribution to distribute the number of importation cases
  // to the locations
  std::vector<uint> importation_cases_by_location(locations.size(), 0);
  Model::RANDOM->random_multinomial(
      locations.size(), number_of_importation_cases,
      infected_cases_by_location.data(), importation_cases_by_location.data());
  for (auto i = 0; i < locations.size(); i++) {
    if (infected_cases_by_location[i] == 0) { continue; }
    if (importation_cases_by_location[i] == 0) { continue; }
    auto location = locations[i];
    auto number_of_importation_cases = importation_cases_by_location[i];

    for (auto i = 0; i < number_of_importation_cases; i++) {
      auto ac =
          Model::RANDOM->random_uniform(Model::CONFIG->number_of_age_classes());
      auto hs = Model::RANDOM->random_uniform(2) + Person::ASYMPTOMATIC;

      auto max_retry = 10;
      while (pi->vPerson()[location][hs][ac].empty() && max_retry > 0) {
        // redraw if the selected state and age class is empty
        ac = Model::RANDOM->random_uniform(
            Model::CONFIG->number_of_age_classes());

        hs = Model::RANDOM->random_uniform(2) + Person::ASYMPTOMATIC;
        max_retry--;
      }
      if (max_retry == 0) { continue; }

      auto index = Model::RANDOM->random_uniform(
          static_cast<unsigned long>(pi->vPerson()[location][hs][ac].size()));

      auto* person = pi->vPerson()[location][hs][ac][index];

      // Mutate all the clonal populations the individual is carrying
      for (auto* pp :
           *(person->all_clonal_parasite_populations()->parasites())) {
        auto* old_genotype = pp->genotype();
        auto* new_genotype =
            old_genotype->combine_mutation_to(locus_, mutant_allele_);
        pp->set_genotype(new_genotype);
      }
    }
  }
}

