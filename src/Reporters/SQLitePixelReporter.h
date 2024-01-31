/*
 * SQLiteDbReporter.h
 *
 * Define the SQLiteDbReporter class which is used to insert relevant
 * information from the model into the database during model execution.
 */
#ifndef SQLITEPIXELREPORTER_H
#define SQLITEPIXELREPORTER_H

#include "Reporters/SQLiteDbReporter.h"

class SQLitePixelReporter : public SQLiteDbReporter {

protected:
  // Return the character code that indicates the level of genotype records (c:
  // cell, d: district)
  char get_genotype_level() override { return 'C'; }
  void monthly_genome_data(int idy) override;
  void monthly_infected_individuals(int id) override;
  void monthly_site_data(int id) override;

public:
  SQLitePixelReporter() = default;
  virtual ~SQLitePixelReporter() override = default;

  // Overrides
  void initialize(int job_number, std::string path) override;
};

#endif
