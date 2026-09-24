#include <catch2/catch.hpp>

#include "libslic3r/GCode/FilamentChangeTopology.hpp"

using namespace Slic3r;

TEST_CASE("Filament change topology separates nozzle and material actions", "[FilamentChangeTopology]")
{
    PhysicalNozzleStateRecorder recorder(2);
    REQUIRE(recorder.seed(0, 0));

    SECTION("same nozzle material change") {
        const auto topology = recorder.evaluate(1, 0, 0);
        REQUIRE(topology.has_value());
        REQUIRE(topology->target_residency_known);
        REQUIRE(topology->target_resident_filament_id == 0);
        REQUIRE(topology->actions.size() == 1);
        REQUIRE_FALSE(topology->has_action(FilamentChangeActionType::CrossNozzleSwitch));
        REQUIRE(topology->has_action(FilamentChangeActionType::SameNozzleSwitch));
    }

    SECTION("cross nozzle with unknown target") {
        const auto topology = recorder.evaluate(1, 0, 1);
        REQUIRE(topology.has_value());
        REQUIRE_FALSE(topology->target_residency_known);
        REQUIRE(topology->requires_initialization_flush);
        REQUIRE(topology->actions.size() == 2);
        REQUIRE(topology->actions[0].type == FilamentChangeActionType::CrossNozzleSwitch);
        REQUIRE(topology->actions[1].type == FilamentChangeActionType::InitializeTargetNozzle);
    }

    SECTION("cross nozzle with another resident filament") {
        REQUIRE(recorder.seed(1, 2));
        const auto topology = recorder.evaluate(1, 0, 1);
        REQUIRE(topology.has_value());
        REQUIRE(topology->target_resident_filament_id == 2);
        REQUIRE(topology->actions.size() == 2);
        REQUIRE(topology->actions[0].type == FilamentChangeActionType::CrossNozzleSwitch);
        REQUIRE(topology->actions[1].type == FilamentChangeActionType::SameNozzleSwitch);
    }

    SECTION("cross nozzle with requested filament already resident") {
        REQUIRE(recorder.seed(1, 1));
        const auto topology = recorder.evaluate(1, 0, 1);
        REQUIRE(topology.has_value());
        REQUIRE(topology->actions.size() == 1);
        REQUIRE(topology->actions[0].type == FilamentChangeActionType::CrossNozzleSwitch);
    }
}

TEST_CASE("Physical nozzle residency is committed without clearing source nozzle", "[FilamentChangeTopology]")
{
    PhysicalNozzleStateRecorder recorder(2);
    REQUIRE(recorder.seed(0, 0));
    REQUIRE(recorder.seed(1, 2));

    const auto topology = recorder.evaluate(1, 0, 1);
    REQUIRE(topology.has_value());
    REQUIRE(recorder.commit(*topology));
    REQUIRE(recorder.resident_filament(0) == 0);
    REQUIRE(recorder.resident_filament(1) == 1);

    const auto return_topology = recorder.evaluate(0, 1, 0);
    REQUIRE(return_topology.has_value());
    REQUIRE(return_topology->actions.size() == 1);
    REQUIRE(return_topology->has_action(FilamentChangeActionType::CrossNozzleSwitch));
}

TEST_CASE("First active filament initializes an unknown nozzle", "[FilamentChangeTopology]")
{
    PhysicalNozzleStateRecorder recorder(2);

    const auto topology = recorder.evaluate(0, -1, 0);
    REQUIRE(topology.has_value());
    REQUIRE_FALSE(topology->target_residency_known);
    REQUIRE(topology->requires_initialization_flush);
    REQUIRE(topology->actions.size() == 1);
    REQUIRE(topology->actions[0].type == FilamentChangeActionType::InitializeTargetNozzle);

    REQUIRE(recorder.commit(*topology));
    REQUIRE(recorder.resident_filament(0) == 0);
}

TEST_CASE("One-to-one mapping does not imply physical nozzle residency", "[FilamentChangeTopology]")
{
    PhysicalNozzleStateRecorder recorder(4);

    const auto topology = recorder.evaluate(1, 0, 1);
    REQUIRE(topology.has_value());
    REQUIRE_FALSE(topology->target_residency_known);
    REQUIRE(topology->target_resident_filament_id == -1);
    REQUIRE(topology->requires_initialization_flush);
    REQUIRE(topology->actions.size() == 2);
    REQUIRE(topology->actions[0].type == FilamentChangeActionType::CrossNozzleSwitch);
    REQUIRE(topology->actions[1].type == FilamentChangeActionType::InitializeTargetNozzle);

    REQUIRE(recorder.commit(*topology));
    const auto second_request = recorder.evaluate(1, 1, 1);
    REQUIRE(second_request.has_value());
    REQUIRE(second_request->target_residency_known);
    REQUIRE(second_request->actions.empty());
}

TEST_CASE("Invalid nozzle ids do not mutate residency", "[FilamentChangeTopology]")
{
    PhysicalNozzleStateRecorder recorder(2);
    REQUIRE_FALSE(recorder.seed(2, 0));
    REQUIRE_FALSE(recorder.evaluate(1, 0, 2).has_value());
    REQUIRE_FALSE(recorder.evaluate(1, 2, 0).has_value());
    REQUIRE(recorder.resident_filament(0) == -1);
    REQUIRE(recorder.resident_filament(1) == -1);
}
