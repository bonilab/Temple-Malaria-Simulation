/*
 * ProgressToClinicalEvent.h
 *
 * Define the event that handles the individual manifesting a clinical case of
 * malaria.
 */
#ifndef PROGRESSTOCLINICALEVENT_H
#define PROGRESSTOCLINICALEVENT_H

#include <string>

#include "Core/ObjectPool.h"
#include "Core/PropertyMacro.h"
#include "Event.h"

class Person;

class Scheduler;

class ClonalParasitePopulation;

class Therapy;

class ProgressToClinicalEvent : public Event {
  OBJECTPOOL(ProgressToClinicalEvent)

  DELETE_COPY_AND_MOVE(ProgressToClinicalEvent)

  POINTER_PROPERTY(ClonalParasitePopulation, clinical_caused_parasite)

public:
  ProgressToClinicalEvent();

  ~ProgressToClinicalEvent() override;

  static void schedule_event(Scheduler* scheduler, Person* person,
                             ClonalParasitePopulation* clinical_caused_parasite,
                             const int &time);

  static void receive_no_treatment_routine(Person* person);

  std::string name() override { return "ProgressToClinicalEvent"; }

private:
  void transition_to_clinical_state(Person* person);
  static std::tuple<Therapy*, bool> determine_therapy(
      Person* person, bool is_recurrence = false);
  void apply_therapy(Person* person, Therapy* therapy,
                     bool is_public_sector = true);

  void execute() override;
};

#endif
