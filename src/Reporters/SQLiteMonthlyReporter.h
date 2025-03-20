/*
 * SQLiteMonthlyReporter.h
 *
 * Override the base DbReporter and log genotype information at the monthly
 * level.
 */
#ifndef SQLITEMONTHLYREPORTER_H
#define SQLITEMONTHLYREPORTER_H

#include "Reporters/SQLiteDbReporter.h"

class Person;

class SQLiteMonthlyReporter : public SQLiteDbReporter {
  DELETE_COPY_AND_MOVE(SQLiteMonthlyReporter);

  void monthly_report_genome_data(int monthId) override;
  void monthly_report_site_data(int monthId) override;

protected:
  struct MonthlySiteData {
    std::vector<double> eir, pfpr_under5, pfpr2to10, pfpr_all;
    std::vector<int> population, clinical_episodes, treatments,
        treatment_failures, nontreatment, treatments_under5, treatments_over5,
        infections_by_unit;
    std::vector<std::vector<int>> clinical_episodes_by_age_class;
  };

  struct MonthlyGenomeData {
    std::vector<std::vector<int>> occurrences;
    std::vector<std::vector<int>> clinical_occurrences;
    std::vector<std::vector<int>> occurrences_0_5;
    std::vector<std::vector<int>> occurrences_2_10;
    std::vector<std::vector<double>> weighted_occurrences;
  };

  std::vector<MonthlySiteData> monthly_site_data_by_level;
  std::vector<MonthlyGenomeData> monthly_genome_data_by_level;

  std::vector<std::string> insert_values;

private:
  void reset_site_data_structures(int level_id, int vectorSize, size_t numAgeClasses);
  void reset_genome_data_structures(int level_id, int vectorSize, size_t numGenotypes);
  void count_infections_for_location(int location, int level_id);
  void collect_site_data_for_location(int location, int level_id);
  void calculate_and_build_up_site_data_insert_values(int monthId, int level_id);
  void collect_genome_data_for_location(size_t location, int level_id);
  void collect_genome_data_for_a_person(Person* person, int unit_id, int level_id);
  void build_up_genome_data_insert_values(int monthId, int level_id);

public:
  SQLiteMonthlyReporter() = default;
  ~SQLiteMonthlyReporter() override = default;

  // Initialize the reporter with job number and path
  void initialize(int jobNumber, const std::string &path) override;
};

#endif
