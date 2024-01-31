
/*
 * SQLiteDistrictReporter.h
 *
 * Override the base DbReporter and log genotype information at the district
 * level.
 */
#ifndef SQLITEDISTRICTREPORTER_H
#define SQLITEDISTRICTREPORTER_H

#include "Reporters/SQLiteDbReporter.h"

class SQLiteDistrictReporter : public SQLiteDbReporter {
private:
  std::vector<int> district_lookup;

private:
  // Get the genotype level (specific to district level in this case)
  char get_genotype_level() override { return 'D'; }
  void monthly_genome_data(int id) override;
  void monthly_infected_individuals(int id) override;
  void monthly_site_data(int id) override;

public:
  SQLiteDistrictReporter() = default;
  virtual ~SQLiteDistrictReporter() override = default;

  // Initialize the reporter with job number and path
  void initialize(int job_number, std::string path) override;
};

#endif
