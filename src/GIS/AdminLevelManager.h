#ifndef ADMIN_LEVEL_MANAGER_H
#define ADMIN_LEVEL_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "Core/PropertyMacro.h"
#include "AscFile.h"
#include "easylogging++.h"


/**
* @struct BoundaryData
* @brief Contains all data related to a single administrative boundary level.
*
* This structure holds both the raw raster data and derived lookup information
* for efficient querying of administrative units and their associated locations.
*/
struct BoundaryData {
    // Maps location ID to its administrative unit ID for this level
    std::vector<int> location_to_unit;        
    // Maps unit ID to all locations within that unit
    std::vector<std::vector<int>> unit_to_locations;

    int min_unit_id{-1};                ///< Minimum unit ID in this level
    int max_unit_id{-1};                ///< Maximum unit ID in this level
    int unit_count{0};                  ///< Number of unique units in this level
};

/**
 * @class AdminLevelManager
 * @brief Manages multiple administrative boundary levels for spatial analysis.
 *
 * This class provides a flexible system for handling multiple administrative boundaries
 * (e.g., districts, provinces, health zones) in the simulation. It supports:
 * - Dynamic loading of administrative levels from raster files
 * - Efficient lookup between locations and administrative units
 * - Backward compatibility with existing district-based functionality
 *
 * Key features:
 * - Supports multiple user-defined administrative levels
 * - Maintains efficient integer-based indexing
 * - Handles both 0-based and 1-based raster indexing
 * - Enforces district level presence for backward compatibility
 *
 * Usage example:
 * @code
 * AdminLevelManager manager;
 * 
 * // Register and set up administrative levels
 * manager.register_level("district", "Health districts");
 * manager.setup_boundary("district", district_raster);
 * 
 * // Query administrative units
 * int district = manager.get_admin_unit(location_id, "district");
 * auto locations = manager.get_locations_in_unit(district_id, "district");
 * @endcode
 *
 * Configuration format:
 * @code
 * administrative_boundaries:
 *   - name: "district"    # Mandatory for backward compatibility
 *     raster: "path/to/district.asc"
 *     description: "Health districts"
 *   - name: "province"    # Optional additional levels
 *     raster: "path/to/province.asc"
 * @endcode
 *
 * @note The "district" level is mandatory if any administrative boundaries are used,
 *       to maintain backward compatibility with existing functionality.
 */
class AdminLevelManager {
private:

    // Efficient lookup storage
    std::unordered_map<std::string, int> name_to_id;  ///< Maps admin level names to internal admin IDs
    std::vector<std::string> id_to_name;              ///< Maps internal IDs to admin level names
    std::vector<BoundaryData> boundaries;             ///< Stores boundary data for each admin level
    
    bool has_district_{false};  ///< Tracks if mandatory district level is configured

public:
    AdminLevelManager() = default;
    ~AdminLevelManager() = default;

    // Delete copy and move to ensure singleton-like behavior
    AdminLevelManager(const AdminLevelManager&) = delete;
    AdminLevelManager& operator=(const AdminLevelManager&) = delete;
    AdminLevelManager(AdminLevelManager&&) = delete;
    AdminLevelManager& operator=(AdminLevelManager&&) = delete;

    /**
     * @brief Check if a specific administrative level exists
     * @param name The name of the administrative level
     * @return true if the level exists, false otherwise
     */
    bool has_level(const std::string& name) const {
        return name_to_id.find(name) != name_to_id.end();
    }

    /**
     * @brief Register a new administrative level
     * @param name The name of the administrative level
     * @return The ID assigned to this level
     * @throws std::runtime_error if name already exists
     */
    int register_level(const std::string& name);

    /**
     * @brief Set up boundary data for an administrative level
     * @param name The name of the administrative level
     * @param raster the raster file containing boundary data
     * @throws std::runtime_error if level doesn't exist or raster is invalid
     */
    void setup_boundary(const std::string& name, AscFile* raster);

    /**
     * @brief Get the admin unit ID for a location
     * @param level_name The name of the administrative level
     * @param location The location ID
     * @return The admin unit ID
     * @throws std::runtime_error if level doesn't exist
     * @throws std::out_of_range if location is invalid
     */
    int get_admin_unit(const std::string& level_name, int location) const;

    /**
     * @brief Get all locations in an administrative unit
     * @param level_name The name of the administrative level
     * @param unit_id The admin unit ID
     * @return Const reference to vector of location IDs
     * @throws std::runtime_error if level doesn't exist
     */
    const std::vector<int>& get_locations_in_unit(const std::string& level_name, int unit_id) const;

    /**
     * @brief Get boundary data for an administrative level
     * @param name The name of the administrative level
     * @return Pointer to boundary data, nullptr if not found
     */
    const BoundaryData* get_boundary(const std::string& name) const;

    /**
     * @brief Get the number of units in an administrative level
     * @param level_name The name of the administrative level
     * @return The number of units
     * @throws std::runtime_error if level doesn't exist
     */
    int get_unit_count(const std::string& level_name) const;

    /**
     * @brief Get all available administrative level names
     * @return Vector of level names
     */
    const std::vector<std::string>& get_level_names() const { return id_to_name; }

    /**
     * @brief Validate the configuration
     * @throws std::runtime_error if validation fails
     */
    void validate() const;

    const std::pair<int,int> get_units(const std::string& level_name) const;

private:

    /**
     * @brief Populate lookup data for a boundary
     * @note remember to check with LocationDB outside 
     * @param raster The raster file to populate
     * @return BoundaryData struct containing lookup data
     */
    BoundaryData populate_lookup(const AscFile* raster);

    /**
     * @brief Validate a raster file
     * @param raster The raster file to validate
     * @throws std::runtime_error if validation fails
     */
    void validate_raster(const AscFile* raster) const;
};

#endif // ADMIN_LEVEL_MANAGER_H
