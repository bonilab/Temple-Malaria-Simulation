/*
 * File:   ReceiveMDADrugEvent.h
 * Author: Merlin
 *
 * Created on August 25, 2013, 10:20 PM
 */

#ifndef RECEIVEMDADRUGEVENT_H
#define RECEIVEMDADRUGEVENT_H

#include <string>

#include "Core/PropertyMacro.h"
#include "Event.h"

class Scheduler;

class Person;

class Therapy;

class ReceiveMDATherapyEvent : public Event {
  DELETE_COPY_AND_MOVE(ReceiveMDATherapyEvent)

  POINTER_PROPERTY(Therapy, received_therapy)

public:
  ReceiveMDATherapyEvent();

  //    ReceiveMDADrugEvent(const ReceiveMDADrugEvent& orig);
  virtual ~ReceiveMDATherapyEvent();

  static void schedule_event(Scheduler* scheduler, Person* p, Therapy* therapy,
                             const int &time);

  std::string name() override { return "ReceiveMDADrugEvent"; }

private:
  void execute() override;
};

#endif /* RECEIVEMDADRUGEVENT_H */
