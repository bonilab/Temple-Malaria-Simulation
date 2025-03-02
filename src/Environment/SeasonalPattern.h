#ifndef SEASONALPATTERN_H
#define SEASONALPATTERN_H

#include <yaml-cpp/yaml.h>
#include <vector>
#include "Core/TypeDef.h"
#include "SeasonalInfo.h"

class SeasonalPattern : public ISeasonalInfo {
protected:
  // Vector of vectors: [district][day/month]
  std::vector<DoubleVector> district_adjustments;
  int period;  // Either 365 for daily or 12 for monthly
  bool is_monthly;  // Flag to indicate if we're using monthly data

  // Make this virtual so we can override it in tests
  virtual int get_district_for_location(int location) const;
  void read(const std::string &filename);
  void initialize(const YAML::Node &node);

public:
  static SeasonalPattern* build(const YAML::Node &node);
  double get_seasonal_factor(const date::sys_days &today,
                           const int &location) override;

  // Getter methods for testing
  bool get_is_monthly() const { return is_monthly; }
  int get_period() const { return period; }
};

#endif 