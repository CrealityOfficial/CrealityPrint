#include <catch2/catch.hpp>

#include "libslic3r/CrealityVersion.hpp"

using namespace Slic3r;

TEST_CASE("Creality version keeps build id separate from semantic version", "[CrealityVersion]")
{
    const ParsedCrealityVersion version = parse_creality_version("7.2.0.5683");

    REQUIRE(version.valid());
    REQUIRE(version.semver.to_string_sf() == "7.2.0");
    REQUIRE(version.build_id == 5683);
}

TEST_CASE("Creality version uses the fallback build id for three-part versions", "[CrealityVersion]")
{
    const ParsedCrealityVersion version = parse_creality_version("7.2.1", 5629);

    REQUIRE(version.valid());
    REQUIRE(version.semver.to_string_sf() == "7.2.1");
    REQUIRE(version.build_id == 5629);
}
