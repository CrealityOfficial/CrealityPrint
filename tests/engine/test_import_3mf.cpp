#include <catch2/catch.hpp>
// 假设已有的 Model 类用于加载 3MF 文件
#include "libslic3r/Model.hpp"
#include "libslic3r/Format/3mf.hpp"
#include "libslic3r/ModelObject.hpp"
#include "libslic3r/ModelInstance.hpp"
#include "slic3r/GUI/MeshUtils.hpp"
#include "libslic3r/ModelVolume.hpp"
#include <boost/filesystem/operations.hpp>

using namespace Slic3r;
TEST_CASE("Import 3MF model", "[import_3mf]") {
    const char* model_path = std::getenv("MODEL_PATH");
    REQUIRE(model_path != nullptr);

    Slic3r::Model model;
    DynamicPrintConfig        config;
    ConfigSubstitutionContext ctxt{ForwardCompatibilitySubstitutionRule::Disable};
    bool  success      = load_3mf(model_path, config, ctxt, &model, false);
    const char* expected_str = std::getenv("EXPECTED_MODEL_VALID");
    bool expected = expected_str ? std::string(expected_str) == "true" : true;
    REQUIRE(success == expected);
}
