//Copyright (c) 2022 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#include "Date.h"

#include <sstream>
#include <cstdio> // sscanf
#include <cstring> // strstr
#include <iomanip> // setw, setfill
#include <chrono>

namespace cura52
{

    Date::Date(int year, int month, int day)
        : year(year)
        , month(month)
        , day(day)
    {
    }

    std::string Date::toStringDashed()
    {
        std::ostringstream str;
        str << std::setfill('0') << std::setw(4) << year << "-"
            << std::setfill('0') << std::setw(2) << month << "-"
            << std::setfill('0') << std::setw(2) << day;
        return str.str();
    }

    Date::Date()
        : year(-1)
        , month(-1)
        , day(-1)
    {
    }

    Date Date::getDate()
    {
        Date ret;
        // code adapted from http://stackoverflow.com/a/1765088/2683223 Jerry Coffin
        const char* build_date = __DATE__;
        char s_month[5];
        static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

        std::sscanf(build_date, "%s %d %d", s_month, &ret.day, &ret.year);

        ret.month = (strstr(month_names, s_month) - month_names) / 3;

        ret.month++; // humans count Jan as month 1, not zero
        return ret;
    }

    std::string Date::getCurrentSystemTime()
    {
        auto tt = std::chrono::system_clock::to_time_t
        (std::chrono::system_clock::now());
        struct tm* ptm = localtime(&tt);
        char date[60] = { 0 };
        sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
        return std::string(date);
    }

    std::string Date::getBuildDateTimeStr()
    {
        Date ret;
        const char* build_date = __DATE__;
        const char* build_time = __TIME__;

        char s_month[5];
        static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

        std::sscanf(build_date, "%s %d %d", s_month, &ret.day, &ret.year);
        std::sscanf(build_time, "%d:%d:%d", &ret.hour, &ret.min, &ret.sec);

        ret.month = (strstr(month_names, s_month) - month_names) / 3;

        ret.month++; // humans count Jan as month 1, not zero

        std::ostringstream str;
        str << std::setfill('0') << std::setw(4) << ret.year << "-"
            << std::setfill('0') << std::setw(2) << ret.month << "-"
            << std::setfill('0') << std::setw(2) << ret.day << " "
            << std::setfill('0') << std::setw(2) << ret.hour << ":"
            << std::setfill('0') << std::setw(2) << ret.min << ":"
            << std::setfill('0') << std::setw(2) << ret.sec;
        return std::move(str.str());
    }

} // namespace cura52