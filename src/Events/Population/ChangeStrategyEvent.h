#ifndef CHANGESTRATEGYEVENT_H
#define CHANGESTRATEGYEVENT_H

#include <string>

#include "Core/PropertyMacro.h"
#include "Events/Event.h"

class ChangeStrategyEvent : public Event {
  DELETE_COPY_AND_MOVE(ChangeStrategyEvent)

public:
  int strategy_id{-1};

  ChangeStrategyEvent(const int &at_time, const int &strategy_id);

  ~ChangeStrategyEvent() override = default;

  std::string name() override { return "ChangeStrategyEvent"; }

private:
  void execute() override;
};

#endif  // CHANGESTRATEGYEVENT_H
