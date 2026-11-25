#include <catch2/catch.hpp>
#include "libslic3r/FDM/WipeTowerTiming.hpp"
#include "libslic3r/GCode/GCodeProcessor.hpp"
#include "libslic3r/Print.hpp"

using namespace Slic3r;

TEST_CASE("Tower timing reuses model time between the same filament changes", "[WipeTowerTiming]")
{
    Print print;
    GCodeProcessorResult result;
    WipeTowerTiming trace;
    trace.change(0, 0, 1, 10., 1., 2.f, 62.f, result);
    trace.change(1, 1, 0, 40., 1., 2.f, 62.f, result);
    WipeTowerTiming::Layout baseline{
        {0, 0, 1, .2, 0., Vec2d::Zero(), Vec2d::Zero()},
        {1, 1, 0, .4, 5., Vec2d::Zero(), Vec2d::Zero()}
    };
    auto next = baseline;
    next[1].tower_time += 10.;

    SECTION("An extra ten tower seconds affect the returning filament") {
        auto volumes = trace.predict(baseline, next, print);
        REQUIRE(volumes);
        REQUIRE(volumes->at(0) == Approx(12.f));
        REQUIRE(volumes->at(1) == Approx(42.f));
    }
    SECTION("A different tool order requires a new measurement") {
        next[1].new_tool = 2;
        REQUIRE_FALSE(trace.predict(baseline, next, print));
    }
    SECTION("A new periodic operation after the last change does not invalidate the tower") {
        trace.start_periodic(0.);
        trace.periodic_site(41., 45., 1, WipeTowerTiming::SiteKind::Tower, 0);
        auto volumes = trace.predict(baseline, next, print);
        REQUIRE(volumes);
        REQUIRE(volumes->at(1) == Approx(42.f));
    }
    SECTION("A new operation before a returning tool requires a measured script duration") {
        WipeTowerTiming measured;
        measured.start_periodic(0.);
        measured.change(0, 0, 1, 10., 1., 2.f, 62.f, result);
        REQUIRE_FALSE(measured.periodic_site(35., 40., 0, WipeTowerTiming::SiteKind::Tower, 0));
        measured.change(1, 1, 0, 40., 1., 2.f, 62.f, result);
        REQUIRE_FALSE(measured.predict(baseline, next, print));
    }
    SECTION("An off-plate flush between the change and its wipe preserves the timed tower volume") {
        trace.start_periodic(0.);
        trace.periodic_site(41., 30., 1, WipeTowerTiming::SiteKind::Tower, 0);
        trace.flushed(43., 30.);
        auto volumes = trace.predict(baseline, next, print);
        REQUIRE(volumes);
        REQUIRE(volumes->at(1) == Approx(42.f));
    }
    SECTION("Elapsed off-plate time counts towards the returning filament's idle time") {
        // The measured interval from 10 s to 40 s already includes the flush.
        // The extra 10 s in the new tower still produces a 40 s total idle time.
        WipeTowerTiming measured;
        measured.start_periodic(0.);
        measured.change(0, 0, 1, 10., 1., 2.f, 62.f, result);
        REQUIRE(measured.periodic_site(15., 15., 0, WipeTowerTiming::SiteKind::Tower, 1));
        measured.flushed(25., 15.);
        measured.change(1, 1, 0, 40., 1., 2.f, 62.f, result);
        auto volumes = measured.predict(baseline, next, print);
        REQUIRE(volumes);
        REQUIRE(volumes->at(0) == Approx(12.f));
        REQUIRE(volumes->at(1) == Approx(42.f));
    }
    SECTION("A script that changes its next interval gets exact timing") {
        trace.periodic_site(41., 30., 1, WipeTowerTiming::SiteKind::Tower, 0);
        trace.flushed(43., 90.);
        REQUIRE_FALSE(trace.predict(baseline, next, print));
    }
}

TEST_CASE("Tower plan verification uses the final volume of every filament change", "[WipeTowerTiming]")
{
    REQUIRE(WipeTowerTiming::same_tower_plan({25.f, 36.f}, {33.f, 36.009f}, {40.f, 20.f}));
    REQUIRE_FALSE(WipeTowerTiming::same_tower_plan({25.f, 36.f}, {33.f, 36.02f}, {40.f, 20.f}));
    REQUIRE_FALSE(WipeTowerTiming::same_tower_plan({25.f}, {40.02f}, {40.f}));
    REQUIRE_FALSE(WipeTowerTiming::same_tower_plan({0.f}, {25.f}, {40.f}));
    REQUIRE_FALSE(WipeTowerTiming::same_tower_plan({25.f}, {0.f}, {40.f}));
    REQUIRE_FALSE(WipeTowerTiming::same_tower_plan({25.f}, {25.f}, {}));
    REQUIRE(WipeTowerTiming::same_tower_plan({}, {0.f}, {40.f}));
}

TEST_CASE("Tower verification converts the actual material's volume error into seconds", "[WipeTowerTiming]")
{
    GCodeProcessorResult result;
    WipeTowerTiming trace;
    trace.change(0, 0, 1, 1000., 20., 25.f, 70.f, result);
    auto error = [&](float plan, float measured, float normal = 20.f) {
        return trace.tower_timing_error({plan}, {measured}, {normal});
    };
    REQUIRE(error(60.f, 61.125f).value() == Approx(30.));
    REQUIRE(error(61.126f, 60.f).value() > 30.);
    REQUIRE_FALSE(error(69.999f, 70.f));
    REQUIRE_FALSE(error(25.001f, 25.f));
    REQUIRE(error(70.f, 70.f).value() == 0.);
    REQUIRE(error(69.f, 70.f, 80.f).value() == 0.);
    REQUIRE_FALSE(error(0.f, 25.f, 80.f));
    REQUIRE_FALSE(trace.tower_timing_error({25.f}, {25.f}, {}));
}

TEST_CASE("No-tower idle timing compares the purge actually required above the color matrix", "[WipeTowerTiming]")
{
    // The no-tower clock starts at zero. At 60 mm3 in ten minutes, 3 mm3 is 30 seconds.
    auto error = [](float planned, float measured, float matrix) {
        return WipeTowerTiming::purge_timing_error(planned, measured, matrix, 10., 0.f, 60.f);
    };
    REQUIRE(error(0.f, 42.f, 45.f).value() == 0.);
    REQUIRE(error(0.f, 48.f, 45.f).value() == Approx(30.));
    REQUIRE(error(49.f, 44.f, 45.f).value() > WipeTowerTiming::idle_tolerance_seconds);
    REQUIRE_FALSE(error(59.99f, 60.f, 45.f));
    REQUIRE(error(0.f, 60.f, 60.f).value() == 0.);
    REQUIRE_FALSE(error(46.f, 0.f, 45.f));
    REQUIRE(error(45.f, 0.f, 45.f).value() == 0.);
}

TEST_CASE("Reading a Klipper tool-change timestamp includes pending motion without draining it", "[WipeTowerTiming]")
{
    PrintConfig config;
    config.gcode_flavor.value = gcfKlipper;
    GCodeProcessor measured, control;
    for (auto* processor : {&measured, &control}) {
        processor->apply_config(config);
        processor->initialize("tower-timing.gcode");
        processor->process_buffer("G90\nG1 X1 F1200\nG1 X2\n");
    }
    const auto mode = PrintEstimatedStatistics::ETimeMode::Normal;
    const double committed = measured.get_time(mode);
    const double boundary = measured.time_at_tower_change();
    REQUIRE(boundary > committed);
    REQUIRE(measured.time_at_tower_change() == boundary);
    REQUIRE(measured.get_time(mode) == committed);
    SECTION("A synchronization at the same boundary gives the same time") {
        control.process_buffer("M400\n");
        REQUIRE(control.get_time(mode) == Approx(boundary));
    }
    SECTION("Observing a boundary preserves subsequent motion timing") {
        for (auto* processor : {&measured, &control})
            processor->process_buffer("G1 X40\nG1 X41 Y2\nM400\n");
        REQUIRE(measured.get_time(mode) == control.get_time(mode));
    }
}

TEST_CASE("A periodic tower flush can reuse a safe node up to thirty seconds late", "[WipeTowerTiming]")
{
    using Kind = WipeTowerTiming::SiteKind;
    WipeTowerTiming previous;
    previous.start_periodic(20.);
    REQUIRE_FALSE(previous.periodic_site(319., 300., 0, Kind::ToolChange, 0));
    REQUIRE(previous.periodic_site(329., 300., 1, Kind::ToolChange, 1));
    previous.flushed(335., 300.);

    WipeTowerTiming current;
    current.start_periodic(100.);
    current.prefer_periodic_sites(&previous);

    SECTION("A node before the deadline never fires") {
        REQUIRE_FALSE(current.periodic_site(399.9, 300., 0, Kind::ToolChange, 0));
        REQUIRE(current.accepted_schedule());
        REQUIRE(current.max_deferred_lateness() < 0.);
    }
    SECTION("The upper bound is inclusive and the next period starts at script completion") {
        REQUIRE_FALSE(current.periodic_site(420., 300., 0, Kind::ToolChange, 0));
        REQUIRE_FALSE(current.accepted_schedule());
        REQUIRE(current.periodic_site(430., 300., 1, Kind::ToolChange, 1));
        current.flushed(445., 300.);
        REQUIRE(current.accepted_schedule());
        REQUIRE(current.max_deferred_lateness() == Approx(30.));
        REQUIRE_FALSE(current.periodic_site(744., 300., 2, Kind::ToolChange, 0));
        REQUIRE(current.periodic_site(745., 300., 3, Kind::ToolChange, 1));
    }
    SECTION("An actual gap larger than predicted rejects the deferred schedule") {
        REQUIRE_FALSE(current.periodic_site(420., 300., 0, Kind::ToolChange, 0));
        REQUIRE(current.periodic_site(430.01, 300., 1, Kind::ToolChange, 1));
        current.flushed(447., 300.);
        REQUIRE_FALSE(current.accepted_schedule());
    }
    SECTION("Use the first due node if the measured next node is too far away") {
        REQUIRE(current.periodic_site(420.01, 300., 0, Kind::ToolChange, 0));
        current.flushed(435., 300.);
        REQUIRE(current.accepted_schedule());
        REQUIRE(current.max_deferred_lateness() < 0.);
    }
    SECTION("Use the first available node when there was no legal node in the window") {
        REQUIRE(current.periodic_site(460., 300., 0, Kind::ToolChange, 0));
        current.flushed(475., 300.);
        REQUIRE(current.accepted_schedule());
        REQUIRE(current.max_deferred_lateness() < 0.);
    }
}
