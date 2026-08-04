#include <catch2/catch.hpp>

#include "libslic3r/PresetConfigCache.hpp"
#include "libslic3r/PrintConfig.hpp"

#include <boost/filesystem.hpp>
#include <boost/nowide/fstream.hpp>

namespace {

class TemporaryDirectory
{
public:
    TemporaryDirectory()
        : path(boost::filesystem::temp_directory_path() /
               boost::filesystem::unique_path("creality-preset-cache-test-%%%%-%%%%"))
    {
        boost::filesystem::create_directories(path);
    }

    ~TemporaryDirectory()
    {
        boost::system::error_code ignored;
        boost::filesystem::remove_all(path, ignored);
    }

    boost::filesystem::path path;
};

void write_source_file(const boost::filesystem::path& path, const std::string& contents)
{
    boost::filesystem::create_directories(path.parent_path());
    boost::nowide::ofstream stream(path.string(), std::ios::binary | std::ios::trunc);
    stream << contents;
}

} // namespace

TEST_CASE("System preset parsed config cache persists and invalidates changed manifests", "[PresetConfigCache]")
{
    TemporaryDirectory temporary;
    const boost::filesystem::path source_root = temporary.path / "system" / "Creality";
    const boost::filesystem::path cache_root = temporary.path / "cache";
    const std::string relative_path = "process/test.json";
    const boost::filesystem::path vendor_manifest = source_root.parent_path() / "Creality.json";
    write_source_file(vendor_manifest, "{\"version\":\"1.0.0\"}");
    write_source_file(source_root / relative_path, "{\"name\":\"test\"}");

    Slic3r::DynamicPrintConfig original;
    original.set("layer_height", 0.2, true);
    const std::map<std::string, std::string> metadata{{"name", "test"}, {"inherits", "base"}};
    const std::vector<std::string> unrecognized{"future_key"};

    {
        Slic3r::PresetConfigCache cache(source_root, "Creality",
            Slic3r::ForwardCompatibilitySubstitutionRule::EnableSilent, true, cache_root);
        cache.store(relative_path, original, metadata, unrecognized);
        cache.save();
        REQUIRE(cache.hits() == 0);
    }

    {
        Slic3r::PresetConfigCache cache(source_root, "Creality",
            Slic3r::ForwardCompatibilitySubstitutionRule::EnableSilent, true, cache_root);
        Slic3r::DynamicPrintConfig loaded;
        std::map<std::string, std::string> loaded_metadata;
        std::vector<std::string> loaded_unrecognized;
        REQUIRE(cache.load(relative_path, loaded, loaded_metadata, loaded_unrecognized));
        REQUIRE(loaded == original);
        REQUIRE(loaded_metadata == metadata);
        REQUIRE(loaded_unrecognized == unrecognized);
        REQUIRE(cache.hits() == 1);
    }

    write_source_file(vendor_manifest, "{\"version\":\"2.0.0 changed\"}");
    {
        Slic3r::PresetConfigCache cache(source_root, "Creality",
            Slic3r::ForwardCompatibilitySubstitutionRule::EnableSilent, true, cache_root);
        Slic3r::DynamicPrintConfig loaded;
        std::map<std::string, std::string> loaded_metadata;
        std::vector<std::string> loaded_unrecognized;
        REQUIRE_FALSE(cache.load(relative_path, loaded, loaded_metadata, loaded_unrecognized));
        REQUIRE(cache.misses() == 1);
    }
}
