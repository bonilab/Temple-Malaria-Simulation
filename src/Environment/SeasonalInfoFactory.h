#ifndef SEASONALINFOFACTORY_H
#define SEASONALINFOFACTORY_H

#include <yaml-cpp/yaml.h>
#include "SeasonalInfo.h"

class Config;

class SeasonalInfoFactory {
public:
  static ISeasonalInfo* build(const YAML::Node &node, Config* config);
};

#endif 