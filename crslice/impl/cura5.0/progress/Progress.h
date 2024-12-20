//Copyright (c) 2018 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef PROGRESS_H
#define PROGRESS_H
#include <string>
#include "ccglobal/tracer.h"
#include "progress/gettime.h"

namespace cura52
{
#define N_PROGRESS_STAGES 7

    /*!
     * Class for handling the progress bar and the progress logging.
     *
     * The progress bar is based on a single slicing of a rather large model which needs some complex support;
     * the relative timing of each stage is currently based on that of the slicing of dragon_65_tilted_large.stl
     */
    class Progress
    {
    public:
        /*!
         * The stage in the whole slicing process
         */
        enum class Stage : unsigned int
        {
            START = 0,
            SLICING = 1,
            PARTS = 2,
            INSET_SKIN = 3,
            SUPPORT = 4,
            EXPORT = 5,
            FINISH = 6
        };
    private:
        static double times[N_PROGRESS_STAGES]; //!< Time estimates per stage
        static std::string names[N_PROGRESS_STAGES]; //!< name of each stage
        double accumulated_times[N_PROGRESS_STAGES]; //!< Time past before each stage
        double total_timing; //!< An estimate of the total time

        TimeKeeper time_keeper; // TODO: use singleton time keeper
        /*!
         * Give an estimate between 0 and 1 of how far the process is.
         *
         * \param stage The current stage of processing
         * \param stage_process How far we currently are in the \p stage
         * \return An estimate of the overall progress.
         */
        float calcOverallProgress(Stage stage, float stage_progress);
    public:
        ccglobal::Tracer* tracer = nullptr;

        Progress();
        ~Progress();

        void init(); //!< Initialize some values needed in a fast computation of the progress

        void restartTime();
        /*!
         * Message progress over the CommandSocket and to the terminal (if the command line arg '-p' is provided).
         *
         * \param stage The current stage of processing
         * \param progress_in_stage Any number giving the progress within the stage
         * \param progress_in_stage_max The maximal value of \p progress_in_stage
         */
        void messageProgress(Stage stage, int progress_in_stage, int progress_in_stage_max);
        /*!
         * Message the progress stage over the command socket.
         *
         * \param stage The current stage
         * \param timeKeeper The stapwatch keeping track of the timings for each stage (optional)
         */
        void messageProgressStage(Stage stage);
    };


} // name space cura
#endif//PROGRESS_H
