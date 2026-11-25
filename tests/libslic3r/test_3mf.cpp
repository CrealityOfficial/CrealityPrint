#include <catch2/catch.hpp>

#include "libslic3r/Model.hpp"
#include "libslic3r/ModelInstance.hpp"
#include "libslic3r/ModelVolume.hpp"
#include "libslic3r/ModelObject.hpp"
#include "libslic3r/Format/3mf.hpp"
#include "libslic3r/Format/STL.hpp"

#include <boost/filesystem/operations.hpp>

using namespace Slic3r;

TEST_CASE("Split objects retain the chosen Auto-Drop behavior", "[split_auto_drop]")
{
    Model model;
    ModelObject* object = model.add_object();
    object->add_instance();
    object->add_volume(make_cube(4, 4, 4));
    TriangleMesh upper = make_cube(4, 4, 4);
    upper.translate(0.0f, 0.0f, 12.0f);
    object->add_volume(std::move(upper));

    ModelObjectPtrs parts;
    object->split(&parts);
    REQUIRE(parts.size() == 2);
    REQUIRE(parts[0]->get_instance_min_z(0) == Approx(0.0));
    REQUIRE(parts[1]->get_instance_min_z(0) == Approx(12.0));

    Model imported;
    SECTION("Automatic placement drops the upper part")
    {
        ModelObject* loaded = imported.add_object(*parts[1]);
        REQUIRE(loaded->instances[0]->auto_drop);
        loaded->ensure_on_bed();
        REQUIRE(loaded->get_instance_min_z(0) == Approx(0.0));
    }
    SECTION("Placement preserves the floating instance and drops an enabled copy")
    {
        parts[1]->instances[0]->auto_drop = false;
        ModelObject* loaded = imported.add_object(*parts[1]);
        ModelInstance* copy = loaded->add_instance(*loaded->instances[0]);
        REQUIRE_FALSE(copy->auto_drop);
        copy->auto_drop = true;
        loaded->ensure_on_bed();
        REQUIRE(loaded->get_instance_min_z(0) == Approx(12.0));
        REQUIRE(loaded->get_instance_min_z(1) == Approx(0.0));

        const auto file = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("split-%%%%-%%%%.3mf");
        std::string path = file.string();
        Model restored;
        DynamicPrintConfig config;
        ConfigSubstitutionContext substitutions{ForwardCompatibilitySubstitutionRule::Disable};
        SECTION("Creality project round trip")
        {
            StoreParams params;
            params.path = path.c_str();
            params.model = &imported;
            DynamicPrintConfig project_config = DynamicPrintConfig::full_print_config();
            params.config = &project_config;
            params.strategy = SaveStrategy::Zip64 | SaveStrategy::Silence | SaveStrategy::SkipAuxiliary;
            REQUIRE(store_bbs_3mf(params));
            PlateDataPtrs plates;
            std::vector<Preset*> presets;
            Semver version;
            bool is_bbl = false;
            bool ok = load_bbs_3mf(path.c_str(), &config, &substitutions, &restored, &plates, &presets,
                                   &is_bbl, &version, nullptr, LoadStrategy::LoadModel);
            release_PlateData_list(plates);
            boost::filesystem::remove(file);
            REQUIRE(ok);
        }
        SECTION("Generic 3MF round trip")
        {
            REQUIRE(store_3mf(path.c_str(), &imported, nullptr, false));
            bool ok = load_3mf(path.c_str(), config, substitutions, &restored, false);
            boost::filesystem::remove(file);
            REQUIRE(ok);
        }
        size_t floating = 0;
        size_t grounded = 0;
        for (ModelObject* result : restored.objects) {
            result->ensure_on_bed(true);
            for (size_t i = 0; i < result->instances.size(); ++i) {
                if (result->instances[i]->auto_drop) {
                    CHECK(result->get_instance_min_z(i) == Approx(0.0));
                    ++grounded;
                } else {
                    CHECK(result->get_instance_min_z(i) == Approx(12.0));
                    ++floating;
                }
            }
        }
        CHECK(floating == 1);
        CHECK(grounded == 1);
    }
}

SCENARIO("Reading 3mf file", "[3mf]") {
    GIVEN("umlauts in the path of the file") {
        Model model;
        WHEN("3mf model is read") {
        	std::string path = std::string(TEST_DATA_DIR) + "/test_3mf/Geräte/Büchse.3mf";
        	DynamicPrintConfig config;
            ConfigSubstitutionContext ctxt{ ForwardCompatibilitySubstitutionRule::Disable };
            bool ret = load_3mf(path.c_str(), config, ctxt, &model, false);
            THEN("load should succeed") {
                REQUIRE(ret);
            }
        }
    }
}

SCENARIO("Export+Import geometry to/from 3mf file cycle", "[3mf]") {
    GIVEN("world vertices coordinates before save") {
        // load a model from stl file
        Model src_model;
        std::string src_file = std::string(TEST_DATA_DIR) + "/test_3mf/Prusa.stl";
        load_stl(src_file.c_str(), &src_model);
        src_model.add_default_instances();

        ModelObject* src_object = src_model.objects.front();

        // apply generic transformation to the 1st volume
        Geometry::Transformation src_volume_transform;
        src_volume_transform.set_offset({ 10.0, 20.0, 0.0 });
        src_volume_transform.set_rotation({ Geometry::deg2rad(25.0), Geometry::deg2rad(35.0), Geometry::deg2rad(45.0) });
        src_volume_transform.set_scaling_factor({ 1.1, 1.2, 1.3 });
        src_volume_transform.set_mirror({ -1.0, 1.0, -1.0 });
        src_object->volumes.front()->set_transformation(src_volume_transform);

        // apply generic transformation to the 1st instance
        Geometry::Transformation src_instance_transform;
        src_instance_transform.set_offset({ 5.0, 10.0, 0.0 });
        src_instance_transform.set_rotation({ Geometry::deg2rad(12.0), Geometry::deg2rad(13.0), Geometry::deg2rad(14.0) });
        src_instance_transform.set_scaling_factor({ 0.9, 0.8, 0.7 });
        src_instance_transform.set_mirror({ 1.0, -1.0, -1.0 });
        src_object->instances.front()->set_transformation(src_instance_transform);

        WHEN("model is saved+loaded to/from 3mf file") {
            // save the model to 3mf file
            std::string test_file = std::string(TEST_DATA_DIR) + "/test_3mf/prusa.3mf";
            store_3mf(test_file.c_str(), &src_model, nullptr, false);

            // load back the model from the 3mf file
            Model dst_model;
            DynamicPrintConfig dst_config;
            {
                ConfigSubstitutionContext ctxt{ ForwardCompatibilitySubstitutionRule::Disable };
                load_3mf(test_file.c_str(), dst_config, ctxt, &dst_model, false);
            }
            boost::filesystem::remove(test_file);

            // compare meshes
            TriangleMesh src_mesh = src_model.mesh();
            TriangleMesh dst_mesh = dst_model.mesh();

            bool res = src_mesh.its.vertices.size() == dst_mesh.its.vertices.size();
            if (res) {
                for (size_t i = 0; i < dst_mesh.its.vertices.size(); ++i) {
                    res &= dst_mesh.its.vertices[i].isApprox(src_mesh.its.vertices[i]);
                }
            }
            THEN("world vertices coordinates after load match") {
                REQUIRE(res);
            }
        }
    }
}

SCENARIO("2D convex hull of sinking object", "[3mf]") {
    GIVEN("model") {
        // load a model
        Model model;
        std::string src_file = std::string(TEST_DATA_DIR) + "/test_3mf/Prusa.stl";
        load_stl(src_file.c_str(), &model);
        model.add_default_instances();

        WHEN("model is rotated, scaled and set as sinking") {
            ModelObject* object = model.objects.front();
            object->center_around_origin(false);

            // set instance's attitude so that it is rotated, scaled and sinking
            ModelInstance* instance = object->instances.front();
            instance->set_rotation(X, -M_PI / 4.0);
            instance->set_offset(Vec3d::Zero());
            instance->set_scaling_factor({ 2.0, 2.0, 2.0 });

            // calculate 2D convex hull
            Polygon hull_2d = object->convex_hull_2d(instance->get_transformation().get_matrix());

            // verify result
            Points result = {
                { -91501496, -15914144 },
                { 91501496, -15914144 },
                { 91501496, 4243 },
                { 78229680, 4246883 },
                { 56898100, 4246883 },
                { -85501496, 4242641 },
                { -91501496, 4243 }
            };

            // Allow 1um error due to floating point rounding.
            bool res = hull_2d.points.size() == result.size();
            if (res)
                for (size_t i = 0; i < result.size(); ++ i) {
                    const Point &p1 = result[i];
                    const Point &p2 = hull_2d.points[i];
                    if (std::abs(p1.x() - p2.x()) > 1 || std::abs(p1.y() - p2.y()) > 1) {
                        res = false;
                        break;
                    }
                }

            THEN("2D convex hull should match with reference") {
                REQUIRE(res);
            }
        }
    }
}

