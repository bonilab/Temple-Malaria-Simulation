/* 
 * File:   Reporter.h
 * Author: Merlin
 *
 * Created on August 1, 2013, 12:05 PM
 */

#ifndef REPORTER_H
#define REPORTER_H

#include "Core/PropertyMacro.h"
#include <map>
#include <sstream>

class Model;

class Reporter {
 DISALLOW_COPY_AND_ASSIGN(Reporter)

 DISALLOW_MOVE(Reporter)

 POINTER_PROPERTY(Model, model)

  enum ReportType {
    CONSOLE,
    GUI,
    MONTHLY_REPORTER,
    MMC_REPORTER
  };

protected:
  std::stringstream ss;

 public:
  const std::string group_sep = "-1111\t";
  const std::string sep = "\t";

  static std::map<std::string, ReportType> ReportTypeMap;

  Reporter();

  virtual ~Reporter();

  virtual void initialize(int job_number, std::string path) = 0;

  virtual void before_run() = 0;

  virtual void after_run() = 0;

  virtual void begin_time_step() = 0;

  virtual void monthly_report() = 0;

  static Reporter *MakeReport(ReportType report_type);

};

#endif /* REPORTER_H */
