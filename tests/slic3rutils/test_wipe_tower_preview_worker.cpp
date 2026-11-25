#include <catch2/catch.hpp>
#include "slic3r/GUI/Jobs/BoostThreadWorker.hpp"
#include "slic3r/GUI/WipeTowerPreviewCache.hpp"

using namespace Slic3r;
using namespace Slic3r::GUI;

TEST_CASE("Tower previews reuse moves and discard superseded work", "[TowerPreview][18799]")
{
    BoostThreadWorker worker(nullptr, "tower_test");
    WipeTowerPreviewCache cache(worker);
    using Use = WipeTowerPreviewCache::Use;
    WipeTowerPreviewInput input{PrintConfig{}, 2, 0.2, 5., {1, 2}};
    input.config.prime_tower_width.value = 50.;
    input.config.filament_diameter.values = {1.75, 1.75};

    auto request = [&](Use use = Use::Scene, ObjectID plate = ObjectID(1)) {
        return cache.request(0, plate, use, input);
    };
    // Publication happens only when the calling thread processes worker events.
    REQUIRE_FALSE(request().ready);
    REQUIRE_FALSE(request(Use::Validation).ready);
    REQUIRE(worker.wait_for_idle(5000));
    auto first = request();
    REQUIRE(first.ready);
    REQUIRE_FALSE(first.preview->tower_mesh.empty());
    REQUIRE(request(Use::Validation).preview == first.preview);
    REQUIRE(cache.take_changed());
    REQUIRE_FALSE(cache.take_changed());

    input.config.wipe_tower_x.values = {180.};
    input.config.wipe_tower_y.values = {90.};
    input.config.wipe_tower_rotation_angle.value = 43.;
    REQUIRE(request(Use::Placement).ready);
    REQUIRE(request().preview == first.preview);
    REQUIRE(worker.is_idle());

    input.config.prime_tower_width.value = 65.;
    auto pending = request();
    REQUIRE_FALSE(pending.ready);
    REQUIRE(pending.preview == first.preview);
    input.config.prime_tower_width.value = 75.;
    REQUIRE_FALSE(request().ready);
    REQUIRE(worker.wait_for_idle(5000));
    auto newest = request();
    REQUIRE(newest.ready);
    REQUIRE(newest.preview != first.preview);
    REQUIRE(newest.preview->size == input.estimate().size);

    auto replacement_plate = request(Use::Scene, ObjectID(2));
    REQUIRE_FALSE(replacement_plate.ready);
    REQUIRE_FALSE(replacement_plate.preview);
    cache.clear();
    REQUIRE(worker.wait_for_idle(5000));
    REQUIRE_FALSE(cache.take_changed());
    input.height = std::numeric_limits<double>::quiet_NaN();
    REQUIRE(request().ready);
    REQUIRE(request().preview->tower_mesh.empty());
    REQUIRE(worker.is_idle());
}


TEST_CASE("An in-bounds loaded tower leaves room for timed purges", "[TowerPreview][TowerPlacement]")
{
    WipeTowerPreviewInput input{PrintConfig{}, 2, 0.2, 20., {1, 2}};
    input.config.filament_diameter.values = {1.75, 1.75};
    input.config.prime_tower_width.value = 50.;
    input.config.filament_minimal_purge_on_wipe_tower.values = {20., 30.};
    input.config.filament_pre_flush_tower_time.values = {0., 20.};
    input.config.filament_pre_flush_tower_volume.values = {0., 600.};

    auto job = std::make_shared<LoadedTowerPlacement>();
    job->preview = std::make_shared<WipeTowerPreview>(input.estimate());
    const auto nominal = job->preview->footprint();
    WipeTowerPreviewInput reserved = input;
    REQUIRE(reserve_idle_purge_volume(reserved));
    const auto expanded = reserved.estimate().footprint();
    const auto top = scaled(300.);
    const auto saved = WipeTowerPose{Point(scaled(100.), top - get_extents(nominal).max.y()), 0.};
    job->bed = Slic3r::Polygon({Point(0, 0), Point(top, 0), Point(top, top), Point(0, top)});
    auto at = [](Slic3r::Polygon shape, Point anchor) { shape.translate(anchor); return shape; };
    REQUIRE(diff(at(nominal, saved.anchor), job->bed).empty());
    REQUIRE_FALSE(diff(at(expanded, saved.anchor), job->bed).empty());

    BoostThreadWorker worker(nullptr, "tower_test");
    LoadedTowerPlacement::schedule(worker, job, input, saved);
    REQUIRE(worker.wait_for_idle(5000));
    REQUIRE(job->ready);
    REQUIRE(job->pose);
    REQUIRE(job->pose->rotation_degrees == saved.rotation_degrees);
    REQUIRE(diff(at(expanded, job->pose->anchor), job->bed).empty());
}
