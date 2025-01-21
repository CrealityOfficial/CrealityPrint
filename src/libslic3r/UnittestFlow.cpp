#include "unittestflow.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <iostream>
#include <filesystem>
#include <functional>
#include <nlohmann/json.hpp>
#include <Windows.h>
#include <algorithm>

namespace Slic3r
{
    using namespace std;
    static std::string m_scpPath = "";
    static std::string m_3mfDir = "";
    static std::string m_BlPath = "";
    static std::string g_relativePath = "";
    static std::string m_resultPath = "";
    static bool m_enabled = false;
    static bool g_message_enabled = true;
    static int m_curFuncType = 1;     //0:generate 1:compare 2:update
    static int m_cur_file_index = 0;
    
    //static bool g_only_slice_enabled = false;   //false :基线对比   true: 完整切片流程，并保存切片数据
    static std::vector<std::string> m_scpListFiles;  //一个scp列表文件
    static std::vector<MyJsonMessage>m_jsonVec;
    

    std::string getCurrentTime() {
        // 获取当前时间点
        auto now = std::chrono::system_clock::now();

        // 转换为time_t以便使用std::localtime
        std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm = *std::localtime(&now_time_t);

        // 使用stringstream格式化时间
        std::stringstream ss;
        ss << std::put_time(&now_tm, "%d.%m.%Y %H:%M:%S");
        return ss.str();
    }
    std::string stringToUTF8(const std::string& str) {
        // 将 std::string 转换为宽字符字符串
        int wideCharSize = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
        if (wideCharSize == 0) {
            return "";
        }
        std::wstring wideStr(wideCharSize, 0);
        MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wideStr[0], wideCharSize);

        // 将宽字符字符串转换为 UTF-8 编码的字符串
        int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
        if (utf8Size == 0) {
            return "";
        }
        std::string utf8Str(utf8Size - 1, 0); // 减去1以排除终止符
        WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], utf8Size - 1, NULL, NULL);

        return utf8Str;
    }
    std::string UTF8ToGB(const char* str) {
        std::string result;
        WCHAR* strSrc;
        LPSTR szRes;

        //获得临时变量的大小
        int i = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
        strSrc = new WCHAR[i + 1];
        MultiByteToWideChar(CP_UTF8, 0, str, -1, strSrc, i);

        //获得临时变量的大小
        i = WideCharToMultiByte(CP_ACP, 0, strSrc, -1, NULL, 0, NULL, NULL);
        szRes = new CHAR[i + 1];
        WideCharToMultiByte(CP_ACP, 0, strSrc, -1, szRes, i, NULL, NULL);

        result = szRes;
        delete[]strSrc;
        delete[]szRes;

        return result;
    }
    std::string& replace_str(std::string& str, const std::string& to_replaced, const std::string& newchars){
        for (std::string::size_type pos(0); pos != std::string::npos; pos += newchars.length())
        {
            pos = str.find(to_replaced, pos);
            if (pos != std::string::npos)
                str.replace(pos, to_replaced.length(), newchars);
            else
                break;
        }
        return   str;
    }

    // 
    std::string trim_str(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        size_t end = str.find_last_not_of(" \t\n\r");

        if (start == std::string::npos || end == std::string::npos) {
            return "";
        }

        return str.substr(start, end - start + 1);
    }
    void deleteFilesInDirectory(const std::string& directoryPath) {
        namespace fs = std::filesystem;

        try {
            for (const auto& entry : fs::directory_iterator(directoryPath)) {
                if (fs::is_regular_file(entry.path())) {
                    if (entry.path().string() == "readme.txt") continue;
                    fs::remove(entry.path());
                }
            }
        }
        catch (const fs::filesystem_error& e) {
        }
    }
    std::string getRelativePath(const std::string& path) {
        size_t firstSlash = path.find('/');
        if (firstSlash == std::string::npos) {
            return std::string(); // 如果没有斜杠，返回整个路径
        }
        size_t secondSlash = path.find('/', firstSlash + 1);
        if (secondSlash == std::string::npos) {
            return std::string(); // 如果只有一个斜杠，返回空
        }
        return path.substr(0, secondSlash);
    }
    void BLCompareTestFlow::readSlicerBLConfig(const std::string& jsonName) {
        std::ifstream testFile(jsonName);
        if (!testFile) {
            std::cerr << "cannot read file" << std::endl;
            return;
        }
        nlohmann::json testObject;
        testFile >> testObject;

        std::string type = testObject["type"];
        if(type == "Generate")
		{
			m_curFuncType = 1;
		}
		else if(type == "Compare")
		{
			m_curFuncType = 2;
		}
		else if(type == "Update")
		{
			m_curFuncType = 3;
		}
		else
		{
			m_curFuncType = 0;
		}

        nlohmann::json inputObject = testObject["input"];
        m_scpPath = inputObject["script dir"];
        m_3mfDir = inputObject["3mf dir"];
        m_BlPath = inputObject["baseline dir"];
        nlohmann::json outputObject = testObject["output"];
        m_resultPath = outputObject["result dir"];
        m_enabled = true;
        deleteFilesInDirectory(m_resultPath);
        //

    }
    void BLCompareTestFlow::initData(std::string exepath) {
        std::filesystem::path path(exepath);
        std::string config_path = path.parent_path().string() + "/unittest_config/config.json";
        m_enabled = false;
        if (!std::filesystem::exists(config_path)) 
        {
            
            return;
        }
        //1.read config.json
        std::ifstream inFile(config_path);
        if (!inFile) {
			std::cerr << "cannot read file" << std::endl;
			return;
		}
        nlohmann::json jsonObject;
        inFile >> jsonObject;
        bool runStatus = jsonObject["runStatus"];
        if(!runStatus)
		{
			return;
		}
        //2. reset runStatus is false into config.json

        jsonObject["runStatus"] = false;
        std::ofstream file(config_path);
        file << jsonObject.dump(4);
        //bltest
        std::string testTypeName = jsonObject["testTypeName"];
        if (testTypeName == "SlicerBaseline")
        {
            std::string test_config_path = path.parent_path().string() + "/unittest_config/" + testTypeName + ".json";
            readSlicerBLConfig(test_config_path);
        }
        
    }
    void BLCompareTestFlow::setFuncType(int type) {
        m_curFuncType = type;
    }
    bool BLCompareTestFlow::messageEnabled()
    {
        return g_message_enabled;
    }
    void BLCompareTestFlow::setMessageEnabled(bool enable)
    {
        g_message_enabled = enable;
    }
    void BLCompareTestFlow::loadText(const std::string& filename,std::string&resultText)
    {
        std::string strTmp;
        std::ifstream infile(filename);
        if (infile.is_open()) {
            std::getline(infile, strTmp);
            infile.close();
        }
        else {
            std::cout << "Unable to open file";
        }
        resultText = UTF8ToGB(strTmp.c_str()).c_str();
    }
    void BLCompareTestFlow::startBLTest(std::function<void(std::string)> callback)
    {
        if (!enabled())return;
        std::string tmpBlPath = m_BlPath;
        g_relativePath = "";
        //1.BLTEST 开始导入文件。
        //QDir dir(m_scpPath);
        //
        auto _3mfSourceDir = [=]() {
            {
                //源码路径
                const std::string sourcePath = m_3mfDir;
                return sourcePath;
            };
        };

        // 获取所有文件和目录
        std::vector<std::string> list = entryFileInfoList(m_scpPath);
        
        if (m_cur_file_index >= list.size())return;
        if (m_cur_file_index == 0)
        {
            m_jsonVec.clear();
            m_jsonVec.resize(list.size());
        }
        auto filename = list[m_cur_file_index];
        //for(auto filename: list)
		{
            std::cout << filename << std::endl;
            std::filesystem::path filePath(filename);

            if (!std::filesystem::exists(filePath)) {
                MyJsonMessage jsonMessage = m_jsonVec[m_cur_file_index];
                jsonMessage.path = filename;
                jsonMessage.result = "Failed:";
                jsonMessage.reason = "scp config path is not exist";
                jsonMessage.startTime = getCurrentTime();
                jsonMessage.endTime = getCurrentTime();
                m_jsonVec[m_cur_file_index] = jsonMessage;
                nextBLTest(callback);
                return;
            }

            std::string _3mfPath;
            loadText(filename, _3mfPath);
            _3mfPath = replace_str(_3mfPath, ">import", "");
            _3mfPath = replace_str(_3mfPath, "\"", "");
            _3mfPath = trim_str(_3mfPath);
            
            std::string realPath = _3mfSourceDir() + "/" + _3mfPath;
            g_relativePath = getRelativePath(_3mfPath);
            std::string blPath = m_BlPath + "/" + g_relativePath;
            std::filesystem::path dir(blPath);
            if (!std::filesystem::exists(dir)) {
				std::filesystem::create_directories(dir);
			}
            m_jsonVec[m_cur_file_index].path = filename;
            m_jsonVec[m_cur_file_index].startTime = getCurrentTime();
            if (std::filesystem::path(realPath).extension() == ".3mf")
            {
                removeResultCache();
                callback(realPath);
            }
            else
            {
                nextBLTest(callback);
            }
		}
    }
    void BLCompareTestFlow::endBLTest()
    {

        std::string filename = m_resultPath + "/result.errtxt";
        
        std::filesystem::path filePath(filename);

        if (!std::filesystem::exists(filePath)) {
            MyJsonMessage jsonMessage = m_jsonVec[m_cur_file_index];
			jsonMessage.result = "Pass";
			jsonMessage.endTime = getCurrentTime();
			m_jsonVec[m_cur_file_index] = jsonMessage;
			return;
		}

        std::ifstream inFile(filename);

        if (!inFile) {
            std::cerr << "cannot read file" << std::endl;
            return ;
        }

        std::string line;
        std::string resultMess = "";
        while (std::getline(inFile, line)) {
            std::cout << line << std::endl;
            if(line.find("error:") != std::string::npos)
            {
                if (line.find("error:baseline file generate failed!") != std::string::npos)
                {
                    line = "error:baseline file has exist.generate failed!";
                }
                resultMess += line + "\n";
            }
            else{
                continue;
			}
        }

        m_jsonVec[m_cur_file_index].result = "Failed";
        m_jsonVec[m_cur_file_index].endTime = getCurrentTime();
        m_jsonVec[m_cur_file_index].reason = resultMess;

    }
    void BLCompareTestFlow::endBLTestFlowError(const std::string& error)
    {
        //std::string resultMess = "";
        //if (error == "preSlicerError")
        //{
        //    resultMess = "error:pre slicer is failed,can not slice";
        //}
        //m_jsonVec[m_cur_file_index].result = "Failed";
        //m_jsonVec[m_cur_file_index].endTime = getCurrentTime();
        //m_jsonVec[m_cur_file_index].reason = resultMess;
        //nextBLTest();
    }
    
    void BLCompareTestFlow::nextBLTest(std::function<void(std::string)> callback)
    {
        m_cur_file_index++;
        std::vector<std::string> list = entryFileInfoList(m_scpPath);
        if (m_cur_file_index >= list.size())
        {
            writeResultData();
            std::exit(0);
            return;
        }
        startBLTest(callback);

    }
    bool BLCompareTestFlow::isBLTestEnd()
	{
		return m_cur_file_index >= m_jsonVec.size();
	}

    std::vector<std::string> BLCompareTestFlow::entryFileInfoList(const std::string& directory) {
        std::vector<std::string> findFilesInfo;
        
        namespace fs = std::filesystem;
       
        for (const auto& entry : fs::directory_iterator(directory)) {
            std::cout << entry.path().filename() << std::endl;
            if (fs::is_directory(entry.path())) {
                std::cerr << "Invalid directory: " << directory << std::endl;
                std::vector<std::string> files = entryFileInfoList(entry.path().string());
                findFilesInfo.insert(findFilesInfo.end(), files.begin(), files.end());
            }
            else if(fs::is_regular_file(entry.path())) {
                if(entry.path().extension() == ".scp")
                    findFilesInfo.push_back(entry.path().string());
            }
        }

        //fs::directory_iterator end_iter;
        //for (fs::directory_iterator dir_itr(directory); dir_itr != end_iter; ++dir_itr) {
        //    if (fs::is_regular_file(dir_itr->status())) {
        //        std::cout << dir_itr->path().filename().string() << std::endl;
        //        findFilesInfo.push_back(dir_itr->path().string());
        //    }
        //}
        return findFilesInfo;
    }

    void BLCompareTestFlow::writeResultData()
    {
        int _totalCount = m_jsonVec.size();
        int errCount = 0;
        int passCount = 0;
        nlohmann::json jsonObject;
        nlohmann::json result_array = nlohmann::json::array();
        //QJsonArray result_array;
        std::vector<std::string> pathLists;
        for (int index = 0; index < _totalCount; index++)
        {
            nlohmann::json printerObj = nlohmann::json::object();
            std::string path = m_jsonVec[index].path;
            std::string utfpath = stringToUTF8(path);
            printerObj.push_back({ "path",utfpath });
            printerObj.push_back({ "startTime", m_jsonVec[index].startTime });
            printerObj.push_back({ "endTime", m_jsonVec[index].endTime });
            printerObj.push_back({ "result", m_jsonVec[index].result });
            printerObj.push_back({ "reason", m_jsonVec[index].reason });
            result_array.push_back(printerObj);
            if (m_jsonVec[index].result.find("Pass") != std::string::npos)
            {
                passCount++;
            }
            else
            {
                errCount++;
                pathLists.push_back( m_jsonVec[index].path);
            }
        }
        //QJsonArray count_array;
        //{
            nlohmann::json coun_object = nlohmann::json::object();
            coun_object["total"] = _totalCount;
            coun_object["passcount"] = passCount;
            coun_object["errorcount"] = errCount;
            //count_array.push_back(coun_object);
        //}
        
        auto writeResultJson = [=](){
            {
                nlohmann::json rootObj = nlohmann::json::object();
                rootObj.push_back({ "ResultList", result_array });
                rootObj.push_back({ "Statistics", coun_object });
                // 打开文件
                std::string strPath = m_resultPath + "/compare.result";
                std::ofstream file(strPath);
                // 把 JSON 对象写入文件
                file << rootObj.dump(4);
            };
        };
        //auto writeFailedList = [](std::vector<std::string> paths)
        //{
        //    std::string strPath = m_resultPath + "/failedlist.json";
        //    
        //    nlohmann::json result_fail_array= nlohmann::json::array();
        //    for (const auto& path : paths) {
        //        result_fail_array.push_back({ UTF8ToGB(path.c_str()) });
        //   }
        //    std::ofstream file(strPath);
        //    // 把 JSON 对象写入文件
        //    file << result_fail_array.dump(4);
        //};
        writeResultJson();
        //writeFailedList(pathLists);
    }

    bool BLCompareTestFlow::removeResultCache()
    {
        std::string filename = m_resultPath + "/result.errtxt";
        remove(filename.c_str());
        return true;
    }
    void BLCompareTestFlow::copyDirectoryStructureByFilter(const std::string& filterName,const std::string& sourcePath, std::string& destinationPath)
    {
        //QFileInfo info(sourcePath);
        //std::stringList parts = info.absolutePath().split("/");
        //int index = parts.indexOf(filterName);
        //if (sourcePath.contains(filterName) && parts.indexOf(filterName) > 0)
        //{
        //    if (info.isDir() || info.isFile())
        //    {
        //        for (int i = index + 1; i < parts.size(); ++i) {
        //            if (i != index) {
        //                destinationPath += "/" + parts.at(i);
        //                QDir destDir(destinationPath);
        //                if (!destDir.exists()) {
        //                    if (!destDir.mkpath(destinationPath)) {
        //                        continue;
        //                    }
        //                }
        //            }
        //        }
        //    }
        //}
    }
    bool BLCompareTestFlow::enabled()
    {
        return m_enabled;
    }
    int BLCompareTestFlow::blType()
	{
		return m_curFuncType;
	}
    bool BLCompareTestFlow::sliceCacheEnabled()
    {
        return true;
    }
    std::string BLCompareTestFlow::_3MFPath()
    {
        return m_scpPath;
    }
    std::string BLCompareTestFlow::BLPath()
    {
        return m_BlPath + "/" + g_relativePath;
    }
    std::string BLCompareTestFlow::CompareResultPath()
    {
        return m_resultPath;
    }
    void BLCompareTestFlow::setEnabled(bool enable)
    {
        m_enabled = enable;
    }
    void BLCompareTestFlow::setSliceCacheEnabled(bool enable)
    {
        //g_only_slice_enabled = enable;
    }
    void BLCompareTestFlow::set3mfPath(const std::string& path)
    {
        m_scpPath = path;
    }
    void BLCompareTestFlow::setScpList(const std::vector<std::string>& scpListPath)
    {
        //m_scpListFiles = scpListPath;
    }
    void BLCompareTestFlow::setBLPath(const std::string& blpath)
    {
        m_BlPath = blpath;
    }
    void BLCompareTestFlow::setCompareResultPath(const std::string& path)
    {
        m_resultPath = path;
    }
}
