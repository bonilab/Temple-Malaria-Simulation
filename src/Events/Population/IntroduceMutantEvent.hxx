/**
 * IntroduceMutantEvent.hxx
 *
 * Introduce mutant genotypes into the simulation by randomly selecting infected
 * individuals and switching the relevant genotype from the wild type to the
 * specified mutant based upon the district id supplied.
 */
#ifndef INTRODUCEMUTANTEVENT_HXX
#define INTRODUCEMUTANTEVENT_HXX

#include <string>

#include "Core/ObjectPool.h"
#include "Core/PropertyMacro.h"
#include "Core/Scheduler.h"
#include "Events/Event.h"
#include "GIS/SpatialData.h"
#include "IntroduceMutantEventBase.h"
#include "Population/Properties/PersonIndexByLocationStateAgeClass.h"
#include "easylogging++.h"

class IntroduceMutantEvent : public IntroduceMutantEventBase {
  DELETE_COPY_AND_MOVE(IntroduceMutantEvent)

private:
  int admin_level_id_;
  int unit_id_;

  void execute() override {
    // Calculate the target fraction of the district infections and perform them
    // as needed
    auto locations =
        SpatialData::get_instance().get_locations_in_unit(admin_level_id_, unit_id_);
    double target_fraction = calculate(locations);
    auto count = (target_fraction > 0) ? mutate(locations, target_fraction) : 0;

    // Log the event's operation
    LOG(INFO) << date::year_month_day{scheduler->calendar_date}
              << " : Introduce mutant event, target fraction: "
              << target_fraction << ", mutations: " << count;
  }

public:
  inline static const std::string EventName = "introduce_mutant_event";

  explicit IntroduceMutantEvent(const int &time, const int &unit_id,
                                const double &fraction, const int &locus,
                                const int &mutant_allele, const int &admin_level_id)
      : IntroduceMutantEventBase(fraction, locus, mutant_allele),
        unit_id_(unit_id),
        admin_level_id_(admin_level_id) {
    this->time = time;
  }

  ~IntroduceMutantEvent() override = default;

  // Return the name of this event
  std::string name() override { return EventName; }
};

#endif
