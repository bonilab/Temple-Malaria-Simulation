# Releases

The simulation does not follow a fixed release schedule and new versions are typically finalized when a publication is 
submitted for peer review. This also drives the feature that are added to the simulation since the current research needs
dictate the features currently under development.

## Change Log
### Version 4.1.8

Version 4.1.8 is currently in progress and will update the simuation to run in the compute environment at Temple University, 
and includes the following updates:

1. Build script updated, most dependent libraries will be managed by vcpkg. As of 2024-01-20, the versions of dependent libraries are:
    - fmt 10.2.1
    - gsl 2.7.1#3
    - libpq 16.0
    - libpqxx 7.8.1
    - sqlite3 3.43.2#1
    - yaml-cpp 0.8.0#1
    - date 3.0.1#5
    - cli11 2.3.2
2. Added SQLiteDistrictReporter and SQLitePixelReporter to support reporting to SQLite database file.
3. Removal of last-change error handler in `main.cpp` for Linux platforms (will find a better solution later)

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
