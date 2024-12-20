//  Copyright (c) 2018-2022 Ultimaker B.V.
//  CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef SLICE_CONTEXT_H
#define SLICE_CONTEXT_H

#include "MeshGroup.h"
#include "ExtruderTrain.h"

#include "settings/wrapper.h"
#include "settings/gwrapper.h"
#include "settings/ewrapper.h"

#include "progress/Progress.h"
#include "types/header.h"
#include "tools/Cache.h"

#include <stdarg.h>
#include "ccglobal/log.h"

namespace gcode
{
    class GcodeTracer;
}

namespace cura52
{
    struct SliceResult
    {
        unsigned long int print_time; // 预估打印耗时，单位：秒
        double filament_len; // 预估材料消耗，单位：米
        double filament_volume; // 预估材料重量，单位：g
        unsigned long int layer_count;  // 切片层数
        double x;   // 切片x尺寸
        double y;   // 切片y尺寸
        double z;   // 切片z尺寸

        SliceResult()
        {
            print_time = 0;
            filament_len = 0.0f;
            filament_volume = 0.0f;
            layer_count = 0;
            x = 0.0f;
            y = 0.0f;
            z = 0.0f;
        }
    };

    /*
     * An abstract class to provide a common interface for all methods of
     * communicating instructions from and to CuraEngine.
     */
    class ThreadPool;
    class SliceContext : public SceneParamWrapper
        , public GroupParamWrapper
        , public ExtruderParamWrapper
    {
    public:
        /*
         * \brief Close the communication channel.
         */
        virtual ~SliceContext() {}

        virtual int extruderCount() = 0;
        virtual const std::vector<ExtruderTrain>& extruders() const = 0;
        virtual std::vector<ExtruderTrain>& extruders() = 0;

        virtual const Settings& extruderSettings(int index) = 0;
        virtual const Settings& currentGroupSettings() = 0;
        virtual const Settings& sceneSettings() = 0;

        virtual bool isCenterZero() = 0;
        virtual std::string polygonFile() = 0;

        virtual std::string supportFile() = 0;
        virtual std::string antiSupportFile() = 0;
        virtual std::string seamFile() = 0;
        virtual std::string antiSeamFile() = 0;
        
        virtual MeshGroup* currentGroup() = 0;
        virtual int groupCount() = 0;
        virtual bool isFirstGroup() = 0;
        virtual FPoint3 groupOffset() = 0;

        virtual ThreadPool* pool() = 0;
        virtual bool checkInterrupt(const std::string& msg) = 0;
        virtual void setFailed() = 0;
        virtual void tick(const std::string& tag) = 0;
        virtual void message(const char* msg) = 0;
        virtual ccglobal::Tracer* getTracer() = 0;
        virtual gcode::GcodeTracer* debugger() = 0;
        virtual Cache* cache() = 0;

        void formatMessage(const char* format, ...)
        {
            char buf[1024] = { 0 };
            va_list args;
            va_start(args, format);
            vsprintf(buf, format, args);
            message(buf);
            va_end(args);
        }

        virtual void messageProgress(Progress::Stage stage, int progress_in_stage, int progress_in_stage_max) = 0;
        virtual void messageProgressStage(Progress::Stage stage) = 0;

        virtual void setResult(const SliceResult& result) = 0;
    };

} //namespace cura52

#include "tools/ThreadPool.h"

#if 1   // USE_INTERRUPT
#define INTERRUPT_RETURN(x) 	if (application->checkInterrupt(x)) return
#define INTERRUPT_RETURN_FALSE(x) 	if (application->checkInterrupt(x)) return false
#define INTERRUPT_BREAK(x) 	if (application->checkInterrupt(x)) break
#else
#define INTERRUPT_RETURN(x) 	(void)0
#define INTERRUPT_RETURN_FALSE(x)  (void)0
#define INTERRUPT_BREAK(x) (void)0
#endif

#if 1
#define CALLTICK(x) application->tick(x) 
#else
#define CALLTICK(x) (void)0
#endif

#define SAFE_MESSAGE(msg, ...) \
	if(application->getTracer()) application->getTracer()->variadicFormatMessage(msg, __VA_ARGS__)

#endif //SLICE_CONTEXT_H

