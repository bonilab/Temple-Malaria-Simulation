/*
 * File:   ReceiveSMCTherapyEvent.cpp
 * Author: Sarit
 *
 * Created on July 7, 2025
 */

#include "ReceiveSMCTherapyEvent.h"

#include "Core/Scheduler.h"
#include "Model.h"
#include "Population/Person.h"
#include "Therapies/Therapy.hxx"
#include "Population/DrugsInBlood.h"
#include "Therapies/Drug.h"

ReceiveSMCTherapyEvent::ReceiveSMCTherapyEvent() : received_therapy_(nullptr) {}

ReceiveSMCTherapyEvent::~ReceiveSMCTherapyEvent() = default;

void ReceiveSMCTherapyEvent::schedule_event(Scheduler* scheduler, Person* p,
                                            Therapy* therapy, const int &time) {
                                      


  if (scheduler != nullptr) {

    // std::cout << "Scheduling ReceiveSMCTherapyEvent for person ID: "
    //         << p->get_uid() << std::endl;



    auto* e = new ReceiveSMCTherapyEvent();
    e->dispatcher = p;
    e->set_received_therapy(therapy);
    e->time = time;

    p->add(e);
    scheduler->schedule_individual_event(e);
  }
}

void ReceiveSMCTherapyEvent::execute() {
  auto* person = dynamic_cast<Person*>(dispatcher);
  // If needed, add eligibility checks (e.g., age checks for SMC) here

  // std::cout << "Executing ReceiveSMCTherapyEvent for person ID: "
  //           << person->get_uid() << std::endl;

  

  //std::cout << "person "<< person->get_uid() << " is receiving therapy "<< std::endl;
  //received_therapy_->print(std::cout);
  person->receive_therapy(received_therapy_, nullptr);

  // for (const auto &kv_drug : *person->drugs_in_blood()->drugs()) {

  //   std::cout <<"Person ID: "<< person->get_uid() <<" Drug ID: "<< kv_drug.first << " Last update value: " <<kv_drug.second->last_update_value() <<std::endl;


  // }

  //person->

  // Cancel progress to clinical events if needed
  person->cancel_all_other_progress_to_clinical_events_except(nullptr);
  person->change_all_parasite_update_function(
      Model::MODEL->progress_to_clinical_update_function(),
      Model::MODEL->immunity_clearance_update_function());

  person->schedule_update_by_drug_event(nullptr);


 
  // for (const auto &kv_drug : *person->drugs_in_blood()->drugs()) {
  //   std::cout <<"Person ID: "<< person->get_uid() <<" Drug ID: "<< kv_drug.first << " Last update value: " <<kv_drug.second->last_update_value() <<std::endl;
  // }
}