#ifndef CHANGE_RECRUDESCENCE_TREATMENT_EVENT_H
#define CHANGE_RECRUDESCENCE_TREATMENT_EVENT_H

#include <string>

#include "Core/PropertyMacro.h"
#include "Events/Event.h"
class ChangeRecrudescenceTreatmentEvent : public Event {
  DELETE_COPY_AND_MOVE(ChangeRecrudescenceTreatmentEvent)

public:
  int therapy_id{-1};

  ChangeRecrudescenceTreatmentEvent(const int &at_time, const int &therapy_id);

  ~ChangeRecrudescenceTreatmentEvent() override = default;

  std::string name() override { return "ChangeRecrudescenceTreatmentEvent"; }

private:
  void execute() override;
};

#endif  // CHANGE_RECRUDESCENCE_TREATMENT_EVENT_H
