#include "SQLiteDbReporter.h"
#include "Core/Config/Config.h"
#include "MDC/ModelDataCollector.h"
#include "Model.h"
#include "Population/Population.h"
#include "easylogging++.h"
#include <filesystem>

// Function to populate the 'genotype' table in the database
void SQLiteDbReporter::populate_genotype_table() {
  const std::string INSERT_GENOTYPE =
      "INSERT INTO genotype (id, name) VALUES (?, ?);";

  try {
    // Use the Database class to execute and prepare SQL statements
    db->execute("DELETE FROM genotype;"); // Clear the genotype table

    // Prepare the bulk query
    auto stmt = db->prepare(INSERT_GENOTYPE);

    auto *config = Model::CONFIG;

    for (auto id = 0ul; id < config->number_of_parasite_types(); id++) {
      auto genotype = (*config->genotype_db())[id];
      // Bind values to the prepared statement
      sqlite3_bind_int(stmt, 1, id);
      sqlite3_bind_text(stmt, 2, genotype->to_string(config).c_str(), -1,
                        SQLITE_STATIC);

      if (sqlite3_step(stmt) != SQLITE_DONE) {
        throw std::runtime_error("Error executing INSERT statement");
      }

      sqlite3_reset(stmt); // Reset the statement for the next iteration
    }

    sqlite3_finalize(stmt); // Finalize the statement

  } catch (const std::exception &ex) {
    LOG(ERROR) << __FUNCTION__ << "-" << ex.what();
    exit(1);
  }
}

// Function to create the database schema
// This sets up the necessary tables in the database
void SQLiteDbReporter::populate_db_schema() {
  // Create the table schema
  const std::string createMonthlyData = R""""(
    CREATE TABLE IF NOT EXISTS monthlydata (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        dayselapsed INTEGER NOT NULL,
        modeltime INTEGER NOT NULL,
        seasonalfactor INTEGER NOT NULL
    );
  )"""";

  const std::string createMonthlySiteData = R""""(
    CREATE TABLE IF NOT EXISTS monthlysitedata (
        monthlydataid INTEGER NOT NULL,
        locationid INTEGER NOT NULL,
        population INTEGER NOT NULL,
        clinicalepisodes INTEGER NOT NULL,
        treatments INTEGER NOT NULL,
        treatmentfailures INTEGER NOT NULL,
        eir REAL NOT NULL,
        pfprunder5 REAL NOT NULL,
        pfpr2to10 REAL NOT NULL,
        pfprall REAL NOT NULL,
        infectedindividuals INTEGER,
        nontreatment INTEGER NOT NULL,
        under5treatment INTEGER NOT NULL,
        over5treatment INTEGER NOT NULL,
        PRIMARY KEY (monthlydataid, locationid),
        FOREIGN KEY (monthlydataid) REFERENCES monthlydata(id)
    );
  )"""";

  const std::string createGenotype = R""""(
    CREATE TABLE IF NOT EXISTS genotype (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL
    );
  )"""";

  const std::string createMonthlyGenomeData = R""""(
    CREATE TABLE IF NOT EXISTS monthlygenomedata (
        monthlydataid INTEGER NOT NULL,
        locationid INTEGER NOT NULL,
        genomeid INTEGER NOT NULL,
        occurrences INTEGER NOT NULL,
        clinicaloccurrences INTEGER NOT NULL,
        occurrences0to5 INTEGER NOT NULL,
        occurrences2to10 INTEGER NOT NULL,
        weightedoccurrences REAL NOT NULL,
        PRIMARY KEY (monthlydataid, genomeid, locationid),
        FOREIGN KEY (genomeid) REFERENCES genotype(id),
        FOREIGN KEY (monthlydataid) REFERENCES monthlydata(id)
    );
  )"""";

  try {
    // Use the Database class to execute SQL statements
    db->execute(createMonthlyData);
    db->execute(createMonthlySiteData);
    db->execute(createGenotype);
    db->execute(createMonthlyGenomeData);
  } catch (const std::exception &ex) {
    LOG(ERROR) << "Error in populate_db_schema: " << ex.what();
    // Consider more robust error handling rather than simply logging
  }
}

// Initialize the reporter
// Sets up the database and prepares it for data entry
void SQLiteDbReporter::initialize(int job_number, std::string path) {
  VLOG(1) << "Base SQLiteDbReporter initialized." << std::endl;

  // Define the database file path
  auto dbPath = fmt::format("{}monthly_data_{}.db", path, job_number);

  // Check if the file exists
  if (std::filesystem::exists(dbPath)) {
    // Delete the old database file if it exists
    if (std::remove(dbPath.c_str()) != 0) {
      // Handle the error, if any, when deleting the old file
      LOG(ERROR) << "Error deleting old database file.";
    }
  } else {
    // The file doesn't exist, so no need to delete it
    LOG(INFO) << "Database file does not exist. No deletion needed."
              << std::endl;
  }
  int result;

  // Open or create the SQLite database file
  db = std::make_unique<SQLiteDatabase>(dbPath);

  populate_db_schema();
  // populate the genotype table
  populate_genotype_table();
}

void SQLiteDbReporter::monthly_report() {

  // Get the relevant data
  auto days_elapsed = Model::SCHEDULER->current_time();
  auto model_time =
      std::chrono::system_clock::to_time_t(Model::SCHEDULER->calendar_date);
  auto seasonal_factor = Model::CONFIG->seasonal_info()->get_seasonal_factor(
      Model::SCHEDULER->calendar_date, 0);

  auto id =
      db->insert_data(INSERT_COMMON, days_elapsed, model_time, seasonal_factor);

  std::string query = "";
  monthly_site_data(id);
  if (Model::CONFIG->record_genome_db() &&
      Model::DATA_COLLECTOR->recording_data()) {
    // Add the genome information, this will also update infected individuals
    monthly_genome_data(id);
  } else {
    // If we aren't recording genome data still update the infected
    // individuals
    monthly_infected_individuals(id);
  }
}
