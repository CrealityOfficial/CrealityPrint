#include "nestplacer/concaveplacer.h"
#include "data.h"
#include "conv.h"
#include "debug.h"

namespace nestplacer
{
    void convert(const ConcaveItem& _items, Clipper3r::Path& path)
    {
        size_t size = _items.size();
        if (size > 0)
        {
            path.resize(size);
            for (size_t i = 0; i < size; ++i)
                path.at(i) = convert(_items.at(i));
        }
    }

    void initConfig(NfpFisrtFitConfig& config, const NestConcaveParam& param)
    {
        config.placer_config.starting_point = libnest2d::NfpPlacer::Config::Alignment::CENTER;
        config.placer_config.alignment = libnest2d::NfpPlacer::Config::Alignment::CENTER;
        config.placer_config.setNewAlignment(1);
        int step = (int)(360.0f / param.rotationAngle);
        config.placer_config.rotations.clear();
        for (int i = 0; i < step; i++)
            config.placer_config.rotations.push_back(
                libnest2d::Radians(libnest2d::Degrees((double)i * param.rotationAngle)));

        config.placer_config.root = param.rootDir;
    }

    void initControl(libnest2d::NestControl& control, size_t size, ccglobal::Tracer* tracer)
    {
        control.progressfn = [&size, tracer](int remain) {
            if (tracer)
            {
                tracer->progress((float)((int)size - remain) / (float)size);
            }
        };
        control.stopcond = [tracer]()->bool {
            if (tracer)
                return tracer->interrupt();
            return false;
        };
    }

	void layout_all_nest(const ConcaveItems& models, const NestConcaveParam& param, ConcaveNestDebugger* debugger)
	{
        size_t size = models.size();
        if (size == 0)
            return;

        libnest2d::NestControl ctl;
        initControl(ctl, size, param.tracer);

        NfpFisrtFitConfig config;
        initConfig(config, param);
        if (debugger) {
            config.placer_config.parallel = false;
            initDebugger(config, debugger);
        }

        Clipper3r::cInt distance = MM2INT(param.distance);

        std::vector<libnest2d::Item> inputs;
        for (int i = 0; i < size; i++)
        {
            Clipper3r::Path path;
            convert(models.at(i), path);
            if (Clipper3r::Orientation(path))
                Clipper3r::ReversePath(path);
            inputs.emplace_back(libnest2d::Item(std::move(path)));
            inputs.back().convexCal(false);
        }

        libnest2d::Box binBox = convert(param.box);

        std::size_t result = libnest2d::nest(inputs, binBox, distance, config, ctl);

        for (size_t i = 0; i < size; ++i)
        {
            const libnest2d::Item& item = inputs.at(i);
            NestRT rt;
            rt.x = INT2MM(item.translation().X);
            rt.y = INT2MM(item.translation().Y);
            rt.z = item.rotation().toDegrees();
            param.callback((int)i, rt);
        }
	}

    int test_nest(ConcaveItems& models, NestConcaveParam& param, ConcaveNestDebugger* debugger)
    {
        // Example polygons
        std::vector<libnest2d::Item> input1(102, {
                {-5000000, 8954050},
                {5000000, 8954050},
                {5000000, -45949},
                {4972609, -568550},
                {3500000, -8954050},
                {-3500000, -8954050},
                {-4972609, -568550},
                {-5000000, -45949},
                {-5000000, 8954050},
            });
        
        std::vector<libnest2d::Item> input2(25, {
               {-11750000, 13057900},
               {-9807860, 15000000},
               {4392139, 24000000},
               {11750000, 24000000},
               {11750000, -24000000},
               {4392139, -24000000},
               {-9807860, -15000000},
               {-11750000, -13057900},
               {-11750000, 13057900},
            });

        std::vector<libnest2d::Item> input3(2, {
               {-13750000 * 5, 11057900 * 5},
               {-9807860 * 5, 15000000 * 5},
               {4392139 * 6, 18000000 * 5},
               {13750000 * 6, 18000000 * 5},
               {18921390 * 6, 200000 * 5},
               {13750000 * 6, -18000000 * 5},
               {4392139 * 6, -18000000 * 5},
               {-9807860 * 6, -12000000 * 6},
               {-13750000 * 5, -11057900 * 5},
               {-13750000 * 5, 11057900 * 5},
            });
        std::vector<libnest2d::Item> input4(4, {
               {-13750000 * 3, 11057900 * 5},
               {-9807860 * 3, 15000000 * 5},
               {4392139 * 5, 20000000 * 5},
               {13750000 * 5, 20000000 * 5},
               {13750000 * 5, -20000000 * 5},
               {4392139 * 5, -20000000 * 5},
               {-9807860 * 5, -13000000 * 6},
               {-13750000 * 2, -11057900 * 5},
               {-13750000 * 2, 1157900 * 5},
            });
        std::vector<libnest2d::Item> inputs;
        inputs.insert(inputs.end(), input1.begin(), input1.end());
        inputs.insert(inputs.end(), input2.begin(), input2.end());
        inputs.insert(inputs.end(), input3.begin(), input3.end());
        inputs.insert(inputs.end(), input4.begin(), input4.end());
        const size_t size = inputs.size();
        libnest2d::NestControl ctl;
        initControl(ctl, size, param.tracer);

        NfpFisrtFitConfig config;
        initConfig(config, param);
        if (debugger) {
            config.placer_config.parallel = false;
            initDebugger(config, debugger);
        }

        Clipper3r::cInt distance = MM2INT(param.distance);
        int boxWidth = 150000000;
        libnest2d::Box box({ 0,0 }, { 150000000, boxWidth });

        // Perform the nesting with a box shaped bin
        size_t bins = nest(inputs, box, distance, config, ctl);

        param.box.min = trimesh::vec3(box.minCorner().X / 1E6, box.minCorner().Y / 1E6, 0);
        param.box.max = trimesh::vec3(box.maxCorner().X / 1E6, box.maxCorner().Y / 1E6, 0);
        // Retrieve resulting geometries
        for (libnest2d::Item& r : inputs) {
            ConcaveItem poly;
            auto polygon = r.transformedShape();
            for (const auto& p : polygon.Contour) {
                poly.emplace_back(trimesh::vec3(p.X / 1E6, p.Y / 1E6, 0));
            }
            for (const auto& path : polygon.Holes) {
                for (const auto& p : path) {
                    poly.emplace_back(trimesh::vec3(p.X / 1E6, p.Y / 1E6, 0));
                }
            }
            models.emplace_back(poly);
        }
        return bins;
    }

    class NestPlacerImpl
    {
    public:
        NestPlacerImpl()
            : size(0)
        {

        }

        ~NestPlacerImpl()
        {

        }

        void setInputs(const ConcaveItems& models) 
        {
            inputs.clear();
            size = (int)models.size();

            for (int i = 0; i < size; i++)
            {
                Clipper3r::Path path;
                convert(models.at(i), path);

                if (Clipper3r::Orientation(path))
                    Clipper3r::ReversePath(path);

                inputs.emplace_back(libnest2d::Item(std::move(path)));
                inputs.back().convexCal(false);
            }
        }

        bool valid() 
        {
            return size > 0;
        }

        void invokeCallback(NestCallback callback){

            if (callback)
            {
                for (size_t i = 0; i < size; ++i)
                {
                    const libnest2d::Item& item = inputs.at(i);
                    NestRT rt;
                    rt.x = INT2MM(item.translation().X);
                    rt.y = INT2MM(item.translation().Y);
                    rt.z = item.rotation().toDegrees();
                    callback((int)i, rt);
                }
            }
        }

        std::vector<libnest2d::Item> inputs;
        int size;
    };

    NestPlacer::NestPlacer()
        : impl(new NestPlacerImpl())
    {
    }

    NestPlacer::~NestPlacer()
    {

    }

    void NestPlacer::setInput(const ConcaveItems& models)
    {
        impl->setInputs(models);
    }

    void NestPlacer::layout(const NestConcaveParam& param, ConcaveNestDebugger* _debugger)
    {
        libnest2d::NestControl ctl;
        initControl(ctl, impl->size, param.tracer);

        NfpFisrtFitConfig config;
        initConfig(config, param);
        if (_debugger) {
            config.placer_config.parallel = false;
            initDebugger(config, _debugger);
        }

        Clipper3r::cInt distance = MM2INT(param.distance);
        libnest2d::Box binBox = convert(param.box);

        std::size_t result = libnest2d::nest(impl->inputs, binBox, distance, config, ctl);

        impl->invokeCallback(param.callback);
    }

    void NestPlacer::testNFP(trimesh::vec3& point, DebugPolygon& poly, std::vector<trimesh::vec3>& lines)
    {
        using namespace libnest2d;
        if (impl->inputs.size() != 2)
            return;

        const Item& fixedItem = impl->inputs.at(0);
        const Item& orbItem = impl->inputs.at(1);

        auto& fixedp = fixedItem.transformedShape();
        auto& orbp = orbItem.transformedShape();

        struct NfpDebugger {
            void onEdges(const std::vector<_Segment<PointImpl>>& edges) {
                for (const _Segment<PointImpl>& segment : edges)
                {
                    lines.push_back(convert(segment.first()));
                    lines.push_back(convert(segment.second()));
                }
            }

            std::vector<trimesh::vec3> lines;
        } nfpDebugger;

        auto subnfp_r = nfp::nfpConvexOnly<PolygonImpl, double, NfpDebugger>(fixedp, orbp, &nfpDebugger);
        placers::correctNfpPosition(subnfp_r, fixedItem, orbItem);

        point = convert(subnfp_r.second);
        convertPolygon(subnfp_r.first, poly);
        lines = nfpDebugger.lines;
    }
}
