#ifndef SMCEVENT_H
#define SMCEVENT_H

#include <vector>

#include "Core/PropertyMacro.h"
#include "Events/Event.h"

class SMCEvent : public Event {
  DELETE_COPY_AND_MOVE(SMCEvent)

public:
  // Fraction of population targeted for each district listed
  std::vector<double> fraction_population_targeted;

  // Maximum number of days within which all treatments are given
  int days_to_complete_all_treatments{14};

  // List of targeted districts (location indices)
  std::vector<int> districts;

  // Age range for targeted population [min_age, max_age]
  std::vector<int> age_range{0, 120};

  // SMC year
  int smc_year;

  // SMC month
  int smc_month;

  explicit SMCEvent(const int &execute_at);

  virtual ~SMCEvent() = default;

  std::string name() override { return "SMCEvent"; }

private:
  void execute() override;
};

#endif  // SMCEVENT_H
