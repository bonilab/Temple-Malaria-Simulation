/*
 * Reporter.cpp
 *
 * Implements a factory pattern to generate the various reporter types.
 */

#include "Reporter.h"

#include "ConsoleReporter.h"
#include "MMCReporter.h"
#include "Model.h"
#include "MonthlyReporter.h"
#include "SQLiteMonthlyReporter.h"
#include "Specialist/AgeBandReporter.h"
#include "Specialist/CellularReporter.h"
#include "Specialist/NullReporter.hxx"
#include "Specialist/PopulationReporter.h"
#include "Specialist/SeasonalImmunity.h"
#include "easylogging++.h"

#ifdef ENABLE_TRAVEL_TRACKING
#include "Reporters/TravelTrackingReporter.h"
#endif

std::map<std::string, Reporter::ReportType> Reporter::ReportTypeMap{
    {"Console", CONSOLE},
    {"MonthlyReporter", MONTHLY_REPORTER},
    {"MMC", MMC_REPORTER},
    {"PopulationReporter", POPULATION_REPORTER},
    {"CellularReporter", CELLULAR_REPORTER},
    {"SeasonalImmunity", SEASONAL_IMMUNITY},
    {"AgeBand", AGE_BAND_REPORTER},
    {"SQLiteMonthlyReporter", SQLITE_MONTHLY_REPORTER},
#ifdef ENABLE_TRAVEL_TRACKING
    {"TravelTrackingReporter", TRAVEL_TRACKING_REPORTER},
#endif
    {"Null", NULL_REPORTER}};

Reporter* Reporter::MakeReport(ReportType report_type) {
  switch (report_type) {
    case CONSOLE:
      return new ConsoleReporter();
    case MONTHLY_REPORTER:
      return new MonthlyReporter();
    case MMC_REPORTER:
      return new MMCReporter();
    case POPULATION_REPORTER:
      return new PopulationReporter();
    case CELLULAR_REPORTER:
      return new CellularReporter();
    case SEASONAL_IMMUNITY:
      return new SeasonalImmunity();
    case AGE_BAND_REPORTER:
      return new AgeBandReporter();
    case SQLITE_MONTHLY_REPORTER:
      return new SQLiteMonthlyReporter();
#ifdef ENABLE_TRAVEL_TRACKING
    case TRAVEL_TRACKING_REPORTER:
      return new TravelTrackingReporter();
#endif
    case NULL_REPORTER:
      return new NullReporter();
    default:
      LOG(ERROR) << "No reporter type supplied";
      throw std::runtime_error("No reporter type supplied");
  }
}
