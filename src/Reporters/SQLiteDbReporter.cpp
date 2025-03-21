#include "SQLiteDbReporter.h"

#include <filesystem>

#include "Core/Config/Config.h"
#include "GIS/SpatialData.h"
#include "Helpers/StringHelpers.h"
#include "MDC/MainDataCollector.h"
#include "Model.h"
#include "Population/Population.h"
#include "easylogging++.h"

// Function to populate the 'genotype' table in the database
void SQLiteDbReporter::populate_genotype_table() {
  try {
    // Use the Database class to execute and prepare SQL statements
    db->execute("DELETE FROM genotype;");  // Clear the genotype table

    // Prepare the bulk query
    auto* stmt = db->prepare(insert_genotype_query_);

    auto* config = Model::CONFIG;

    for (auto id = 0; id < config->number_of_parasite_types(); id++) {
      auto* genotype = (*config->genotype_db())[id];
      // Bind values to the prepared statement
      sqlite3_bind_int(stmt, 1, id);
      sqlite3_bind_text(stmt, 2, genotype->to_string(config).c_str(), -1,
                        SQLITE_STATIC);

      if (sqlite3_step(stmt) != SQLITE_DONE) {
        throw std::runtime_error("Error executing INSERT statement");
      }

      sqlite3_reset(stmt);  // Reset the statement for the next iteration
    }

    sqlite3_finalize(stmt);  // Finalize the statement

  } catch (const std::exception &ex) {
    LOG(FATAL) << __FUNCTION__ << "-" << ex.what();
  }
}

// Function to populate the 'admin_level' table in the database
void SQLiteDbReporter::populate_admin_level_table() {
  try {
    // Clear the admin_level table
    db->execute("DELETE FROM admin_level;");

    // Prepare the bulk query
    auto* stmt = db->prepare(insert_admin_level_query_);

    // Get admin level names from SpatialData
    auto admin_levels = SpatialData::get_instance().get_admin_level_manager()->get_level_names();

    if (admin_levels.empty()) {
      LOG(WARNING) << "No admin levels found. Skipping admin_level table population.";
      return;
    }

    for (size_t id = 0; id < admin_levels.size(); id++) {
      // Bind values to the prepared statement
      sqlite3_bind_int(stmt, 1, id);
      sqlite3_bind_text(stmt, 2, admin_levels[id].c_str(), -1, SQLITE_STATIC);

      if (sqlite3_step(stmt) != SQLITE_DONE) {
        throw std::runtime_error("Error executing INSERT statement for admin_level");
      }

      sqlite3_reset(stmt);  // Reset the statement for the next iteration
    }

    sqlite3_finalize(stmt);  // Finalize the statement

  } catch (const std::exception &ex) {
    LOG(FATAL) << __FUNCTION__ << "-" << ex.what();
  }
}

// Function to create tables for each admin level
void SQLiteDbReporter::create_all_reporting_tables() {
  auto admin_levels = SpatialData::get_instance().get_admin_level_manager()->get_level_names();
  

  std::string ageClassColumnDefinitions;
  for (auto ndx = 0; ndx < Model::CONFIG->age_structure().size(); ndx++) {
    auto agFrom = ndx == 0 ? 0 : Model::CONFIG->age_structure()[ndx - 1];
    auto agTo = Model::CONFIG->age_structure()[ndx];
    ageClassColumnDefinitions +=
        fmt::format("clinical_episodes_by_age_class_{}_{} INTEGER, ", agFrom, agTo);
  }

  std::string ageClassColumns;
  for (auto ndx = 0; ndx < Model::CONFIG->age_structure().size(); ndx++) {
    auto agFrom = ndx == 0 ? 0 : Model::CONFIG->age_structure()[ndx - 1];
    auto agTo = Model::CONFIG->age_structure()[ndx];
    ageClassColumns +=
        fmt::format("clinical_episodes_by_age_class_{}_{}, ", agFrom, agTo);
  }

  // Resize the query prefix vectors to include all admin levels plus cell level
  insert_site_query_prefixes_.resize(admin_levels.size() + 1);
  insert_genome_query_prefixes_.resize(admin_levels.size() + 1);
  
  // First create the cell-level tables
  create_reporting_tables_for_level(CELL_LEVEL_ID, ageClassColumnDefinitions, ageClassColumns);
  
  // Now create tables for each admin level
  for (size_t level_id = 0; level_id < admin_levels.size(); level_id++) {
    create_reporting_tables_for_level(level_id, ageClassColumnDefinitions, ageClassColumns);
  }
}

void SQLiteDbReporter::create_reporting_tables_for_level(int level_id, const std::string& ageClassColumnDefinitions, const std::string& ageClassColumns) {
  // Generate table names for this level
  std::string site_table_name = get_site_table_name(level_id);
  std::string genome_table_name = get_genome_table_name(level_id);

  // location_id for cell level or unit_id for admin level
  std::string location_id_column = (level_id == CELL_LEVEL_ID) ? "location_id" : "unit_id";
  
  // Create site data table for this level
  std::string createSiteDataTable = 
    fmt::format(R""""(
      CREATE TABLE IF NOT EXISTS {} (
          monthly_data_id INTEGER NOT NULL,
          {} INTEGER NOT NULL,
          population INTEGER NOT NULL,
          clinical_episodes INTEGER NOT NULL, )"""", site_table_name, location_id_column)
    + ageClassColumnDefinitions +
    fmt::format(R""""(
          treatments INTEGER NOT NULL,
          treatment_failures INTEGER NOT NULL,
          eir REAL NOT NULL,
          pfpr_under5 REAL NOT NULL,
          pfpr_2to10 REAL NOT NULL,
          pfpr_all REAL NOT NULL,
          infected_individuals INTEGER,
          non_treatment INTEGER NOT NULL,
          under5_treatment INTEGER NOT NULL,
          over5_treatment INTEGER NOT NULL,
          PRIMARY KEY (monthly_data_id, {}),
          FOREIGN KEY (monthly_data_id) REFERENCES monthly_data(id)
      );
    )"""",location_id_column);
  
  // Create genome data table for this level
  std::string createGenomeDataTable = 
    fmt::format(R""""(
      CREATE TABLE IF NOT EXISTS {} (
          monthly_data_id INTEGER NOT NULL,
          {} INTEGER NOT NULL,
          genome_id INTEGER NOT NULL,
          occurrences INTEGER NOT NULL,
          clinical_occurrences INTEGER NOT NULL,
          occurrences_0to5 INTEGER NOT NULL,
          occurrences_2to10 INTEGER NOT NULL,
          weighted_occurrences REAL NOT NULL,
          PRIMARY KEY (monthly_data_id, genome_id, {}),
          FOREIGN KEY (genome_id) REFERENCES genotype(id),
          FOREIGN KEY (monthly_data_id) REFERENCES monthly_data(id)
      );
    )"""", genome_table_name, location_id_column, location_id_column);
  try {
    // Execute the creation queries
    db->execute(createSiteDataTable);
    db->execute(createGenomeDataTable);
    
    // Determine index for query prefixes
    int prefix_index = (level_id == CELL_LEVEL_ID) ? 
                      insert_site_query_prefixes_.size() - 1 : level_id;
    
    // Create insert query prefixes for this level
    insert_site_query_prefixes_[prefix_index] = 
      fmt::format("INSERT INTO {} (monthly_data_id, {}, "
        "population, clinical_episodes, ", site_table_name, location_id_column) 
      + ageClassColumns + 
      " treatments, eir, pfpr_under5, pfpr_2to10, pfpr_all, infected_individuals, treatment_failures,"
      " non_treatment, under5_treatment, over5_treatment) VALUES";
    
    insert_genome_query_prefixes_[prefix_index] = 
      fmt::format(R"""(
        INSERT INTO {} 
        (monthly_data_id, {}, genome_id, occurrences, 
        clinical_occurrences, occurrences_0to5, occurrences_2to10, 
        weighted_occurrences) 
        VALUES 
      )""", genome_table_name, location_id_column);
  } catch (const std::exception &ex) {
    LOG(ERROR) << "Error creating tables for level " << level_id << ": " << ex.what();
  }
}

// Function to populate the 'location_admin_map' table in the database
void SQLiteDbReporter::populate_location_admin_map_table() {
  try {
    // Clear the table
    db->execute("DELETE FROM location_admin_map;");

    // Prepare the bulk query
    auto* stmt = db->prepare(insert_location_admin_map_query_);

    auto& spatial_data = SpatialData::get_instance();
    auto& location_db = Model::CONFIG->location_db();
    auto admin_level_manager = spatial_data.get_admin_level_manager();
    
    // For each location in the location database
    for (int location_id = 0; location_id < location_db.size(); location_id++) {
      // For each admin level
      for (int admin_level_id = 0; admin_level_id < admin_level_manager->get_level_count(); admin_level_id++) {
        // Get admin unit ID for this location at this admin level
        auto admin_unit_id = spatial_data.get_admin_unit(admin_level_id, location_id);
        
        // Bind values to the prepared statement
        sqlite3_bind_int(stmt, 1, location_id);
        sqlite3_bind_int(stmt, 2, admin_level_id);
        sqlite3_bind_int(stmt, 3, admin_unit_id);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
          throw std::runtime_error("Error executing INSERT statement for location_admin_map");
        }

        sqlite3_reset(stmt);  // Reset the statement for the next iteration
      }
    }

    sqlite3_finalize(stmt);  // Finalize the statement

  } catch (const std::exception &ex) {
    LOG(ERROR) << __FUNCTION__ << "-" << ex.what();
  }
}

// Function to create the database schema
// This sets up the necessary tables in the database
void SQLiteDbReporter::populate_db_schema() {
  // Create the table schema
  const std::string createMonthlyData = R""""(
    CREATE TABLE IF NOT EXISTS monthly_data (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        days_elapsed INTEGER NOT NULL,
        model_time INTEGER NOT NULL,
        seasonal_factor INTEGER NOT NULL
    );
  )"""";

  const std::string createGenotype = R""""(
    CREATE TABLE IF NOT EXISTS genotype (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL
    );
  )"""";

  const std::string createAdminLevel = R""""(
    CREATE TABLE IF NOT EXISTS admin_level (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL
    );
  )"""";
  
  const std::string createLocationAdminMap = R""""(
    CREATE TABLE IF NOT EXISTS location_admin_map (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        location_id INTEGER NOT NULL,
        admin_level_id INTEGER NOT NULL,
        admin_unit_id INTEGER NOT NULL,
        FOREIGN KEY (admin_level_id) REFERENCES admin_level(id)
    );
    CREATE INDEX IF NOT EXISTS idx_admin_unit ON location_admin_map (admin_level_id, admin_unit_id);
    CREATE UNIQUE INDEX IF NOT EXISTS idx_location_admin ON location_admin_map (location_id, admin_level_id);
  )"""";

  try {
    TransactionGuard tx(db.get());
    // Use the Database class to execute SQL statements
    db->execute(createMonthlyData);
    db->execute(createGenotype);
    db->execute(createAdminLevel);
    db->execute(createLocationAdminMap);
    
    // Create tables for all reporting levels (cell and admin)
    create_all_reporting_tables();
    
  } catch (const std::exception &ex) {
    LOG(ERROR) << "Error in populate_db_schema: " << ex.what();
    // Consider more robust error handling rather than simply logging
  }
}

// Initialize the reporter
// Sets up the database and prepares it for data entry
void SQLiteDbReporter::initialize(int jobNumber, const std::string &path) {
  VLOG(1) << "Base SQLiteDbReporter initialized.\n";

  // Define the database file path
  auto dbPath = fmt::format("{}monthly_data_{}.db", path, jobNumber);

  // Check if the file exists
  if (std::filesystem::exists(dbPath)) {
    // Delete the old database file if it exists
    if (std::remove(dbPath.c_str()) != 0) {
      // Handle the error, if any, when deleting the old file
      LOG(ERROR) << "Error deleting old database file.";
    }
  } else {
    // The file doesn't exist, so no need to delete it
    LOG(INFO) << "Database file does not exist. No deletion needed.\n";
  }

  // Open or create the SQLite database file
  db = std::make_unique<SQLiteDatabase>(dbPath);

  // Get number of admin levels to initialize vectors
  int admin_level_count = SpatialData::get_instance().get_admin_level_manager()->get_level_names().size();
  insert_site_query_prefixes_.resize(admin_level_count);
  insert_genome_query_prefixes_.resize(admin_level_count);

  populate_db_schema();
  // populate the genotype table
  populate_genotype_table();
  // populate the admin level table
  populate_admin_level_table();
  populate_location_admin_map_table();

  std::string ageClassColumns;
  for (auto ndx = 0; ndx < Model::CONFIG->age_structure().size(); ndx++) {
    auto agFrom = ndx == 0 ? 0 : Model::CONFIG->age_structure()[ndx - 1];
    auto agTo = Model::CONFIG->age_structure()[ndx];
    ageClassColumns +=
        fmt::format("clinical_episodes_by_age_class_{}_{}, ", agFrom, agTo);
  }
}

void SQLiteDbReporter::monthly_report() {
  // Get the relevant data
  auto daysElapsed = Model::SCHEDULER->current_time();
  auto modelTime =
      std::chrono::system_clock::to_time_t(Model::SCHEDULER->calendar_date);
  auto seasonalFactor = Model::CONFIG->seasonal_info()->get_seasonal_factor(
      Model::SCHEDULER->calendar_date, 0);

  auto monthId = db->insert_data(insert_common_query_, daysElapsed, modelTime,
                                 seasonalFactor);

  monthly_report_site_data(monthId);
  if (Model::CONFIG->record_genome_db()
      && Model::MAIN_DATA_COLLECTOR->recording_data()) {
    // Add the genome information, this will also update infected individuals
    monthly_report_genome_data(monthId);
  }
}

void SQLiteDbReporter::insert_monthly_site_data(int level_id,
    const std::vector<std::string> &siteData) {
  // Skip if empty
  if (siteData.empty()) return;
    
  // For cell level, use the last index in the query prefix vector
  int query_index = (level_id == CELL_LEVEL_ID) ? 
                    insert_site_query_prefixes_.size() - 1 : level_id;
    
  // Insert the site data into the database
  std::string query = insert_site_query_prefixes_[query_index] + 
                      StringHelpers::join(siteData, ",") + ";";
  db->execute(query);
}

void SQLiteDbReporter::insert_monthly_genome_data(int level_id,
    const std::vector<std::string> &genomeData) {
  // Skip if empty
  if (genomeData.empty()) return;
    
  // For cell level, use the last index in the query prefix vector
  int query_index = (level_id == CELL_LEVEL_ID) ? 
                    insert_genome_query_prefixes_.size() - 1 : level_id;
    
  // Insert the genome data into the database
  std::string query = insert_genome_query_prefixes_[query_index] +
                      StringHelpers::join(genomeData, ",") + ";";
  db->execute(query);
}

std::string SQLiteDbReporter::get_site_table_name(int level_id) const {
  if (level_id == CELL_LEVEL_ID) {
    return "monthly_site_data_cell";
  }
  return "monthly_site_data_" + SpatialData::get_instance().get_admin_level_name(level_id);
}

std::string SQLiteDbReporter::get_genome_table_name(int level_id) const {
  if (level_id == CELL_LEVEL_ID) {
    return "monthly_genome_data_cell";
  }
  return "monthly_genome_data_" + SpatialData::get_instance().get_admin_level_name(level_id);
}

const std::string insert_location_admin_map_query_ =
      "INSERT INTO location_admin_map (location_id, admin_level_id, admin_unit_id) VALUES (?, ?, ?);";

