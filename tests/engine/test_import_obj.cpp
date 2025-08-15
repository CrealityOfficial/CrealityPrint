
#include <catch2/catch.hpp>
#include "libslic3r/Model.hpp"
#include "libslic3r/Format/OBJ.hpp"

using namespace Slic3r;
TEST_CASE("Import OBJ model", "[import_obj]") {
    const char* model_path = std::getenv("MODEL_PATH");
    REQUIRE(model_path);

    Slic3r::Model model;
    std::string   message;
    Slic3r::ObjInfo obj_info;
    bool    success  = load_obj(model_path, &model, obj_info, message);
    const char* expected_str = std::getenv("EXPECTED_MODEL_VALID");
    bool expected = expected_str ? std::string(expected_str) == "true" : true;
    REQUIRE(success == expected);
}
