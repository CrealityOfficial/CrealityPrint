
#include <catch2/catch.hpp>
#include "libslic3r/Model.hpp"
using namespace Slic3r;
TEST_CASE("Import STL model", "[import_stl]") {
    const char* model_path = std::getenv("MODEL_PATH");
    REQUIRE(model_path);

    Slic3r::Model model;
    
    bool        success      = Slic3r::load_stl(model_path, &model);
    const char* expected_str = std::getenv("EXPECTED_MODEL_VALID");
    bool expected = expected_str ? std::string(expected_str) == "true" : true;

    REQUIRE(success == expected);
}
