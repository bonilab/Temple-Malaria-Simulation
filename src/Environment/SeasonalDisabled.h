#ifndef SEASONALDISABLED_H
#define SEASONALDISABLED_H

#include "SeasonalInfo.h"

class SeasonalDisabled : public ISeasonalInfo {
public:
  double get_seasonal_factor(const date::sys_days &today,
                           const int &location) override;
};

#endif 