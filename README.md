# PSU-CIDD-Malaria-Simulation

[Penn State](https://www.psu.edu/) - [Center for Infectious Disease Dynamics (CIDD)](https://www.huck.psu.edu/institutes-and-centers/center-for-infectious-disease-dynamics) - [Boni Lab](http://mol.ax/)

---

## About

The HRSpatial branch of the Malaria Simulation (MaSim) marks a significant upgrade from previous versions, although backwards comparability has been maintained where possible. The three key changes with this version of the model are:

1. Increased spatial support - models can now use a [ESRI ASCII Raster](http://resources.esri.com/help/9.3/arcgisengine/java/GP_ToolRef/spatial_analyst_tools/esri_ascii_raster_format.htm) for geographic data such as population distribution or a country's political organization.

2. Reporting of fine grained information to a PostgreSQL database.

3. Reporting of agent movement during model execution.

---

## Documentation

The following commands are available from the simulation:
<pre>
-c / --config     Configuration file       
-i / --input

-l / --load       Load genotypes to the database and exit

-h / --help       Display this help menu
-j                The job number for this replicate
-r                The reporter type to use
-s                The study number to assoicate with the configuration

--dump            Dump the movement matrix as calculated
--im              Record individual movement detail
--mc              Record the movement between cells, cannot run with --md
--md              Record the movement between districts, cannot run with --mc

--v=[int]         Sets the verbosity of the logging, default zero
</pre>

When the `-r` switch is not supplied the simuation defaults to the `DbReporter`; however, with the `-r` switch the following reporters can be used:

CellularReporter - Generates two CSV files, one of which contains general popuation data and the second of which contains detailed data about the popuation for one year. 
