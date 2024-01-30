/*
 * File:   Dispatcher.h
 * Author: nguyentran
 *
 * Created on May 3, 2013, 3:46 PM
 */

#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "Core/PropertyMacro.h"
#include "Core/TypeDef.h"

class Event;

class Dispatcher {
  DISALLOW_COPY_AND_ASSIGN(Dispatcher)

  POINTER_PROPERTY(EventPtrVector, events)

private:
  const int INTITAL_ALLOCATION = 32;

public:
  Dispatcher();

  virtual ~Dispatcher();

  virtual void init();

  virtual void add(Event* event);

  virtual void remove(Event* event);

  virtual void update();

  virtual void clear_events();
};

#endif
