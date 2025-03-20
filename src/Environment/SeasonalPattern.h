#ifndef SEASONALPATTERN_H
#define SEASONALPATTERN_H

#include <yaml-cpp/yaml.h>
#include <vector>
#include "Core/TypeDef.h"
#include "SeasonalInfo.h"

class SpatialData;

class SeasonalPattern : public ISeasonalInfo {
protected:
  // Vector of vectors: [admin_unit][day/month]
  std::vector<DoubleVector> admin_unit_adjustments;
  int period{12};  // Either 365 for daily or 12 for monthly
  bool is_monthly{true};  // Flag to indicate if we're using monthly data

  int min_admin_unit_id{-1};
  int max_admin_unit_id{-1};
  std::string admin_level{""};
  int admin_level_id{-1};

  // Make this virtual so we can override it in tests
  virtual int get_admin_unit_for_location(int location) const;
  void read(const std::string &filename);
  void initialize(const YAML::Node &node, SpatialData* spatial_data);

public:
  static SeasonalPattern* build(const YAML::Node &node, SpatialData* spatial_data);
  double get_seasonal_factor(const date::sys_days &today,
                           const int &location) override;

  // Getter methods for testing
  bool get_is_monthly() const { return is_monthly; }
  int get_period() const { return period; }
};

#endif 