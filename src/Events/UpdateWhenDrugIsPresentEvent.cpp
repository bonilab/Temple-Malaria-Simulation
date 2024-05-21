/*
 * File:   UpdateByHavingDrugEvent.cpp
 * Author: Merlin
 *
 * Created on July 31, 2013, 12:16 PM
 */

#include "UpdateWhenDrugIsPresentEvent.h"

#include "Core/Config/Config.h"
#include "Core/Scheduler.h"
#include "Model.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/DrugsInBlood.h"
#include "Population/Person.h"
#include "Population/SingleHostClonalParasitePopulations.h"

OBJECTPOOL_IMPL(UpdateWhenDrugIsPresentEvent)

UpdateWhenDrugIsPresentEvent::UpdateWhenDrugIsPresentEvent()
    : clinical_caused_parasite_(nullptr) {}

UpdateWhenDrugIsPresentEvent::~UpdateWhenDrugIsPresentEvent() = default;

void UpdateWhenDrugIsPresentEvent::schedule_event(
    Scheduler* scheduler, Person* person,
    ClonalParasitePopulation* clinical_caused_parasite, const int &time) {
  if (scheduler != nullptr) {
    auto* event = new UpdateWhenDrugIsPresentEvent();
    event->dispatcher = person;
    event->set_clinical_caused_parasite(clinical_caused_parasite);
    event->time = time;

    person->add(event);
    scheduler->schedule_individual_event(event);
  }
}

void UpdateWhenDrugIsPresentEvent::execute() {
  auto* person = dynamic_cast<Person*>(dispatcher);

  // Check if there are drugs in the blood
  if (person->drugs_in_blood()->size() > 0) {
    if (person->all_clonal_parasite_populations()->contain(
            clinical_caused_parasite_)
        && person->host_state() == Person::CLINICAL
        && clinical_caused_parasite_->last_update_log10_parasite_density()
               <= Model::CONFIG->parasite_density_level()
                      .log_parasite_density_asymptomatic) {
      person->set_host_state(Person::ASYMPTOMATIC);
    }
    // Schedule update by drug event
    person->schedule_update_by_drug_event(clinical_caused_parasite_);
  } else {
    // no drug in blood, reset the status of drug Update parasite
    for (auto i = 0; i < person->all_clonal_parasite_populations()->size();
         i++) {
      auto* const blood_parasite =
          person->all_clonal_parasite_populations()->parasites()->at(i);
      // Check if the update function matches having drug update function
      if (blood_parasite->update_function()
          == Model::MODEL->having_drug_update_function()) {
        blood_parasite->set_update_function(
            Model::MODEL->immunity_clearance_update_function());
        // person->determine_symptomatic_recrudescence(blood_parasite);
      }
    }
  }
}
