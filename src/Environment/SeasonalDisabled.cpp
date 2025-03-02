#include "SeasonalDisabled.h"

double SeasonalDisabled::get_seasonal_factor(const date::sys_days &today,
                                           const int &location) {
  return 1.0;
} 