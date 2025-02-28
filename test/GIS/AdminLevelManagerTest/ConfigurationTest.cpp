#include "TestHelpers.h"

class ConfigurationTest : public AdminLevelManagerTestHelper {
};

TEST_CASE_METHOD(ConfigurationTest, "Indexing Configuration", "[AdminLevel][Config]") {
    SetUp();

    SECTION("Zero-based indexing") {
        create_test_raster("test_district.asc", true);  // zero-based
        
        REQUIRE_NOTHROW(manager.register_level("district"));
        REQUIRE_NOTHROW(manager.setup_boundary("district", 
            std::unique_ptr<AscFile>(AscFileManager::read("test_district.asc"))));
        
        const auto* boundary = manager.get_boundary("district");
        REQUIRE(boundary != nullptr);
        REQUIRE(boundary->first_index == 0);
    }

    SECTION("One-based indexing") {
        create_test_raster("test_district.asc", false);  // one-based
        
        REQUIRE_NOTHROW(manager.register_level("district"));
        REQUIRE_NOTHROW(manager.setup_boundary("district", 
            std::unique_ptr<AscFile>(AscFileManager::read("test_district.asc"))));
        
        const auto* boundary = manager.get_boundary("district");
        REQUIRE(boundary != nullptr);
        REQUIRE(boundary->first_index == 1);
    }

    SECTION("Different raster dimensions") {
        create_test_raster("test_district.asc", true, 5, 4);  // 5x4 raster
        
        REQUIRE_NOTHROW(manager.register_level("district"));
        REQUIRE_NOTHROW(manager.setup_boundary("district", 
            std::unique_ptr<AscFile>(AscFileManager::read("test_district.asc"))));
        
        const auto* boundary = manager.get_boundary("district");
        REQUIRE(boundary != nullptr);
        REQUIRE(boundary->raster->NROWS == 5);
        REQUIRE(boundary->raster->NCOLS == 4);
    }

    SECTION("Optional descriptions") {
        REQUIRE_NOTHROW(manager.register_level("district", "Health Districts"));
        const auto* boundary = manager.get_boundary("district");
        REQUIRE(boundary != nullptr);
        REQUIRE(boundary->description == "Health Districts");
    }

    TearDown();
}
