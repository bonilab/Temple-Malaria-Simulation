
/*
 * SQLiteDbReporter.h
 *
 * Override the base DbReporter and log genotype information at the district
 * level.
 */
#ifndef SQLITEDBREPORTER_H
#define SQLITEDBREPORTER_H

#include "Helpers/SQLiteDatabase.h"
#include "Reporter.h"

class SQLiteDbReporter : public Reporter {
protected:
  std::unique_ptr<SQLiteDatabase> db;

protected:
  const std::string INSERT_COMMON = R""""(
  INSERT INTO MonthlyData (DaysElapsed, ModelTime, SeasonalFactor)
  VALUES (?, ?, ?)
  RETURNING id;
  )"""";

  const std::string INSERT_SITE_PREFIX = R""""(
    INSERT INTO MonthlySiteData (MonthlyDataId, LocationId, Population, ClinicalEpisodes, Treatments, EIR, PfPrUnder5, PfPr2to10, PfPrAll, TreatmentFailures, NonTreatment, Under5Treatment, Over5Treatment) 
    VALUES 
  )"""";

  const std::string INSERT_GENOTYPE_PREFIX = R"""(
    INSERT INTO MonthlyGenomeData 
    (MonthlyDataId, LocationId, GenomeId, Occurrences, 
    ClinicalOccurrences, Occurrences0to5, Occurrences2to10, 
    WeightedOccurrences) 
    VALUES 
  )""";

  const std::string UPDATE_INFECTED_INDIVIDUALS = R"""(
    UPDATE MonthlySiteData SET InfectedIndividuals = {} 
    WHERE MonthlyDataId = {} AND LocationId = {};
  )""";

  void populate_genotype_table();
  void populate_db_schema();

  // Return the character code that indicates the level of genotype records (c:
  // cell, d: district)
  virtual char get_genotype_level() = 0;
  virtual void monthly_genome_data(int id) = 0;
  virtual void monthly_infected_individuals(int id) = 0;
  virtual void monthly_site_data(int id) = 0;

public:
  // Constructor and destructor
  SQLiteDbReporter() = default;
  virtual ~SQLiteDbReporter() = default;

  // Initialize the reporter with job number and path
  void initialize(int job_number, std::string path) override;

  // Basic declarations for before run and begin time step
  void before_run() override {}
  void begin_time_step() override {}

  // Overridden functions for monthly reporting and post-run cleanup
  void monthly_report() override;
  // With the new Database class, the database connection will automatically be
  // closed when the Database object is destroyed. Therefore, no explicit action
  // is required here. If there are other cleanup actions to perform, they can
  // be added here.
  void after_run() override {}
};

#endif
