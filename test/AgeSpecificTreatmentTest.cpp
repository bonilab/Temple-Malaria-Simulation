#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <vector>
#include <stdexcept>
#include <cassert>
#include "Treatment/ITreatmentCoverageModel.h"
using Catch::Approx;


class TestTreatmentCoverageModel : public ITreatmentCoverageModel {
public:
    void monthly_update() override {
        // No-op for testing
    }

};

// -----------------------------
// Catch2 Test Cases
// -----------------------------

TEST_CASE("Treatment probability is correctly computed", "[treatment]") {
    auto* model = new TestTreatmentCoverageModel();
    model->p_treatment_base = {0.5, 0.6, 0.7}; // Base treatment probabilities for locations

    std::vector<int> age_classes = {5, 10, 15};
    std::vector<double> adjustments = {1.0, 0.8, 0.6};

    SECTION("Age falls into middle class") {
        double result = model->get_probability_to_be_treated(1, 7, age_classes, adjustments);
        REQUIRE(result == Approx(0.6 * 0.8));
    }

    SECTION("Age falls into first class") {
        double result = model->get_probability_to_be_treated(0, 3, age_classes, adjustments);
        REQUIRE(result == Approx(0.5 * 1.0));
    }

    SECTION("Age falls into last class") {
        double result = model->get_probability_to_be_treated(2, 12, age_classes, adjustments);
        REQUIRE(result == Approx(0.7 * 0.6));
    }

    SECTION("Throws if age does not match any class") {
        REQUIRE_THROWS_AS(model->get_probability_to_be_treated(0, 20, age_classes, adjustments), std::runtime_error);
    }

    SECTION("Throws on invalid location") {
        REQUIRE_THROWS_AS(model->get_probability_to_be_treated(3, 2, age_classes, adjustments), std::out_of_range);
    }
}