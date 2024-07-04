
#include "ChangeRecrudescenceTreatmentEvent.h"

#include "Core/Config/Config.h"
#include "Model.h"

ChangeRecrudescenceTreatmentEvent::ChangeRecrudescenceTreatmentEvent(
    const int &at_time, const int &therapy_id)
    : therapy_id(therapy_id) {
  time = at_time;
}

void ChangeRecrudescenceTreatmentEvent::execute() {
  Model::CONFIG->recurrence_therapy_id() = therapy_id;
  LOG(INFO) << date::year_month_day{scheduler->calendar_date} << " : switch to "
            << this->name();
}

