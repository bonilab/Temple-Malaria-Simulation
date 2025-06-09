//
// Created by Nguyen Tran on 1/25/2018.
//

#ifndef SPATIAL_LOCATION_H
#define SPATIAL_LOCATION_H

#include <memory>
#include <ostream>
#include <vector>

#include "Coordinate.h"
#include "Core/PropertyMacro.h"

namespace Spatial {

/*!
 *  Location is the smallest entity in the spatial structure.
 *  Location could be district, province, or zone depends on the availability of
 * the data
 */

class Location {
  //    DISALLOW_COPY_AND_ASSIGN_(Location)

public:
  int id;
  int population_size;
  float beta;
  float p_treatment_less_than_5;
  float p_treatment_more_than_5;
  std::unique_ptr<Coordinate> coordinate;
  std::vector<double> age_distribution;

  // added for ASTC May 2025
  float p_treatment_base;

public:
  Location(int id, float latitude, float longitude, int population_size);

  virtual ~Location();

  Location(const Location &org);

  Location &operator=(const Location &other);

  friend std::ostream &operator<<(std::ostream &os, const Location &location);
};
}  // namespace Spatial

#endif  // SPATIAL_LOCATION_H
