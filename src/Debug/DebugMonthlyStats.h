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
  long long genotype_draw_failed_age0 = 0;

  // ------------------------------------------------------------
  // Age-0 bite / infection-pressure debug counters
  // ------------------------------------------------------------
  double sum_relative_biting_rate_age0 = 0.0;
  long long n_relative_biting_rate_age0 = 0;

  double sum_infection_probability_age0 = 0.0;
  long long n_infection_probability_age0 = 0;

  long long clinical_decision_age0 = 0;
  long long clinical_will_schedule_age0 = 0;

  long long clinical_count_age0_new_infection = 0;

  std::unordered_map<long long, int> clinical_count_age0_by_person;
  long long clinical_count_age0_duplicate_person = 0;

  long long clinical_count_age0_from_normal_progression = 0;
  long long clinical_count_age0_from_recurrence = 0;
  long long clinical_count_age0_from_unknown = 0;

  long long clinical_count_age0_duplicate_normal = 0;
  long long clinical_count_age0_duplicate_recurrence = 0;
  long long clinical_count_age0_duplicate_unknown = 0;
  long long clinical_count_age0_after_min_gap = 0;

  // ------------------------------------------------------------
  // Eligible / selected biting debug
  // ------------------------------------------------------------
  long long bite_eligible_persons_total = 0;
  long long bite_eligible_persons_age0 = 0;

  double bite_eligible_weight_total = 0.0;
  double bite_eligible_weight_age0 = 0.0;

  double bite_eligible_weight_sq_total = 0.0;
  double bite_eligible_weight_sq_age0 = 0.0;

  long long bite_selected_persons_total = 0;
  long long bite_selected_persons_age0 = 0;
  double bite_selected_weight_total = 0.0;
  double bite_selected_weight_age0 = 0.0;

  // ------------------------------------------------------------
  // FOI construction aggregate debug
  // This replaces heavy per-person debug_foi_terms.csv logging.
  // These are FOI-weighted means, so high-FOI contributors matter more.
  // ------------------------------------------------------------
  long long foi_debug_rows = 0;
  double foi_debug_sum = 0.0;

  double foi_debug_biting_weight_weighted_sum = 0.0;
  double foi_debug_relative_infectivity_weighted_sum = 0.0;
  double foi_debug_log_density_weighted_sum = 0.0;
  double foi_debug_genotype_share_weighted_sum = 0.0;

  bool header_written = false;
  bool event_header_written = false;
  bool bite_terms_header_written = false;

  std::string version = "unknown";
  std::string output_path = "debug_monthly_stats.csv";
  std::string event_output_path = "debug_age0_clinical_events.csv";
  std::string bite_terms_output_path = "debug_bite_terms.csv";

  static std::string csv_escape(const std::string& s) {
    bool need_quotes = false;

    for (char c : s) {
      if (c == ',' || c == '"' || c == '\n' || c == '\r') {
        need_quotes = true;
        break;
      }
    }

    if (!need_quotes) {
      return s;
    }

    std::string out = "\"";

    for (char c : s) {
      if (c == '"') {
        out += "\"\"";
      } else {
        out += c;
      }
    }

    out += "\"";
    return out;
  }

  void set_version(const std::string& v) {
    version = v;
    output_path = "debug_monthly_stats_" + v + ".csv";
    event_output_path = "debug_age0_clinical_events_" + v + ".csv";
    bite_terms_output_path = "debug_bite_terms_" + v + ".csv";

    {
      std::ofstream out(bite_terms_output_path, std::ios::trunc);
      out.close();
    }

    bite_terms_header_written = false;

    {
      std::ofstream out(output_path, std::ios::trunc);
      out.close();
    }

    {
      std::ofstream out(event_output_path, std::ios::trunc);
      out.close();
    }

    header_written = false;
    event_header_written = false;
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
    genotype_draw_failed_age0 = 0;

    sum_relative_biting_rate_age0 = 0.0;
    n_relative_biting_rate_age0 = 0;

    sum_infection_probability_age0 = 0.0;
    n_infection_probability_age0 = 0;

    clinical_decision_age0 = 0;
    clinical_will_schedule_age0 = 0;

    clinical_count_age0_new_infection = 0;
    clinical_count_age0_duplicate_person = 0;
    clinical_count_age0_by_person.clear();

    clinical_count_age0_from_normal_progression = 0;
    clinical_count_age0_from_recurrence = 0;
    clinical_count_age0_from_unknown = 0;

    clinical_count_age0_duplicate_normal = 0;
    clinical_count_age0_duplicate_recurrence = 0;
    clinical_count_age0_duplicate_unknown = 0;
    clinical_count_age0_after_min_gap = 0;

    bite_eligible_persons_total = 0;
    bite_eligible_persons_age0 = 0;

    bite_eligible_weight_total = 0.0;
    bite_eligible_weight_age0 = 0.0;

    bite_eligible_weight_sq_total = 0.0;
    bite_eligible_weight_sq_age0 = 0.0;

    bite_selected_persons_total = 0;
    bite_selected_persons_age0 = 0;
    bite_selected_weight_total = 0.0;
    bite_selected_weight_age0 = 0.0;

    foi_debug_rows = 0;
    foi_debug_sum = 0.0;
    foi_debug_biting_weight_weighted_sum = 0.0;
    foi_debug_relative_infectivity_weighted_sum = 0.0;
    foi_debug_log_density_weighted_sum = 0.0;
    foi_debug_genotype_share_weighted_sum = 0.0;
  }

  void record_bite_selected_person(double biting_weight, int age) {
    bite_selected_persons_total++;
    bite_selected_weight_total += biting_weight;

    if (age == 0) {
      bite_selected_persons_age0++;
      bite_selected_weight_age0 += biting_weight;
    }
  }

  void record_bite_eligible_person(double biting_weight, int age) {
    bite_eligible_persons_total++;
    bite_eligible_weight_total += biting_weight;
    bite_eligible_weight_sq_total += biting_weight * biting_weight;

    if (age == 0) {
      bite_eligible_persons_age0++;
      bite_eligible_weight_age0 += biting_weight;
      bite_eligible_weight_sq_age0 += biting_weight * biting_weight;
    }
  }

  void record_genotype_draw_failed_age0() {
    genotype_draw_failed_age0++;
  }

  void record_clinical_count_age0_after_min_gap() {
    clinical_count_age0_after_min_gap++;
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

  void record_clinical_scheduled_age0() {
    clinical_scheduled_age0++;
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

  // ------------------------------------------------------------
  // Age-0 bite / infection-pressure record functions
  // ------------------------------------------------------------
  void record_relative_biting_rate_age0(double x) {
    sum_relative_biting_rate_age0 += x;
    n_relative_biting_rate_age0++;
  }

  void record_infection_probability_age0(double x) {
    sum_infection_probability_age0 += x;
    n_infection_probability_age0++;
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

  void record_clinical_count_age0_new_infection() {
    clinical_count_age0_new_infection++;
  }

  bool record_clinical_count_age0_person(long long person_id) {
    clinical_count_age0++;

    int& count = clinical_count_age0_by_person[person_id];
    count++;

    if (count > 1) {
      clinical_count_age0_duplicate_person++;
      return true;
    }

    return false;
  }

  void record_clinical_count_age0_normal(long long person_id) {
    const bool duplicate = record_clinical_count_age0_person(person_id);
    clinical_count_age0_from_normal_progression++;

    if (duplicate) {
      clinical_count_age0_duplicate_normal++;
    }
  }

  void record_clinical_count_age0_recurrence(long long person_id) {
    const bool duplicate = record_clinical_count_age0_person(person_id);
    clinical_count_age0_from_recurrence++;

    if (duplicate) {
      clinical_count_age0_duplicate_recurrence++;
    }
  }

  void record_clinical_count_age0_unknown(long long person_id) {
    const bool duplicate = record_clinical_count_age0_person(person_id);
    clinical_count_age0_from_unknown++;

    if (duplicate) {
      clinical_count_age0_duplicate_unknown++;
    }
  }

  // ------------------------------------------------------------
  // Light FOI construction debug.
  // Use this instead of writing one row per person FOI term.
  //
  // v6 call example in Population::update_current_foi():
  //
  // DEBUG_MONTHLY_STATS.record_foi_term_aggregate(
  //     person_relative_biting_rate,
  //     log_10_total_infectious_density,
  //     rel_inf,
  //     1.0,
  //     individual_foi
  // );
  // ------------------------------------------------------------
  void record_foi_term_aggregate(
      double biting_weight,
      double log10_total_density,
      double relative_infectivity,
      double genotype_density_share,
      double foi_contribution) {
    if (foi_contribution <= 0.0) {
      return;
    }

    foi_debug_rows++;
    foi_debug_sum += foi_contribution;

    foi_debug_biting_weight_weighted_sum += biting_weight * foi_contribution;
    foi_debug_relative_infectivity_weighted_sum +=
        relative_infectivity * foi_contribution;
    foi_debug_log_density_weighted_sum +=
        log10_total_density * foi_contribution;
    foi_debug_genotype_share_weighted_sum +=
        genotype_density_share * foi_contribution;
  }

  void record_bite_term(
      int day,
      int month,
      int location,
      int parasite_type_id,
      double beta,
      double seasonal_factor,
      double new_beta,
      double foi,
      double poisson_mean,
      int number_of_bites,
      const std::string& note) {
    std::ofstream out(bite_terms_output_path, std::ios::app);

    if (!bite_terms_header_written) {
      out << "version,day,month,location,parasite_type_id,"
          << "beta,seasonal_factor,new_beta,foi,poisson_mean,number_of_bites,note\n";
      bite_terms_header_written = true;
    }

    out << csv_escape(version) << ","
        << day << ","
        << month << ","
        << location << ","
        << parasite_type_id << ","
        << beta << ","
        << seasonal_factor << ","
        << new_beta << ","
        << foi << ","
        << poisson_mean << ","
        << number_of_bites << ","
        << csv_escape(note) << "\n";
  }

  void record_age0_clinical_event(
      int month,
      int day,
      long long person_id,
      int age,
      int location,
      const std::string& event_type,
      const std::string& source,
      int host_state,
      int number_of_parasites,
      int last_clinical_count_day,
      double p_clinical,
      double immunity,
      double clinical_probability,
      const std::string& note) {
    std::ofstream out(event_output_path, std::ios::app);

    if (!event_header_written) {
      out << "version,month,day,person_id,age,location,event_type,source,"
          << "host_state,number_of_parasites,last_clinical_count_day,"
          << "days_since_last_count,p_clinical,immunity,clinical_probability,note\n";
      event_header_written = true;
    }

    const int days_since_last_count =
        last_clinical_count_day >= 0 ? day - last_clinical_count_day : -1;

    out << csv_escape(version) << ","
        << month << ","
        << day << ","
        << person_id << ","
        << age << ","
        << location << ","
        << csv_escape(event_type) << ","
        << csv_escape(source) << ","
        << host_state << ","
        << number_of_parasites << ","
        << last_clinical_count_day << ","
        << days_since_last_count << ","
        << p_clinical << ","
        << immunity << ","
        << clinical_probability << ","
        << csv_escape(note) << "\n";
  }

  void write_month(int month, double pfpr, double incidence) {
    std::ofstream out(output_path, std::ios::app);

    if (!header_written) {
      out << "version,month,pfpr,incidence,total_foi,new_beta,total_poisson_mean,number_of_bites,"
          << "successful_infections,successful_infections_age0,bite_attempts_age0,infectious_bites_age0,"
          << "genotype_draw_failed_age0,"
          << "mean_relative_biting_rate_age0,n_relative_biting_rate_age0,"
          << "mean_infection_probability_age0,n_infection_probability_age0,"
          << "clinical_scheduled_age0,clinical_count_age0,"
          << "clinical_decision_age0,clinical_will_schedule_age0,"
          << "clinical_count_age0_new_infection,clinical_count_age0_duplicate_person,"
          << "clinical_count_age0_from_normal_progression,"
          << "clinical_count_age0_from_recurrence,"
          << "clinical_count_age0_from_unknown,"
          << "clinical_count_age0_duplicate_normal,"
          << "clinical_count_age0_duplicate_recurrence,"
          << "clinical_count_age0_duplicate_unknown,"
          << "clinical_count_age0_after_min_gap,"
          << "bite_eligible_persons_total,"
          << "bite_eligible_persons_age0,"
          << "bite_eligible_age0_fraction,"
          << "bite_eligible_weight_total,"
          << "bite_eligible_weight_age0,"
          << "bite_eligible_age0_weight_fraction,"
          << "bite_eligible_mean_weight_total,"
          << "bite_eligible_mean_weight_age0,"
          << "bite_selected_persons_total,"
          << "bite_selected_persons_age0,"
          << "bite_selected_age0_fraction,"
          << "bite_selected_mean_weight_total,"
          << "bite_selected_mean_weight_age0,"
          << "foi_debug_rows,"
          << "foi_debug_sum,"
          << "foi_debug_weighted_biting_weight,"
          << "foi_debug_weighted_relative_infectivity,"
          << "foi_debug_weighted_log_density,"
          << "foi_debug_weighted_genotype_share,"
          << "mean_p_clinical_age0,mean_immunity_age0\n";
      header_written = true;
    }

    const double bite_selected_age0_fraction =
        bite_selected_persons_total > 0
            ? static_cast<double>(bite_selected_persons_age0) / bite_selected_persons_total
            : 0.0;

    const double bite_selected_mean_weight_total =
        bite_selected_persons_total > 0
            ? bite_selected_weight_total / bite_selected_persons_total
            : 0.0;

    const double bite_selected_mean_weight_age0 =
        bite_selected_persons_age0 > 0
            ? bite_selected_weight_age0 / bite_selected_persons_age0
            : 0.0;

    const double bite_eligible_age0_fraction =
        bite_eligible_persons_total > 0
            ? static_cast<double>(bite_eligible_persons_age0) / bite_eligible_persons_total
            : 0.0;

    const double bite_eligible_age0_weight_fraction =
        bite_eligible_weight_total > 0.0
            ? bite_eligible_weight_age0 / bite_eligible_weight_total
            : 0.0;

    const double bite_eligible_mean_weight_total =
        bite_eligible_persons_total > 0
            ? bite_eligible_weight_total / bite_eligible_persons_total
            : 0.0;

    const double bite_eligible_mean_weight_age0 =
        bite_eligible_persons_age0 > 0
            ? bite_eligible_weight_age0 / bite_eligible_persons_age0
            : 0.0;

    const double mean_p =
        n_p_clinical_age0 > 0 ? sum_p_clinical_age0 / n_p_clinical_age0 : 0.0;

    const double mean_immune =
        n_p_clinical_age0 > 0 ? sum_immunity_age0 / n_p_clinical_age0 : 0.0;

    const double mean_relative_biting_rate_age0 =
        n_relative_biting_rate_age0 > 0
            ? sum_relative_biting_rate_age0 / n_relative_biting_rate_age0
            : 0.0;

    const double mean_infection_probability_age0 =
        n_infection_probability_age0 > 0
            ? sum_infection_probability_age0 / n_infection_probability_age0
            : 0.0;

    const double foi_debug_weighted_biting_weight =
        foi_debug_sum > 0.0
            ? foi_debug_biting_weight_weighted_sum / foi_debug_sum
            : 0.0;

    const double foi_debug_weighted_relative_infectivity =
        foi_debug_sum > 0.0
            ? foi_debug_relative_infectivity_weighted_sum / foi_debug_sum
            : 0.0;

    const double foi_debug_weighted_log_density =
        foi_debug_sum > 0.0
            ? foi_debug_log_density_weighted_sum / foi_debug_sum
            : 0.0;

    const double foi_debug_weighted_genotype_share =
        foi_debug_sum > 0.0
            ? foi_debug_genotype_share_weighted_sum / foi_debug_sum
            : 0.0;

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
        << genotype_draw_failed_age0 << ","
        << mean_relative_biting_rate_age0 << ","
        << n_relative_biting_rate_age0 << ","
        << mean_infection_probability_age0 << ","
        << n_infection_probability_age0 << ","
        << clinical_scheduled_age0 << ","
        << clinical_count_age0 << ","
        << clinical_decision_age0 << ","
        << clinical_will_schedule_age0 << ","
        << clinical_count_age0_new_infection << ","
        << clinical_count_age0_duplicate_person << ","
        << clinical_count_age0_from_normal_progression << ","
        << clinical_count_age0_from_recurrence << ","
        << clinical_count_age0_from_unknown << ","
        << clinical_count_age0_duplicate_normal << ","
        << clinical_count_age0_duplicate_recurrence << ","
        << clinical_count_age0_duplicate_unknown << ","
        << clinical_count_age0_after_min_gap << ","
        << bite_eligible_persons_total << ","
        << bite_eligible_persons_age0 << ","
        << bite_eligible_age0_fraction << ","
        << bite_eligible_weight_total << ","
        << bite_eligible_weight_age0 << ","
        << bite_eligible_age0_weight_fraction << ","
        << bite_eligible_mean_weight_total << ","
        << bite_eligible_mean_weight_age0 << ","
        << bite_selected_persons_total << ","
        << bite_selected_persons_age0 << ","
        << bite_selected_age0_fraction << ","
        << bite_selected_mean_weight_total << ","
        << bite_selected_mean_weight_age0 << ","
        << foi_debug_rows << ","
        << foi_debug_sum << ","
        << foi_debug_weighted_biting_weight << ","
        << foi_debug_weighted_relative_infectivity << ","
        << foi_debug_weighted_log_density << ","
        << foi_debug_weighted_genotype_share << ","
        << mean_p << ","
        << mean_immune << "\n";

    reset_month(month + 1);
  }
};

extern DebugMonthlyStats DEBUG_MONTHLY_STATS;

#endif  // TMS_DEBUGMONTHLYSTATS_H