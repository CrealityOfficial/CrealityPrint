//Copyright (c) 2018 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef APPLICATION_H
#define APPLICATION_H

#include <cstddef> //For size_t.
#include <cassert>
#include <string>
#include <vector>
#include <memory>

#include "communication/slicecontext.h"

#include "FffGcodeWriter.h"
#include "FffPolygonGenerator.h"
#include "Scene.h"
#include "crslice/header.h"

namespace cura52
{
    class SceneFactory;
    /*!
     * A singleton class that serves as the starting point for all slicing.
     *
     * The application provides a starting point for the slicing engine. It
     * maintains communication with other applications and uses that to schedule
     * slices.
     */
    class Application : public SliceContext
    {
    public:
        /*!
         * \brief Constructs a new Application instance.
         *
         * You cannot call this because this goes via the getInstance() function.
         */
        Application(ccglobal::Tracer* tracer = nullptr);

        /*!
         * \brief Destroys the Application instance.
         *
         * This destroys the Communication instance along with it.
         */
        virtual ~Application();

        SliceResult runSceneFactory(SceneFactory* factory);
    protected:
        int extruderCount() override;
        const std::vector<ExtruderTrain>& extruders() const override;
        std::vector<ExtruderTrain>& extruders() override;

        const Settings& extruderSettings(int index) override;
        const Settings& currentGroupSettings() override;
        const Settings& sceneSettings() override;
        bool isCenterZero() override;
        std::string polygonFile() override;

        std::string supportFile() override;
        std::string antiSupportFile()override;
        std::string seamFile()override;
        std::string antiSeamFile()override;

        MeshGroup* currentGroup() override;
        int groupCount() override;
        bool isFirstGroup() override;
        FPoint3 groupOffset() override;

        ThreadPool* pool() override;
        bool checkInterrupt(const std::string& message = "") override;
        void setFailed() override;
        void tick(const std::string& tag) override;
        void message(const char* msg) override;
        ccglobal::Tracer* getTracer() override;
        gcode::GcodeTracer* debugger() override;
        Cache* cache() override;

        void messageProgress(Progress::Stage stage, int progress_in_stage, int progress_in_stage_max) override;
        void messageProgressStage(Progress::Stage stage) override;

        void setResult(const SliceResult& result) override;

        void compute();

        /*!
         * \brief Start the global thread pool.
         *
         * If `nworkers` <= 0 and there is no pre-existing thread pool, a thread
         * pool with hardware_concurrency() workers is initialized.
         * The thread pool is restarted when the number of thread differs from
         * previous invocations.
         *
         * \param nworkers The number of workers (including the main thread) that are ran.
         */
        void startThreadPool(int nworkers = 0);

        /*!
        * init cache
        */
        void initCache();

        void wrapperSceneSettings();
        void wrapperOtherSettings();
    private:
        bool m_error;

        /*!
         * \brief ThreadPool with lifetime tied to Application
         */
        std::unique_ptr<ThreadPool> thread_pool;

        SliceResult m_result;

        /*!
          * The gcode writer, which generates paths in layer plans in a buffer, which converts these paths into gcode commands.
          */
        FffGcodeWriter gcode_writer;

        /*!
         * The polygon generator, which slices the models and generates all polygons to be printed and areas to be filled.
         */
        FffPolygonGenerator polygon_generator;

        Progress progressor;

        ccglobal::Tracer* tracer = nullptr;
        /*
         * \brief The slice that is currently ongoing.
         *
         * If no slice has started yet, this will be a nullptr.
         */
        std::shared_ptr<Scene> scene;

        std::unique_ptr<Cache> m_cache;
    };

} //Cura namespace.

#endif //APPLICATION_H