#ifndef slic3r_LoadOldPreset_hpp_
#define slic3r_LoadOldPreset_hpp_
#include <string>
#include <functional>
#include <boost/filesystem/path.hpp>
#include "../Utils/json_diff.hpp"
#include "libslic3r/PresetBundle.hpp"

namespace Slic3r {
namespace GUI {

    class path;
    class LoadOldPresets
    {
    public:
        struct LastError {
            std::string code = "";
            std::string message = "";
            std::string requestId = "";
        };
        enum FileDataType {Default,DefJson,Filament,Process};
        struct LoadOldPresetFromFolderFileData
        {
            std::string inheritsName;
        };
        struct FileData
        {
            std::string  name = "";  // fileNamePath
            std::string  fileName = ""; // only fileName no Path
            std::string  inheritsName = "";
            std::string  outputName     = "";
            bool         isSystemPreset = false;
            FileDataType type = Default;
            LoadOldPresetFromFolderFileData loadOldPresetFromFolderFileData; 
        };
        struct PrinterData
        {
            std::string printerInherits = "";
            std::string filamentType    = "";
            std::string nozzle = "";
        };
        LoadOldPresets();
        virtual ~LoadOldPresets();
        
        void setOverrideConfirmCb(std::function<int(const std::list<Slic3r::PresetBundle::STOverrideConfirmFile*>&)> overrideConfirmCb);
        virtual bool loadPresets(const std::string& fileName);
        bool isOldPresets() { return m_bIsOldPresets; }

        const LastError& getLastError() { return m_lastError; }

    protected:
        void createTmpDir();
        void setLastError(const std::string& code, const std::string& msg);
        virtual void getSubFileName(const std::string& zipFileName, std::vector<FileData>& vtSubFileName);
        bool loadMachineList();
        bool loadSystemFilamentFile();
        bool loadSystemProcessFile();
        std::string getPrinterName(const std::string& printerIntName);
        void        splitPrinterPresetFileName(const std::string& printerPresetFileName, std::string& printerName, std::string& nozzle);
        std::string getInherits(const std::string& inherits);
        std::string getFilamentInherits(const std::string& inherits);
        std::string getProcessInherits(const std::string& layerHeight);
        std::string getFilamentInheritsFileName(const std::string& inheritsType);
        std::string getProcessInheritsFileName(const std::string& inheritsLayerHeight);
        bool        doDefJson(const FileData& fileData);
        bool        doFilamentJson(const FileData& fileData);
        bool        doProcessJson(const FileData& fileData);
        int        isPresetExist(const FileData& fileData);
        virtual std::string  getTmpOutputName(const std::string& fileName);
        virtual std::string getCompatiblePrinters(const FileData& fileData);

    protected:
        LastError m_lastError;
        bool      m_bIsOldPresets = false;
        boost::filesystem::path m_temp_folder;
        json                    m_jsonMachineList = json();
        std::vector<std::string> m_vtSystemFilamentFile;
        std::vector<std::string> m_vtSystemProcessFile;
        PrinterData              m_printerData;
        std::function<int(const std::list<Slic3r::PresetBundle::STOverrideConfirmFile*>&)> m_overrideConfirmCb = nullptr;
        int                                                                        m_override          = 1; // 1-覆盖 4-创建副本
    };

}
}

#endif
