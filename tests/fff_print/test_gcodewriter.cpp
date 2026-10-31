#include <catch2/catch.hpp>

#include <memory>

#include "libslic3r/GCodeWriter.hpp"

using namespace Slic3r;

TEST_CASE("Tower Z synchronization allows travel hops at accumulated layer heights", "[GCodeWriter][tower_z_sync]")
{
    const auto lift_type = GENERATE(LiftType::NormalLift, LiftType::LazyLift, LiftType::SpiralLift);
    GCodeWriter writer;
    writer.config.z_hop.values = {0.4};
    writer.config.retract_lift_above.values = {0.};
    writer.config.retract_lift_below.values = {254.};
    writer.set_extruders({0});
    writer.set_extruder(0);
    writer.set_current_position_clear(true);

    double nominal_z = 0.;
    for (int layer = 1; layer <= 188; ++layer) {
        nominal_z += 0.2;
        const double emitted_z = std::round(nominal_z * 1000.) / 1000.;
        CAPTURE(layer, nominal_z, emitted_z, static_cast<int>(lift_type));
        writer.set_position(Vec3d(112.5, 222.5, emitted_z));
        writer.sync_lifted_to_nominal_z(nominal_z);
        REQUIRE(writer.get_zhop() == 0.);

        writer.lift(lift_type);
        const std::string travel = writer.travel_to_xyz(Vec3d(115.183, 145.932, nominal_z));
        REQUIRE(travel.find('Z') != std::string::npos);
        REQUIRE(writer.get_position().z() == Approx(emitted_z + 0.4));
        REQUIRE(writer.get_zhop() == Approx(0.4));
        writer.unlift();
        REQUIRE(writer.get_position().z() == Approx(nominal_z));
    }
}

TEST_CASE("Tower Z synchronization preserves real hops and clears stale pending hops", "[GCodeWriter][tower_z_sync]")
{
    GCodeWriter writer;
    writer.config.z_hop.values = {0.4};
    writer.config.retract_lift_above.values = {0.};
    writer.config.retract_lift_below.values = {254.};
    writer.set_extruders({0});
    writer.set_extruder(0);
    writer.set_current_position_clear(true);
    writer.set_position(Vec3d(112.5, 222.5, 1.6));
    writer.lift(LiftType::LazyLift);

    SECTION("An actual hop is preserved without adding another hop") {
        writer.set_position(Vec3d(112.5, 222.5, 2.0));
        writer.sync_lifted_to_nominal_z(1.6);
        REQUIRE(writer.get_zhop() == Approx(0.4));
        writer.lift(LiftType::LazyLift);
        writer.travel_to_xyz(Vec3d(115.183, 145.932, 1.6));
        REQUIRE(writer.get_position().z() == Approx(2.0));
        writer.unlift();
        REQUIRE(writer.get_position().z() == Approx(1.6));
    }
    SECTION("A tower below the model does not retain a pending hop") {
        writer.sync_lifted_to_nominal_z(1.8);
        REQUIRE(writer.get_zhop() == 0.);
        writer.travel_to_xyz(Vec3d(115.183, 145.932, 1.8));
        REQUIRE(writer.get_position().z() == Approx(1.8));
        REQUIRE(writer.get_zhop() == 0.);
    }
}

SCENARIO("lift() is not ignored after unlift() at normal values of Z", "[GCodeWriter]") {
    GIVEN("A config from a file and a single extruder.") {
        GCodeWriter writer;
        GCodeConfig &config = writer.config;
        config.load(std::string(TEST_DATA_DIR) + "/fff_print_tests/test_gcodewriter/config_lift_unlift.ini", ForwardCompatibilitySubstitutionRule::Disable);

        std::vector<unsigned int> extruder_ids {0};
        writer.set_extruders(extruder_ids);
        writer.set_extruder(0);

        WHEN("Z is set to 203") {
            double trouble_Z = 203;
            writer.travel_to_z(trouble_Z);
            AND_WHEN("GcodeWriter::Lift() is called") {
                REQUIRE(writer.lift().size() > 0);
                AND_WHEN("Z is moved post-lift to the same delta as the config Z lift") {
                    REQUIRE(writer.travel_to_z(trouble_Z + config.retract_lift_below.values[0]).size() == 0);
                    AND_WHEN("GCodeWriter::Unlift() is called") {
                        REQUIRE(writer.unlift().size() == 0); // we're the same height so no additional move happens.
                        THEN("GCodeWriter::Lift() emits gcode.") {
                            REQUIRE(writer.lift().size() > 0);
                        }
                    }
                }
            }
        }
        WHEN("Z is set to 500003") {
            double trouble_Z = 500003;
            writer.travel_to_z(trouble_Z);
            AND_WHEN("GcodeWriter::Lift() is called") {
                REQUIRE(writer.lift().size() > 0);
                AND_WHEN("Z is moved post-lift to the same delta as the config Z lift") {
                    REQUIRE(writer.travel_to_z(trouble_Z + config.retract_lift_below.values[0]).size() == 0);
                    AND_WHEN("GCodeWriter::Unlift() is called") {
                        REQUIRE(writer.unlift().size() == 0); // we're the same height so no additional move happens.
                        THEN("GCodeWriter::Lift() emits gcode.") {
                            REQUIRE(writer.lift().size() > 0);
                        }
                    }
                }
            }
        }
        WHEN("Z is set to 10.3") {
            double trouble_Z = 10.3;
            writer.travel_to_z(trouble_Z);
            AND_WHEN("GcodeWriter::Lift() is called") {
                REQUIRE(writer.lift().size() > 0);
                AND_WHEN("Z is moved post-lift to the same delta as the config Z lift") {
                    REQUIRE(writer.travel_to_z(trouble_Z + config.retract_lift_below.values[0]).size() == 0);
                    AND_WHEN("GCodeWriter::Unlift() is called") {
                        REQUIRE(writer.unlift().size() == 0); // we're the same height so no additional move happens.
                        THEN("GCodeWriter::Lift() emits gcode.") {
                            REQUIRE(writer.lift().size() > 0);
                        }
                    }
                }
            }
        }
		// The test above will fail for trouble_Z == 9007199254740992, where trouble_Z + 1.5 will be rounded to trouble_Z + 2.0 due to double mantisa overflow.
    }
}

SCENARIO("set_speed emits values with fixed-point output.", "[GCodeWriter]") {

    GIVEN("GCodeWriter instance") {
        GCodeWriter writer;
        WHEN("set_speed is called to set speed to 99999.123") {
            THEN("Output string is G1 F99999.123") {
                REQUIRE_THAT(writer.set_speed(99999.123), Catch::Equals("G1 F99999.123\n"));
            }
        }
        WHEN("set_speed is called to set speed to 1") {
            THEN("Output string is G1 F1") {
                REQUIRE_THAT(writer.set_speed(1.0), Catch::Equals("G1 F1\n"));
            }
        }
        WHEN("set_speed is called to set speed to 203.200022") {
            THEN("Output string is G1 F203.2") {
                REQUIRE_THAT(writer.set_speed(203.200022), Catch::Equals("G1 F203.2\n"));
            }
        }
        WHEN("set_speed is called to set speed to 203.200522") {
            THEN("Output string is G1 F203.201") {
                REQUIRE_THAT(writer.set_speed(203.200522), Catch::Equals("G1 F203.201\n"));
            }
        }
    }
}
