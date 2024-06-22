
/*
 * SQLiteDistrictReporter.h
 *
 * Override the base DbReporter and log genotype information at the district
 * level.
 */
#ifndef SQLITEDISTRICTREPORTER_H
#define SQLITEDISTRICTREPORTER_H

#include "Reporters/SQLiteDbReporter.h"

class Person;

class SQLiteDistrictReporter : public SQLiteDbReporter {
  DELETE_COPY_AND_MOVE(SQLiteDistrictReporter);

  // Get the genotype level (specific to district level in this case)
  char get_genotype_level() override { return 'D'; }
  void monthly_report_genome_data(int monthId) override;
  void monthly_report_site_data(int monthId) override;

protected:
  struct MonthlySiteData {
    std::vector<double> eir, pfpr_under5, pfpr2to10, pfpr_all;
    std::vector<int> population, clinical_episodes, treatments,
        recrudescence_treatments, treatment_failures, nontreatment,
        treatments_under5, treatments_over5, infections_by_district;
    std::vector<std::vector<int>> clinical_episodes_by_age_class;
  };

  struct MonthlyGenomeData {
    std::vector<std::vector<int>> occurrences;
    std::vector<std::vector<int>> clinical_occurrences;
    std::vector<std::vector<int>> occurrences_0_5;
    std::vector<std::vector<int>> occurrences_2_10;
    std::vector<std::vector<double>> weighted_occurrences;
  };

  MonthlySiteData monthly_site_data;
  MonthlyGenomeData monthly_genome_data;

  std::vector<std::string> insert_values;

private:
  void reset_site_data_structures(int numDistricts, size_t numAgeClasses);
  void reset_genome_data_structures(int numDistricts, size_t numGenotypes);
  void count_infections_for_location(int location);
  void collect_site_data_for_location(int location);
  void calculate_and_build_up_site_data_insert_values(int monthId);
  void collect_genome_data_for_location(size_t location);
  void collect_genome_data_for_a_person(Person* person, int site);
  void build_up_genome_data_insert_values(int monthId);

public:
  SQLiteDistrictReporter() = default;
  ~SQLiteDistrictReporter() override = default;

  // Initialize the reporter with job number and path
  void initialize(int jobNumber, const std::string &path) override;
};

#endif
