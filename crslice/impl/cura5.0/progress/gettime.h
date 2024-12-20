//Copyright (c) 2020 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef GETTIME_H
#define GETTIME_H

namespace cura52
{
    class TimeKeeper
    {
    private:
        double startTime;
    public:
        TimeKeeper();
        ~TimeKeeper();

        double restart();
    };

}//namespace cura52
#endif//GETTIME_H
