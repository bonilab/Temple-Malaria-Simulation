/*
 * Reporter.h
 *
 * Define various enumerations and constants related to reporters as well as the
 * factory pattern used to initialize reporters.
 */

#ifndef REPORTER_H
#define REPORTER_H

#include <map>
#include <sstream>

#include "Core/PropertyMacro.h"

class Model;

// Wrapper for TSV file constants
namespace Tsv {
const std::string sep = "\t";
const std::string end_line = "\n";
const std::string extension = "tsv";
}  // namespace Tsv

// Wrapper for CSV file constants
namespace Csv {
const std::string sep = ",";
const std::string end_line = "\n";
const std::string extension = "csv";
}  // namespace Csv

class Reporter {
  DELETE_COPY_AND_MOVE(Reporter)

  POINTER_PROPERTY(Model, model)

protected:
  // Constants used when generating TSV files
  const std::string group_sep = "-1111\t";

  std::stringstream ss;

public:
  enum ReportType {
    CONSOLE,
    GUI,
    MONTHLY_REPORTER,
    MMC_REPORTER,

    // Specialist reporters for specific experiments
    POPULATION_REPORTER,
    CELLULAR_REPORTER,
    SEASONAL_IMMUNITY,
    AGE_BAND_REPORTER,

    // SQLite reporter
    // SQLITE_DISTRICT_REPORTER,
    // SQLITE_PIXEL_REPORTER,
    SQLITE_MONTHLY_REPORTER,

    TRAVEL_TRACKING_REPORTER,
    // Null reporter used when the model needs to be initialized for access to
    // functionality
    NULL_REPORTER
  };

  static std::map<std::string, ReportType> ReportTypeMap;

  Reporter() : model_(nullptr) {}

  virtual ~Reporter() = default;

  virtual void initialize(int job_number, const std::string &path) = 0;

  virtual void before_run() = 0;

  virtual void after_run() = 0;

  virtual void begin_time_step() = 0;

  virtual void monthly_report() = 0;

  static Reporter* MakeReport(ReportType report_type);
};

#endif
