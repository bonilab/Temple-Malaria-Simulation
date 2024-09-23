/*
 * ProgressToClinicalEvent.cpp
 *
 * Move the individual from being infected to having a clinical case of malaria,
 * also test to see if they will seek treatment, and trigger any record keeping.
 */
#include "ProgressToClinicalEvent.h"

#include "Core/Config/Config.h"
#include "Core/Random.h"
#include "Core/Scheduler.h"
#include "Events/ReportTreatmentFailureDeathEvent.h"
#include "MDC/MainDataCollector.h"
#include "Model.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/Person.h"
#include "Population/Population.h"
#include "Strategies/IStrategy.h"
#include "Strategies/NestedMFTStrategy.h"

OBJECTPOOL_IMPL(ProgressToClinicalEvent)

ProgressToClinicalEvent::ProgressToClinicalEvent()
    : clinical_caused_parasite_(nullptr) {}

ProgressToClinicalEvent::~ProgressToClinicalEvent() = default;

bool should_receive_treatment(Person* person) {
  const auto rand_p = Model::RANDOM->random_flat(0.0, 1.0);
  const auto p_treatment =
      Model::TREATMENT_COVERAGE->get_probability_to_be_treated(
          person->location(), person->age());
  return rand_p <= p_treatment;
}

void handle_no_treatment(Person* person) {
  // Did not receive treatment
  Model::MAIN_DATA_COLLECTOR->record_1_non_treated_case(person->location(),
                                                        person->age_class());

  if (person->will_progress_to_death_when_receive_no_treatment()) {
    person->cancel_all_events_except(nullptr);
    person->set_host_state(Person::DEAD);
    Model::MAIN_DATA_COLLECTOR->record_1_malaria_death(person->location(),
                                                       person->age_class());
  }
}

std::tuple<Therapy*, bool> ProgressToClinicalEvent::determine_therapy(
    Person* person, bool is_recurrence) {
  auto* strategy = dynamic_cast<NestedMFTStrategy*>(Model::TREATMENT_STRATEGY);
  if (strategy != nullptr) {
    // if the strategy is NestedMFT and the therapy is the public sector
    const auto probability = Model::RANDOM->random_flat(0.0, 1.0);

    double sum = 0;
    std::size_t s_id = -1;
    for (std::size_t i = 0; i < strategy->distribution.size(); i++) {
      sum += strategy->distribution[i];
      if (probability <= sum) {
        s_id = i;
        break;
      }
    }
    // this is public sector
    if (s_id == 0) {
      if (is_recurrence && Model::CONFIG->recurrence_therapy_id() != -1) {
        return {
            Model::CONFIG->therapy_db()[Model::CONFIG->recurrence_therapy_id()],
            false};
      }
      return {strategy->strategy_list[s_id]->get_therapy(person), true};
    }
    return {strategy->strategy_list[s_id]->get_therapy(person), false};
  }

  // main strategy is not NestedMFT or public private sector strategies
  if (Model::CONFIG->recurrence_therapy_id() != -1) {
    return {Model::CONFIG->therapy_db()[Model::CONFIG->recurrence_therapy_id()],
            false};
  }
  return {Model::TREATMENT_STRATEGY->get_therapy(person), true};
}
void ProgressToClinicalEvent::apply_therapy(Person* person, Therapy* therapy,
                                            bool is_public_sector) {
  person->receive_therapy(therapy, clinical_caused_parasite_, false,
                          is_public_sector);

  clinical_caused_parasite_->set_update_function(
      Model::MODEL->having_drug_update_function());

  person->schedule_update_by_drug_event(clinical_caused_parasite_);
  // Check if the person will progress to death despite treatment, this should
  // be 90% lower than no treatment
  if (person->will_progress_to_death_when_receive_treatment()) {
    person->cancel_all_events_except(nullptr);
    person->set_host_state(Person::DEAD);
    Model::MAIN_DATA_COLLECTOR->record_1_malaria_death(person->location(),
                                                       person->age_class());
    ReportTreatmentFailureDeathEvent::schedule_event(
        Model::SCHEDULER, person, therapy->id(),
        Model::SCHEDULER->current_time() + Model::CONFIG->tf_testing_day());
    return;
  }
}

void ProgressToClinicalEvent::transition_to_clinical_state(Person* person) {
  // std::cout << "Transition to clinical state" << std::endl;
  const auto density = Model::RANDOM->random_uniform_double(
      Model::CONFIG->parasite_density_level()
          .log_parasite_density_clinical_from,
      Model::CONFIG->parasite_density_level().log_parasite_density_clinical_to);
  // std::cout << "day: " << Model::SCHEDULER->current_time()
  //           << " - density: " << density << std::endl;
  clinical_caused_parasite_->set_last_update_log10_parasite_density(density);

  // Person change state to Clinical
  person->set_host_state(Person::CLINICAL);

  // TODO: what is the best option to apply here?
  // on one hand we don't what an individual have multiple clinical episodes
  // consecutively, on the other hand we don't want all the other clinical
  // episode to be cancled (i.e recrudescence epidsodes)
  person->cancel_all_other_progress_to_clinical_events_except(this);

  person->change_all_parasite_update_function(
      Model::MODEL->progress_to_clinical_update_function(),
      Model::MODEL->immunity_clearance_update_function());

  clinical_caused_parasite_->set_update_function(
      Model::MODEL->clinical_update_function());

  // Statistic collect cumulative clinical episodes
  Model::MAIN_DATA_COLLECTOR->collect_1_clinical_episode(person->location(),
                                                         person->age_class());

  if (should_receive_treatment(person)) {
    // std::cout << "Receive treatment" << std::endl;
    // if the lastest public treatment is within 30 days
    // and the person decided to go to public sector
    // then the person will receive the recrudenscence therapy

    if (Model::SCHEDULER->current_time()
            - person->lastest_time_received_public_sector_treatment()
        < 30) {
      const auto [therapy, is_public_sector] = determine_therapy(person, true);

      // record  1 treatment for recrudensence
      Model::MAIN_DATA_COLLECTOR->record_1_recrudescence_treatment(
          person->location(), person->age_class(), therapy->id());

      apply_therapy(person, therapy, is_public_sector);
    } else {
      // this is normal routine for clinical cases
      const auto [therapy, is_public_sector] = determine_therapy(person, false);
      // only record for non-recrudenscence treatment
      // Statistic increase today treatments
      Model::MAIN_DATA_COLLECTOR->record_1_treatment(
          person->location(), person->age_class(), therapy->id());

      // TODO: should we only record for the public sector treatment?
      person->schedule_test_treatment_failure_event(
          clinical_caused_parasite_, Model::CONFIG->tf_testing_day(),
          therapy->id());
      apply_therapy(person, therapy, is_public_sector);
    }
  } else {
    handle_no_treatment(person);
  }
  // schedule end clinical event for both treated and non treated cases
  person->schedule_end_clinical_event(clinical_caused_parasite_);
}

void ProgressToClinicalEvent::execute() {
  auto* person = dynamic_cast<Person*>(dispatcher);
  if (person->all_clonal_parasite_populations()->size() == 0) {
    // parasites might be cleaned by immune system or other things else
    return;
  }

  // if the clinical_caused_parasite eventually removed then do nothing
  if (!person->all_clonal_parasite_populations()->contain(
          clinical_caused_parasite_)) {
    return;
  }

  if (person->host_state() == Person::CLINICAL) {
    // prevent the case that the person has multiple clinical states
    clinical_caused_parasite_->set_update_function(
        Model::MODEL->immunity_clearance_update_function());
    return;
  }

  transition_to_clinical_state(person);
}

void ProgressToClinicalEvent::schedule_event(
    Scheduler* scheduler, Person* person,
    ClonalParasitePopulation* clinical_caused_parasite, const int &time) {
  // Ensure that the scheduler exists
  assert(scheduler != nullptr);

  // Create the event to be added to the queue
  auto* event = new ProgressToClinicalEvent();
  event->dispatcher = person;
  event->set_clinical_caused_parasite(clinical_caused_parasite);
  event->time = time;
  person->add(event);
  scheduler->schedule_individual_event(event);
}

void ProgressToClinicalEvent::receive_no_treatment_routine(Person* person) {}
