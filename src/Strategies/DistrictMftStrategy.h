/*
 * DistrictMftStrategy.h
 *
 * Define the district multiple first-line therapy (MFT) strategy class.
 */
#ifndef POMS_DISTRICTMFTSTRATEGY_H
#define POMS_DISTRICTMFTSTRATEGY_H

#include <memory>  // Add this for unique_ptr
#include "Core/PropertyMacro.h"
#include "IStrategy.h"

class DistrictMftStrategy : public IStrategy {
  DELETE_COPY_AND_MOVE(DistrictMftStrategy)

public:
  // The basic structure of an MFT
  struct MftStrategy {
    std::vector<int> therapies;
    std::vector<float> percentages;
  };

private:
  std::map<int, std::unique_ptr<MftStrategy>> district_strategies;

public:
  DistrictMftStrategy();
  ~DistrictMftStrategy() override = default;  // unique_ptr handles cleanup automatically

  // Override the method for IStrategy and throw an error if called.
  void add_therapy(Therapy* therapy) override;

  /**
   * @brief Sets the Multiple First-line Therapy (MFT) strategy for a specific district
   *
   * This function assigns a treatment strategy to a district. Each district can only 
   * have one strategy assigned, and the assignment cannot be changed once set.
   *
   * @param district The district ID (matches IDs from district raster file)
   * @param strategy The MFT strategy to be assigned
   * 
   * @throws std::out_of_range If district ID is outside valid range
   * @throws std::runtime_error If district already has a strategy assigned
   * 
   * @note District IDs should match those in the district raster file (can be 0-based or 1-based)
   */
  void set_district_strategy(int district, std::unique_ptr<MftStrategy> strategy);

  // Get the therapy that should be given to the individual.
  Therapy* get_therapy(Person* person) override;

  // Return a string with the name of the district MFT strategy from the
  // configuration
  [[nodiscard]] std::string to_string() const override { return this->name(); }

  void adjust_started_time_point(const int &current_time) override {}
  void monthly_update() override {}
  void update_end_of_time_step() override {}
};

#endif
