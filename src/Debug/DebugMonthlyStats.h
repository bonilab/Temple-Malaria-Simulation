//
// Created by Kien Tran on 4/25/26.
//

#ifndef TMS_DEBUGMONTHLYSTATS_H
#define TMS_DEBUGMONTHLYSTATS_H

#pragma once

#include <fstream>
#include <string>
#include <unordered_map>

struct DebugMonthlyStats {
  int current_month = -1;

  double pfpr = -1.0;
  double incidence = -1.0;
  double total_foi = 0.0;
  long long number_of_bites = 0;
  long long successful_infections = 0;

  long long clinical_scheduled_age0 = 0;
  long long clinical_count_age0 = 0;

  double sum_p_clinical_age0 = 0.0;
  double sum_immunity_age0 = 0.0;
  long long n_p_clinical_age0 = 0;

  long long successful_infections_age0 = 0;
  long long bite_attempts_age0 = 0;
  long long infectious_bites_age0 = 0;

  long long clinical_decision_age0 = 0;
  long long clinical_will_schedule_age0 = 0;

  std::unordered_map<long long, int> clinical_count_age0_by_person;
  long long clinical_count_age0_duplicate_person = 0;

  bool header_written = false;
  std::string version = "unknown";
  std::string output_path = "debug_monthly_stats.csv";

  void set_version(const std::string& v) {
    version = v;
    output_path = "debug_monthly_stats_" + v + ".csv";

    std::ofstream out(output_path, std::ios::trunc);
    out.close();

    header_written = false;
  }

  void reset_month(int month) {
    current_month = month;
    total_foi = 0.0;
    number_of_bites = 0;
    successful_infections = 0;
    clinical_scheduled_age0 = 0;
    clinical_count_age0 = 0;
    sum_p_clinical_age0 = 0.0;
    sum_immunity_age0 = 0.0;
    n_p_clinical_age0 = 0;
    total_poisson_mean = 0.0;
    new_beta = 0.0;
    successful_infections_age0 = 0;
    bite_attempts_age0 = 0;
    infectious_bites_age0 = 0;
    clinical_decision_age0 = 0;
    clinical_will_schedule_age0 = 0;
    clinical_count_age0_duplicate_person = 0;
    clinical_count_age0_by_person.clear();

  }

  void record_foi(double foi) {
    total_foi += foi;
  }

  void record_bites(int bites) {
    number_of_bites += bites;
  }

  void record_successful_infection() {
    successful_infections++;
  }

  void record_clinical_scheduled_age0(double p_clinical, double immunity) {
    clinical_scheduled_age0++;
    sum_p_clinical_age0 += p_clinical;
    sum_immunity_age0 += immunity;
    n_p_clinical_age0++;
  }

  void record_p_clinical_age0(double p_clinical, double immunity) {
    sum_p_clinical_age0 += p_clinical;
    sum_immunity_age0 += immunity;
    n_p_clinical_age0++;
  }

  void record_clinical_count_age0() {
    clinical_count_age0++;
  }

  double total_poisson_mean = 0.0;

  void record_poisson_mean(double x) {
    total_poisson_mean += x;
  }

  double new_beta = 0.0;

  void record_new_beta(double x) {
    new_beta = x;
  }

  void record_bite_attempt_age0() {
    bite_attempts_age0++;
  }

  void record_infectious_bite_age0() {
    infectious_bites_age0++;
  }

  void record_successful_infection_age0() {
    successful_infections_age0++;
  }

  void record_clinical_decision_age0(double p_clinical, double immunity) {
    clinical_decision_age0++;
    sum_p_clinical_age0 += p_clinical;
    sum_immunity_age0 += immunity;
    n_p_clinical_age0++;
  }

  void record_clinical_will_schedule_age0() {
    clinical_will_schedule_age0++;
  }

  void record_clinical_count_age0_person(long long person_id) {
    clinical_count_age0++;

    int &count = clinical_count_age0_by_person[person_id];
    count++;

    if (count > 1) {
      clinical_count_age0_duplicate_person++;
    }
  }

  void write_month(int month, double pfpr, double incidence) {
    std::ofstream out(output_path, std::ios::app);

    if (!header_written) {
      out << "version,month,pfpr,incidence,total_foi,new_beta,total_poisson_mean,number_of_bites,"
    << "successful_infections,successful_infections_age0,bite_attempts_age0,infectious_bites_age0,"
    << "clinical_scheduled_age0,clinical_count_age0,"
    << "clinical_decision_age0,clinical_will_schedule_age0,"
    << "clinical_count_age0_duplicate_person,"
    << "mean_p_clinical_age0,mean_immunity_age0\n";
      header_written = true;
    }

    const double mean_p =
        n_p_clinical_age0 > 0 ? sum_p_clinical_age0 / n_p_clinical_age0 : 0.0;

    const double mean_immune =
        n_p_clinical_age0 > 0 ? sum_immunity_age0 / n_p_clinical_age0 : 0.0;

    out << version << ","
    << month << ","
    << pfpr << ","
    << incidence << ","
    << total_foi << ","
    << new_beta << ","
    << total_poisson_mean << ","
    << number_of_bites << ","
    << successful_infections << ","
    << successful_infections_age0 << ","
    << bite_attempts_age0 << ","
    << infectious_bites_age0 << ","
    << clinical_scheduled_age0 << ","
    << clinical_count_age0 << ","
    << clinical_decision_age0 << ","
    << clinical_will_schedule_age0 << ","
    << clinical_count_age0_duplicate_person << ","
    << mean_p << ","
    << mean_immune << "\n";

    reset_month(month + 1);
  }
};

extern DebugMonthlyStats DEBUG_MONTHLY_STATS;

#endif  // TMS_DEBUGMONTHLYSTATS_H
