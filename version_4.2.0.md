# Version 4.2.0 Release Notes

Version 4.2.0 introduces flexible multi-administrative boundaries support, allowing simulations to work with multiple geographic divisions simultaneously (districts, provinces, regions, etc.).

## ✅ Completed Features

1. Core Administrative Boundary System:
   - Support for multiple independent administrative levels (districts, provinces, etc.)
   - Efficient mapping between locations and administrative units
   - Mandatory district level for backward compatibility
   - Comprehensive validation of admin level registration

2. Data Structure Improvements:
   - Clear separation between admin levels, units, and locations
   - Efficient lookup tables for both directions:
     - Location to administrative unit mapping (O(1) lookup)
     - Administrative unit to locations mapping (O(1) lookup)
   - Support for both 0-based and 1-based unit indexing
   - Consistent boundary data structure across all admin levels

3. Integration with Existing Systems:
   - Seamless integration with SpatialData system
   - Backward compatibility with district-based functionality
   - Support for existing reporting systems
   - Integration with location database

4. Configuration System:
   - YAML configuration format for admin levels via `administrative_boundaries` section
   - Auto-detection of admin levels from configuration
   - Validation rules for boundary configurations
   - Error handling for invalid configurations

5. Testing:
   - Unit tests for basic operations
   - Validation tests for error cases
   - Integration tests with location system
   - Comprehensive test fixtures for different admin level scenarios

6. Multi-Level Admin Reporting:
   - SQLiteMonthlyReporter now reports data for all administrative levels
   - Each admin level has its own dedicated database tables:
     - `monthlysitedata_[level_name]` - Site data aggregated by admin level
     - `monthlygenomedata_[level_name]` - Genome data aggregated by admin level
   - Maintains backward compatibility with original district-level reporting
   - Performance optimizations for multiple level reporting
   - Consistent data aggregation methodology across all admin levels

## 🚧 Ongoing Improvements

1. Data Validation and Error Handling:
   - Additional validation of admin boundaries
   - More specific error messages
   - Validation of admin level independence
   - Additional coordinate system consistency checks

2. Reporting System:
   - Enhanced visualization options for multi-level reports
   - Additional aggregation metrics
   - Custom admin-level filtering options
   - Performance optimizations for large datasets

## 🔜 Planned for Future Releases

1. Advanced Features:
   - Efficient caching of boundary lookups
   - Advanced spatial relationship queries
   - Optimized memory usage for large datasets
   - Support for dynamic boundary changes

2. Documentation:
   - Complete API documentation
   - Migration guides from old district system
   - Configuration examples
   - Best practices guide

## Implementation Details

### Configuration Format
```yaml
administrative_boundaries:
  - name: "district"    # First admin level
    raster: "path/to/district.asc"
  - name: "province"    # Second additional level
    raster: "path/to/province.asc"
  - name: "region"      # Can add as many levels as needed
    raster: "path/to/region.asc"
```

### Data Structure
```cpp
struct BoundaryData {
    vector<int> location_to_unit;         // Maps locations to admin units
    vector<vector<int>> unit_to_locations;  // Maps units to their locations
    int min_unit_id;                      // Minimum unit ID (0 or 1)
    int max_unit_id;                      // Maximum unit ID
    int unit_count;                       // Number of unique units
}
```

### Public API
```cpp
// Key methods available in SpatialData
vector<string> get_admin_levels();                 // Get all available admin levels
bool has_admin_level(string level_name);           // Check if an admin level exists
pair<int,int> get_admin_units(string level_name);  // Get min/max unit IDs for a level
int get_unit_count(string level_name);             // Get number of units in a level
int get_admin_unit(string level_name, int location); // Get admin unit for a location
vector<int> get_locations_in_unit(string level_name, int unit); // Get locations in a unit
```

### Reporting Database Schema
Each administrative level now has dedicated database tables:

```sql
CREATE TABLE monthlysitedata_[level_name] (
    monthlydataid INTEGER NOT NULL,
    locationid INTEGER NOT NULL,  -- This is the admin unit ID for this level
    population INTEGER NOT NULL,
    clinicalepisodes INTEGER NOT NULL,
    -- Other fields...
    PRIMARY KEY (monthlydataid, locationid)
);

CREATE TABLE monthlygenomedata_[level_name] (
    monthlydataid INTEGER NOT NULL,
    locationid INTEGER NOT NULL,  -- This is the admin unit ID for this level
    genomeid INTEGER NOT NULL,
    -- Other fields...
    PRIMARY KEY (monthlydataid, genomeid, locationid)
);
```

Where `[level_name]` is the name of the administrative level as defined in your configuration. For example:
- `monthlysitedata_district` - For district-level data
- `monthlysitedata_province` - For province-level data
- `monthlysitedata_region` - For region-level data

This naming convention makes it easier to identify which table corresponds to which administrative level when querying the database.

### Validation Rules
1. Mandatory Requirements:
   - Valid raster files with consistent dimensions
   - Unit IDs must start from 0 or 1
   - No gaps in unit ID sequence

2. Error Handling:
   - Comprehensive validation of raster files
   - Registration validation
   - Dimension consistency checks
   - Unit ID range validation

## Technical Requirements
- C++17 compatible compiler
- Valid ASC format raster files
- Proper setup of location database
- Consistent coordinate systems across all admin level rasters

## Migration Guide
For existing projects using the district raster system, migration requires minimal changes:

1. Update YAML configurations to use the new format:
```yaml
# Old format
district_raster: "path/to/district.asc"

# New format
administrative_boundaries:
  - name: "district"
    raster: "path/to/district.asc"
```

2. Update API calls that previously accessed district information:
```cpp
// Old API calls
int district = location.district;
vector<int> locations = get_locations_in_district(district_id);

// New API calls
int district = spatial_data.get_admin_unit("district", location_id);
vector<int> locations = spatial_data.get_locations_in_unit("district", district_id);
```

3. Replace district-specific methods with the more general admin level methods:
```cpp
// Old method
int district_count = get_district_count();

// New method
int district_count = spatial_data.get_unit_count("district");
```

4. Add additional admin levels as needed:
```cpp
// Check if a specific admin level exists
if (spatial_data.has_admin_level("province")) {
    // Access province information
    int province = spatial_data.get_admin_unit("province", location_id);
    auto province_locations = spatial_data.get_locations_in_unit("province", province);
}
```

5. Access multi-level reports in the SQLite database:
```sql
-- Get data for district level
SELECT * FROM monthlysitedata_district;

-- Get data for province level
SELECT * FROM monthlysitedata_province;

-- Get data for specific unit in a specific level
SELECT * FROM monthlysitedata_province WHERE locationid = 3;
```

This new API provides a consistent interface for working with any administrative level while maintaining backward compatibility through the "district" level name.

_All features have been implemented and extensively tested. The API is now stable for production use._

### Integration with Seasonal Patterns

The SeasonalPattern system has been updated to work with the multi-administrative boundary system:

1. Configuration:
```yaml
seasonal_pattern:
  pattern:
    admin_level: "district"  # Can be any registered level: "district", "province", etc.
    filename: "seasonal_data.csv"
    period: 12  # 12 for monthly, 365 for daily
```

2. CSV File Format:

```csv
admin_unit_id,jan,feb,mar,apr,may,jun,jul,aug,sep,oct,nov,dec
1,0.8,1.0,1.2,1.5,1.8,2.0,1.8,1.5,1.2,1.0,0.8,0.7
2,0.7,0.8,1.0,1.2,1.5,1.8,2.0,1.8,1.5,1.2,1.0,0.8
```

3. Benefits:
   - Seasonal patterns can now be defined at any administrative level
   - Consistent API across all spatial components
   - Full backward compatibility with district-based seasonal patterns
   - Support for both 0-based and 1-based admin unit IDs

### Integration with IntroduceMutantEvent

The IntroduceMutantEvent system has been updated to support the multi-administrative boundary system:

1. YAML Configuration Format:
```yaml
# Introduce the 469Y mutant
- name: introduce_mutant_event
  admin_level: "district"  # Can be any registered level: "district", "province", etc.
  info:
    - day: 2006/6/3
      unit_id: 73
      fraction: 0.05339234151348579
      locus: 2
      mutant_allele: 1
```

2. Benefits:
   - Mutant introductions can be targeted to any administrative level
   - Full backward compatibility with district-based introductions
   - Support for both 0-based and 1-based admin unit IDs
   - Consistent API across all spatial components
   - Same mutation behavior, just with more flexible targeting options
