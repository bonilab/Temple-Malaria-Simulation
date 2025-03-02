#ifndef SEASONALEQUATION_H
#define SEASONALEQUATION_H

#include <yaml-cpp/yaml.h>
#include "Core/TypeDef.h"
#include "SeasonalInfo.h"

class Config;

class SeasonalEquation : public ISeasonalInfo {
private:
  DoubleVector base;
  DoubleVector A;
  DoubleVector B;
  DoubleVector phi;

  // The reference values contain the inputs from the YAML so the
  // UpdateEcozoneEvent can change the ecozone (i.e., seasonal information)
  // during model execution
  DoubleVector reference_base;
  DoubleVector reference_A;
  DoubleVector reference_B;
  DoubleVector reference_phi;

  void set_from_raster(const YAML::Node &node);
  void set_seasonal_period(const YAML::Node &node, unsigned long index);

public:
  static SeasonalEquation* build(const YAML::Node &node, Config* config);
  double get_seasonal_factor(const date::sys_days &today,
                           const int &location) override;
  void update_seasonality(int from, int to);
};

#endif 