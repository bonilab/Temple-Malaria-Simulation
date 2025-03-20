#ifndef SEASONAL_PATTERN_FIXTURE_H
#define SEASONAL_PATTERN_FIXTURE_H

#include <fstream>
#include <string>

class SeasonalPatternFixture {
protected:
    const std::string monthly_file = "test_monthly_pattern.csv";
    const std::string daily_file = "test_daily_pattern.csv";

    void SetUp() {
        create_monthly_data();
        create_daily_data();
    }

    void TearDown() {
        std::remove(monthly_file.c_str());
        std::remove(daily_file.c_str());
    }

private:
    void create_monthly_data() {
        std::ofstream test_file(monthly_file);
        test_file << "admin_unit_id,jan,feb,mar,apr,may,jun,jul,aug,sep,oct,nov,dec\n";
        test_file << "1,0.5,0.6,0.7,0.8,0.9,1.0,1.1,1.0,0.9,0.8,0.7,0.6\n";
        test_file << "2,0.6,0.7,0.8,0.9,1.0,1.1,1.2,1.1,1.0,0.9,0.8,0.7\n";
        test_file.close();
    }

    void create_daily_data() {
        std::ofstream test_file(daily_file);
        test_file << "admin_unit_id";
        for (int i = 1; i <= 365; i++) {
            test_file << ",day" << i;
        }
        test_file << "\n1";
        for (int i = 1; i <= 365; i++) {
            test_file << "," << (0.5 + i * 0.001);
        }
        test_file << "\n2";
        for (int i = 1; i <= 365; i++) {
            test_file << "," << (0.5 + i * 0.001);
        }
        test_file << "\n";
        test_file.close();
    }
};

#endif 