#include "TestHelpers.h"

class ConfigurationTest : public AdminLevelManagerTestHelper {
};

TEST_CASE_METHOD(ConfigurationTest, "Indexing Configuration", "[AdminLevel][Config]") {
    SetUp();

    SECTION("Zero-based indexing") {
        create_test_raster("test_district.asc", true);  // zero-based
        auto raster = std::unique_ptr<AscFile>(AscFileManager::read("test_district.asc"));
        REQUIRE_NOTHROW(manager.register_level("district"));
        REQUIRE_NOTHROW(manager.setup_boundary("district", raster.get()));
        
        const auto* boundary = manager.get_boundary("district");
        REQUIRE(boundary != nullptr);
        REQUIRE(boundary->min_unit_id == 0);
        REQUIRE(boundary->max_unit_id == 2);
        REQUIRE(boundary->unit_count == 3);
    }

    SECTION("One-based indexing") {
        create_test_raster("test_district.asc", false);  // one-based
        
        REQUIRE_NOTHROW(manager.register_level("district"));
        auto raster = std::unique_ptr<AscFile>(AscFileManager::read("test_district.asc"));
        REQUIRE_NOTHROW(manager.setup_boundary("district", raster.get()));
        
        const auto* boundary = manager.get_boundary("district");
        REQUIRE(boundary != nullptr);
        REQUIRE(boundary->min_unit_id == 1);
        REQUIRE(boundary->max_unit_id == 3);
        REQUIRE(boundary->unit_count == 3);
    }

    TearDown();
}
