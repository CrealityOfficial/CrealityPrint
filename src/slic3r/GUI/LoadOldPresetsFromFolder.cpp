#include "LoadOldPresetsFromFolder.hpp"

#include <fstream>
#include <algorithm>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "GUI.hpp"
#include "GUI_App.hpp"
#include "Plater.hpp"

#include <iostream>
#include <fstream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <miniz/miniz.h>

namespace Slic3r { 
namespace GUI {
LoadOldPresetsFromFolder::LoadOldPresetsFromFolder():LoadOldPresets() {

}
LoadOldPresetsFromFolder::~LoadOldPresetsFromFolder() {}

bool LoadOldPresetsFromFolder::loadPresets(const std::string& presetFolder)
{
    bool bRet = false;
    do {
        fs::path              folderPath = boost::filesystem::path(presetFolder).make_preferred();
        if (!fs::exists(folderPath)) {
            setLastError("1", "presetFolder not exist");
            break;
        }

        createTmpDir();

        fs::path              filePath = boost::filesystem::path(presetFolder).make_preferred();
        std::string           file     = filePath.string();
        m_vtFileName.clear();
        m_vtLoadedSuccessFileName.clear();
        m_vtLoadedFailFileName.clear();

        getSubFileName(file, m_vtFileName);
        if (!m_bIsOldPresets) {
            setLastError("2", "not old presets");
            break;
        }

        //  判断预设值是否存在
        PresetBundle::STOverrideConfirmFile stOverrideConfirmFile;
        stOverrideConfirmFile.fileName = "5.x Presets";
        for (FileData& item : m_vtFileName) {
            int ret = isPresetExist(item);
            if (ret == 1) {
                if (item.type == DefJson) {
                    if (item.isSystemPreset)
                        continue;
                    stOverrideConfirmFile.lstPrinterPreset.push_back(item.fileName);
                } else if (item.type == Filament) {
                    stOverrideConfirmFile.lstFilamentPreset.push_back(item.fileName);
                } else if (item.type == Process) {
                    stOverrideConfirmFile.lstProcessPreset.push_back(item.fileName);
                }
            } else if (ret == -2) {
                item.isSystemPreset = true;
            }
        }
        m_override = 1; // 1-覆盖 4-创建副本
        if (stOverrideConfirmFile.lstPrinterPreset.size() != 0 || stOverrideConfirmFile.lstFilamentPreset.size() != 0 ||
            stOverrideConfirmFile.lstProcessPreset.size() != 0) {
            if (m_overrideConfirmCb) {
                std::list<PresetBundle::STOverrideConfirmFile*> lstOverrideConfirmFile;
                lstOverrideConfirmFile.push_back(&stOverrideConfirmFile);
                m_override = m_overrideConfirmCb(lstOverrideConfirmFile);
            }
        }

        int errFileCout = 0;
        m_printerData   = PrinterData();
        //  导入打印机预设
        for (auto item : m_vtFileName) {
            if (item.type == DefJson) {
                if (item.isSystemPreset) {
                    m_printerData.printerInherits = item.inheritsName;
                    bRet = true;
                    continue;
                }
                bRet = doDefJson(item);
                if (bRet) {
                    m_vtLoadedSuccessFileName.push_back(item);
                } else {
                    m_vtLoadedFailFileName.push_back(item);
                }
            }
        }
        //if (!bRet) {
        //    setLastError("3", "load def json fail");
        //    break;
        //}
        // 导入耗材、工艺预设
        for (auto item : m_vtFileName) {
            if (item.type == DefJson) {
                continue;
            }
            if (item.isSystemPreset) {
                continue;
            }
            if (item.type == Filament) {
                bRet = doFilamentJson(item);
            } else if (item.type == Process) {
                bRet = doProcessJson(item);
            }

            if (bRet) {
                m_vtLoadedSuccessFileName.push_back(item);
            } else {
                m_vtLoadedFailFileName.push_back(item);
            }
            if (!bRet) {
                errFileCout++;
            }
        }

        //  重新加载预设文件
        auto* app_config = GUI::wxGetApp().app_config;
        GUI::wxGetApp().preset_bundle->load_presets(*app_config, ForwardCompatibilitySubstitutionRule::EnableSilentDisableSystem);
        GUI::wxGetApp().load_current_presets();
        GUI::wxGetApp().plater()->set_bed_shape();

    } while (0);
    return bRet;
}

size_t LoadOldPresetsFromFolder::getLoadFileSize() { 
    //return m_vtFileName.size(); 
    return m_vtLoadedSuccessFileName.size();
}

bool LoadOldPresetsFromFolder::hasOldPresets(const std::string& presetFolder)
{
    fs::path path(presetFolder);
    if (!fs::exists(path) || fs::is_empty(path))
        return false;
    for (const auto& item : fs::directory_iterator(path)) {
        if (fs::is_directory(item.path())) {
            for (const auto& item2 : fs::directory_iterator(item.path())) {
                if (!fs::is_directory(item2.path())) {
                    std::string fileName = item2.path().filename().string();
                    int         pos      = fileName.find(".def.json");
                    if (pos != -1) {
                        return true;
                    }
                    pos = fileName.find(".json");
                    if (pos != -1) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

void LoadOldPresetsFromFolder::getSubFileName(const std::string& presetFolder, std::vector<FileData>& vtSubFileName) {
    m_lastError = LastError();
    try {
        fs::path path(presetFolder);
        for (const auto& item : fs::directory_iterator(path)) {
            if (fs::is_directory(item.path())) {
                getSubSubFileName(item.path().string(), vtSubFileName);
            }
        }
    }
    catch (const std::exception& e) {
        setLastError("2", "getSubFileName fail,crash");
    }
}

void LoadOldPresetsFromFolder::getSubSubFileName(const std::string& presetFolder, std::vector<FileData>& vtSubFileName) {
    try {
        fs::path path(presetFolder);
        for (const auto& item : fs::directory_iterator(path)) {
            if (fs::is_directory(item.path())) {
                getSubSubSubFileName(item.path().string(), vtSubFileName);
                continue;
            }
            fs::path    targetFilePath(item.path().string());
            std::string targetFileName = targetFilePath.make_preferred().string();
            FileData    fileData;
            fileData.name     = targetFileName;
            fileData.fileName = targetFilePath.filename().stem().string();
            int pos           = targetFileName.find(".def.json");
            if (pos != std::string::npos) {
                int pos2              = fileData.fileName.find(".def");
                fileData.fileName     = fileData.fileName.substr(0, pos2);
                fileData.inheritsName = getInherits(fileData.fileName);
                if (fileData.inheritsName.empty()) {
                    std::string printerName = "";
                    std::string nozzle      = "";
                    splitPrinterPresetFileName(fileData.fileName, printerName, nozzle);
                    fileData.loadOldPresetFromFolderFileData.inheritsName = printerName;
                }

                PresetCollection* collection = &GUI::wxGetApp().preset_bundle->printers;
                Preset*           p          = collection->find_preset(fileData.inheritsName, false);
                if (p) {
                    if (p->is_default || p->is_system) {
                        fileData.inheritsName   = fileData.inheritsName;
                        fileData.isSystemPreset = true;
                    }
                }
                fileData.type   = DefJson;
                m_bIsOldPresets = true;
            } else {
                continue;
            }
            vtSubFileName.push_back(std::move(fileData));
        }
    } catch (const std::exception& e) {
        setLastError("2", "getSubSubFileName fail,crash");
    }
}

void LoadOldPresetsFromFolder::getSubSubSubFileName(const std::string& presetFolder, std::vector<FileData>& vtSubFileName) {
    try {
        fs::path path(presetFolder);
        for (const auto& item : fs::directory_iterator(path)) {
            if (fs::is_directory(item.path())) {
                getSubSubFileName(item.path().string(), vtSubFileName);
                continue;
            }
            fs::path    targetFilePath(item.path().string());
            std::string targetFileName = targetFilePath.make_preferred().string();
            FileData    fileData;
            fileData.name     = targetFileName;
            std::string ssExt = targetFilePath.extension().string();
            if (ssExt == ".json") {
                fileData.fileName = targetFilePath.filename().stem().string();
            } else {
                int pos = targetFileName.rfind("\\");
                if (pos != -1) {
                    fileData.fileName = targetFileName.substr(pos+1, targetFileName.length() - pos);
                }
            }
            int pos           = targetFileName.find(".def.json");
            if (pos != std::string::npos) {
                int pos2              = fileData.fileName.find(".def");
                fileData.fileName     = fileData.fileName.substr(0, pos2);
                fileData.inheritsName = getInherits(fileData.fileName);

                PresetCollection* collection = &GUI::wxGetApp().preset_bundle->printers;
                Preset*           p          = collection->find_preset(fileData.inheritsName, false);
                if (p) {
                    if (p->is_default || p->is_system) {
                        fileData.inheritsName   = fileData.inheritsName;
                        fileData.isSystemPreset = true;
                    }
                }
                fileData.type   = DefJson;
                m_bIsOldPresets = true;
            } else if (path.filename() == "Materials") {
                fileData.type = Filament;
                std::string parentPathName                            = path.parent_path().filename().string();
                std::string printerName    = "";
                std::string nozzle         = "";
                splitPrinterPresetFileName(parentPathName, printerName, nozzle);
                if (parentPathName.find("Creality") != std::string::npos && printerName.find(nozzle+" nozzle") != std::string::npos) {
                    fileData.loadOldPresetFromFolderFileData.inheritsName = printerName;
                } else {
                    fileData.loadOldPresetFromFolderFileData.inheritsName = getInherits(parentPathName);
                    if (fileData.loadOldPresetFromFolderFileData.inheritsName.empty()) {
                        fileData.loadOldPresetFromFolderFileData.inheritsName = printerName;
                    }
                }
                m_bIsOldPresets = true;
            } else if (path.filename() == "Processes") {
                fileData.type                                         = Process;
                std::string parentPathName = path.parent_path().filename().string();
                std::string printerName = "";
                std::string nozzle      = "";
                splitPrinterPresetFileName(parentPathName, printerName, nozzle);
                fileData.loadOldPresetFromFolderFileData.inheritsName = getInherits(path.parent_path().filename().string());
                if (fileData.loadOldPresetFromFolderFileData.inheritsName.empty()) {
                    fileData.loadOldPresetFromFolderFileData.inheritsName = printerName;
                }
                m_bIsOldPresets                                       = true;
            } else {
                continue;
            }
            vtSubFileName.push_back(std::move(fileData));
        }
    } catch (const std::exception& e) {
        setLastError("2", "getSubSubSubFileName fail,crash");
    }
}

std::string LoadOldPresetsFromFolder::getTmpOutputName(const std::string& fileName) { 
    if (fileName.empty())
        return "";

    const UserInfo& userInfo = wxGetApp().get_user();
    std::string     tmpOutputFileName;
    fs::path path(fileName);
    tmpOutputFileName = m_temp_folder.string() + "/" + path.filename().string() + ".tmp";

    return tmpOutputFileName; 
}

std::string LoadOldPresetsFromFolder::getCompatiblePrinters(const FileData& fileData)
{
    return fileData.loadOldPresetFromFolderFileData.inheritsName;
}

}} // namespace Slic3r::GUI
