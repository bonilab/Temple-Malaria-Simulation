/*
 * File:   ReceiveSMCTherapyEvent.h
 * Author: Sarit
 *
 * Created on July 7, 2025
 */

#ifndef RECEIVESMCTHERAPYEVENT_H
#define RECEIVESMCTHERAPYEVENT_H

#include <string>

#include "Core/PropertyMacro.h"
#include "Event.h"

class Scheduler;
class Person;
class Therapy;

class ReceiveSMCTherapyEvent : public Event {
  DELETE_COPY_AND_MOVE(ReceiveSMCTherapyEvent)

  POINTER_PROPERTY(Therapy, received_therapy)

public:
  ReceiveSMCTherapyEvent();
  virtual ~ReceiveSMCTherapyEvent();

  static void schedule_event(Scheduler* scheduler, Person* p, Therapy* therapy,
                             const int &time);

  std::string name() override { return "ReceiveSMCTherapyEvent"; }

private:
  void execute() override;
};

#endif /* RECEIVESMCTHERAPYEVENT_H */