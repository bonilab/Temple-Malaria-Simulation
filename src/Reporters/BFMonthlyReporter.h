//
// Created by Nguyen Tran on 3/5/2018.
//

#ifndef POMS_BFREPORTER_H
#define POMS_BFREPORTER_H


#include "Reporter.h"
#include <sstream>

class BFMonthlyReporter : public Reporter {
DISALLOW_COPY_AND_ASSIGN(BFMonthlyReporter)

public:
  std::stringstream ss;

  BFMonthlyReporter();
  virtual ~BFMonthlyReporter();

  void initialize() override;

  void before_run() override;

  void after_run() override;

  void begin_time_step() override;

  void monthly_report() override;

  void print_PfPR_0_5_by_location();

  void print_monthly_incidence_by_location();
};


#endif //POMS_BFREPORTER_H
