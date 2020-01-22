/*
 * MovementReporter.h
 * 
 * Define the MovementReporter class which is used to insert movement information into
 * the database. When running for large populations over long periods of time, this will
 * be a bit quicker since updates are buffered until the end of the time step.
 */
#ifndef MOVEMENTREPORTER_H
#define MOVEMENTREPORTER_H

#include "Reporter.h"

#include <pqxx/pqxx>
#include <vector>

class MovementReporter : public Reporter {
    private:
        const std::string INSERT_FINE_MOVE = 
        "INSERT INTO sim.Movement (ReplicateId, Timestep, IndividualId, Source, Destination) "
        "VALUES ({}, {}, {}, {}, {});";

        const std::string INSERT_COARSE_MOVE = 
        "INSERT INTO sim.DistrictMovement (ReplicateId, Timestep, Count, Source, Destination) "
        "VALUES ({}, {}, {}, {}, {});";

        // NOTE This may not always work if the same random number is reused.
        //      However, this assumed to be a testing / early validation reporter
        //      as opposed to one being used to generate manuscript data.
        const std::string SELECT_REPLICATE = 
        "SELECT id FROM sim.replicate WHERE seed = {}";

        int count = 0;
        int replicate;
        pqxx::connection* conn;
        std::string fine_update_query;

        int division_count;
        int** movement_counts = nullptr;

    public:
        MovementReporter() = default;
        ~MovementReporter() override = default;

        // Basic declarations
        void before_run() override {}
        void begin_time_step() override {}

        // Overrides
        void initialize(int job_number, std::string path) override;
        void monthly_report() override;
        void after_run() override;

        void coarse_report();
        void add_coarse_move(int individual, int source, int destination);
        void add_fine_move(int individual, int source, int destination);
        void fine_report();
};

#endif