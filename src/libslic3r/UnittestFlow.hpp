#ifndef _NULLSPACE_UNITTEST_1590155449305_H
#define _NULLSPACE_UNITTEST_1590155449305_H
#include <string>
#include <vector>
namespace Slic3r 
{
    
    enum SlicerUnitType
    {
        None = 0,
        GanerateBL,
        CompareBL,
        UpdateBL
    };
    struct MyJsonMessage
    {
        std::string path = "";
        std::string startTime = "";
        std::string endTime = "";
        std::string result = "";
        std::string reason = "";
    };

    class  BLCompareTestFlow
    {
    public:
        static void initData(std::string exepath);
        static void readSlicerBLConfig(const std::string&jsonName);

        static void startBLTest(std::function<void(std::string)> callback);
        static void endBLTest();
        static void endBLTestFlowError(const std::string & error);
        //
        static void nextBLTest(std::function<void(std::string)> callback);
        static bool isBLTestEnd();
        static bool enabled();
        static int  blType();
        static bool sliceCacheEnabled();
        static std::string _3MFPath();
        static std::string BLPath();
        static std::string CompareResultPath();
        static void setEnabled(bool enable);
        static void setSliceCacheEnabled(bool enable);
        static void set3mfPath(const std::string& path);
        static void setScpList(const std::vector<std::string>& scpListPath);
        static void setBLPath(const std::string&blpath);
        static void setCompareResultPath(const std::string&path);
        static void setFuncType(int type);
        static bool messageEnabled();
        static void setMessageEnabled(bool enable);
        static void loadText(const std::string& filename, std::string& resultText);
        static std::vector<std::string> entryFileInfoList(const std::string& directory);
        static bool removeResultCache();
        static void writeResultData();
        static void copyDirectoryStructureByFilter(const std::string& filterName, const std::string& sourcePath, std::string& destinationPath);
    };

}
#endif // _NULLSPACE_UNITTEST_1590155449305_H
