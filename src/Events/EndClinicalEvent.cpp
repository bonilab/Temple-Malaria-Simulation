/*
 * File:   EndClinicalEvent.cpp
 * Author: Merlin
 *
 * Created on July 31, 2013, 12:27 PM
 */

#include "EndClinicalEvent.h"

#include "Core/Scheduler.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/ImmuneSystem.h"
#include "Population/Person.h"
#include "Population/SingleHostClonalParasitePopulations.h"

OBJECTPOOL_IMPL(EndClinicalEvent)

EndClinicalEvent::EndClinicalEvent() : clinical_caused_parasite_(nullptr) {}

EndClinicalEvent::~EndClinicalEvent() = default;

void EndClinicalEvent::schedule_event(
    Scheduler* scheduler, Person* person,
    ClonalParasitePopulation* clinical_caused_parasite, const int &time) {
  if (scheduler != nullptr) {
    auto* event = new EndClinicalEvent();
    event->dispatcher = person;
    event->set_clinical_caused_parasite(clinical_caused_parasite);
    event->time = time;

    person->add(event);
    scheduler->schedule_individual_event(event);
  }
}

void EndClinicalEvent::execute() {
  auto* person = dynamic_cast<Person*>(dispatcher);

  if (person->all_clonal_parasite_populations()->size() == 0) {
    person->change_state_when_no_parasite_in_blood();

  } else {
    // still have parasite in blood
    person->immune_system()->set_increase(true);
    person->set_host_state(Person::ASYMPTOMATIC);

    if (person->all_clonal_parasite_populations()->contain(
            clinical_caused_parasite_)) {
      auto log_parasite_density =
          clinical_caused_parasite_->last_update_log10_parasite_density();

      // becase the current model does not have within host dynamics, so we
      // assume that the threshold for the parasite density to re-appear in
      // blood is 100 per uL
      const bool isHigherThanRecrudescenceThreshold = log_parasite_density > 2;
      if (isHigherThanRecrudescenceThreshold) {
        person->determine_symptomatic_recrudescence(clinical_caused_parasite_);
      }
    }
  }
}
