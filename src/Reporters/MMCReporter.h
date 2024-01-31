
#ifndef MMCREPORTER_H
#define MMCREPORTER_H

#include <sstream>

#include "Reporter.h"

class MMCReporter : public Reporter {
  DISALLOW_COPY_AND_ASSIGN(MMCReporter)

  DISALLOW_MOVE(MMCReporter)

public:
  MMCReporter();

  ~MMCReporter() override = default;

  void initialize(int job_number, std::string path) override {}

  void before_run() override;

  void after_run() override;

  void begin_time_step() override;

  void print_genotype_frequency();

  void monthly_report() override;

  void print_EIR_PfPR_by_location();
};

#endif  // MMCREPORTER_H
