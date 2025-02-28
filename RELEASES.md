# Releases

The simulation does not follow a fixed release schedule and new versions are typically finalized when a publication is
submitted for peer review. This also drives the feature that are added to the simulation since the current research needs
dictate the features currently under development.

## Change Log

### Version 4.2.0

See [Version 4.2.0 Release Notes](version_4.2.0.md) for details about the multi-level administrative boundaries update.

### Version 4.1.10

Version 4.1.10 improves memory management and code quality:

1. Enhanced SpatialData implementation with modern C++ practices:
   - Replaced raw pointer array with std::array of std::unique_ptr
   - Improved memory management with RAII principles
   - Better error handling and null pointer checks
   - More consistent pointer access patterns
   - Cleaner initialization and cleanup

2. Code quality improvements:
   - Removed manual memory management
   - Eliminated potential memory leaks
   - Improved type safety
   - Better const correctness
   - More consistent error handling

3. Performance considerations:
   - Optimized pointer access patterns
   - Reduced overhead from manual memory management
   - Cache-friendly data structures
   - Zero-cost abstractions for pointer management

4. Add UnitTest for SpatialData

These changes improve the robustness and maintainability of the spatial data handling system while maintaining the existing functionality and performance characteristics.

### Version 4.1.9

Version 4.1.9 enhances the seasonal modeling capabilities by adding district-specific seasonal patterns:

1. Enhanced SeasonalPattern implementation to support district-specific patterns:
   - Added support for both monthly (12 values) and daily (365 values) seasonal factors per district
   - Automatic conversion from monthly to daily data during initialization
   - Input via CSV file with format: `district_id,factor1,factor2,...,factor12|365`
   - Proper handling of leap years in seasonal calculations
   - Integration with district mapping from SpatialData
   - Robust error handling for:
     * Missing or invalid district data
     * Malformed CSV files
     * Negative seasonal factors
     * Non-existent input files
     * Incorrect number of values (must be exactly 12 for monthly or 365 for daily)

2. Configuration example for district-specific seasonal patterns:
```yaml
seasonal_info:
  enable: true
  mode: "PATTERN"
  pattern:
    filename: "district_seasonal_factors.csv"
    period: 12  # or 365 for daily data
```

The CSV input file format can be either monthly or daily:

For monthly data (12 values per district):
```csv
district_id,jan,feb,mar,apr,may,jun,jul,aug,sep,oct,nov,dec
1,0.5,0.55,0.6,0.65,0.7,0.8,0.9,0.85,0.7,0.65,0.6,0.55
2,0.6,0.65,0.7,0.75,0.8,0.9,1.0,0.95,0.8,0.75,0.7,0.65
```

For monthly data:
- Values are automatically expanded to daily values
- Each month's value is repeated for all days in that month
- Accounts for varying month lengths (28/29 for February, 30/31 for others)

For daily data (365 values per district):
```csv
district_id,day1,day2,day3,day4,day5,...,day364,day365
1,0.5,0.51,0.52,0.53,0.54,...,0.52,0.51
2,0.6,0.61,0.62,0.63,0.64,...,0.62,0.61
```

Notes:
- For leap years (day 366), the simulation will use the same value as day 365
- All seasonal factors must be non-negative
- District IDs must match those in the spatial configuration
- Missing or malformed data will cause the simulation to fail with an appropriate error message

### Version 4.1.8

Version 4.1.8 is currently in progress and will update the simuation to run in the compute environment at Temple University,
and includes the following updates:

1. Build script updated, most dependent libraries will be managed by vcpkg. As of 2024-01-30, the versions of dependent libraries are:
   - fmt 10.2.1
   - gsl 2.7.1#3
   - libpq 16.0
   - libpqxx 7.8.1
   - sqlite3 3.43.2#1
   - yaml-cpp 0.8.0#1
   - date 3.0.1#5
   - cli11 2.3.2
   - easyloggingpp 9.97.1
   - catch2 3.5.2
2. Added SQLiteDistrictReporter and SQLitePixelReporter to support reporting to SQLite database file.
3. Removal of last-change error handler in `main.cpp` for Linux platforms (will find a better solution later)
4. Add clang-format to the project to enforce code style.
5. Add clang-tidy to the project to enforce code quality.
6. Add TravelTracking feature to keep track and report the percentage of population travels in the past 30, 60, or 90 days.

- Two new attributes to the `Person` class to track the initiation days of the last trip and the last trip outside the district: `day_that_last_trip_was_initiated` and `day_that_last_trip_outside_district_was_initiated`.
- Compile-time flag `ENABLE_TRAVEL_TRACKING` to toggle the new travel tracking feature, allowing it to be enabled or disabled at compile time for performance considerations.
- Added TravelTrackingReporter to report (by district) the percentages of the population that have traveled in the past 30, 60, or 90 days to csv file named `travel_tracking_{job_id}.csv`
- Noted that, if a person traveled in the past 30 days, he will also be counted as traveled in the past 60 days and 90 days.

7. SQLiteDistrictReporter and SQLitePixelReporter report clinical episodes by age group (the structure of age group defined in the input file)

8. Introduce `relative_probability_that_child_travels_compared_to_adult`, refer to [Issue #7](https://github.com/bonilab/Temple-Malaria-Simulation/issues/7) for more details.

9. Introduces the MFTAgeBasedStrategy, an efficient mechanism for assigning therapies based on individual ages within the simulation. We've optimized therapy assignment by precomputing a mapping from integer ages to therapy indices, thus eliminating the need for repetitive boundary checks. Refer to [Issue #8](https://github.com/bonilab/Temple-Malaria-Simulation/issues/8) for more details.

#### Usage

To turn on the `TravelTracking` feature, you need to specify the `-DENABLE_TRAVEL_TRACKING=ON` option to `cmake` when building the simulation:

```sh
# Generate the build folder with cmake and turn on the `TravelTracking` feature
make generate ENABLE_TRAVEL_TRACKING=ON
make build
```

To use this `SQLiteDistrictReporter` reporter, you need to specify the `-r SQLiteDistrictReporter` option when running the simulation:

```sh
MaSim -i input.yaml -r SQLiteDistrictReporter -j 1 # this will generate a SQLite database named monthly_data_1.db
```

### Version 4.1.7.1

The version 4.1.7.1 release is the last version of the simulation while the project was based at Penn State, and
includes new features needed for research, with significant improvements to the documentation.

1. Added `change_circulation_percent_event`
2. Added `rotate_treatment_strategy_event`

### Version 4.1.7

Version 4.1.7 was an internal version focused on adding new features needed for research:

1. Added the `DistrictMftStrategy` treatment strategy to the simulation.

### Version 4.1.5

Version 4.1.5 was an internal version focused on adding new features needed for research:

1. Added the `WesolowskiSurface` movement model to the simulation.
2. Added the `AgeBandReporter` specialist reporter to the simulation.
3. Updated the default seeding of random numbers to use `std::random_device`.
4. Corrected district level reporting of infected individuals when genotype data is not also captured.

### Version 4.1.4

Version 4.1.4 was an internal version focused on adding new features needed for research:

1. Added `update_beta_raster_event`

### Version 4.1.3

Version 4.1.3 release was an internal version focused on adding new features needed for research:

1. Add the relevant classes and events needed for the RAPT protocol.
2. Additional input validation added for input data files.
3. Updated console logging to report physical and virtual memory usage at end of simulation.

### Version 4.1.2.1

The version 4.1.2.1 release is focused on bug fixes and adding new features needed for research:

1. Removed the configuration field `p_compliance`.
2. Updated simulation and configuration to support variable treatment compliance rates based upon the day of treatment.

### Version 4.1.1.1

Version 4.1.1.1 was an internal version specifically intended for work with manuscript revisions.

1. Updated the Marshall movement model to precompute the kernel.

### Version 4.1.1

The version 4.1.1 release is focused on adding new features needed for research:

1. Added new seasonality method based upon rainfall data.
2. Added new event (`introduce_mutant_event`) which allows mutations to be introduced in the population explicitly.
3. Added the genotype carriers reporter to the simulation.
4. Added the seasonal immunity reporter to the simulation.
5. Updated `DbReporter` to use ASCII character one (`SOH` or `☺`) to encode `\n\r` instead of removing them. Stored configurations can now have formatting restored.
6. Updated `-r` switch to support multiple comma delimited reporter types.
7. Refactored movement model for better flexibility, better Burkina Faso model performance.

### Version 4.1.0

The version 4.1.0 release focused on quality of life updates as well as correcting bugs found in the 4.0.0 release:

1. Added the `DbReporterDistrict` allowing for reporting data to be aggregated to the district level when reporting, database schema also updated to support this.
2. Weighted frequency has been removed from the genotype table (`sim.monthlygenomedata.weightedfrequency`), this will break code written for the 4.0.0 schema, but will reduce the storage requirements studies with many replicates and genotypes.
3. Deprecated values have been removed from the configuration (`min_clinical_probablity`).
4. Fixes for various bugs that were found in version 4.0.0.

### Version 4.0.0

The 4.0.0 release of MaSim marks a significant upgrade from previous versions, although backwards comparability has been maintained where possible. The major changes with this version of the model are:

1. Increased spatial support - models can now use a [ESRI ASCII Raster](http://resources.esri.com/help/9.3/arcgisengine/java/GP_ToolRef/spatial_analyst_tools/esri_ascii_raster_format.htm) for geographic data such as population distribution or a country's political organization.
2. Reporting of fine-grained information to a PostgreSQL database.
3. Reporting of agent movement during model execution.
