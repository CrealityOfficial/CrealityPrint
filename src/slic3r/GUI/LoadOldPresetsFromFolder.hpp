#ifndef slic3r_LoadOldPresetFromFolder_hpp_
#define slic3r_LoadOldPresetFromFolder_hpp_

#include "LoadOldPresets.hpp"

namespace Slic3r {
namespace GUI {

    class path;
    class LoadOldPresetsFromFolder : public LoadOldPresets
    {
    public:
        LoadOldPresetsFromFolder();
        ~LoadOldPresetsFromFolder();
        
        bool loadPresets(const std::string& presetFolder) override;
        size_t      getLoadFileSize();
        static bool hasOldPresets(const std::string& presetFolder);

    protected:
        void getSubFileName(const std::string& presetFolder, std::vector<FileData>& vtSubFileName) override;
        void        getSubSubFileName(const std::string& presetFolder, std::vector<FileData>& vtSubFileName);
        void        getSubSubSubFileName(const std::string& presetFolder, std::vector<FileData>& vtSubFileName);
        std::string getTmpOutputName(const std::string& fileName) override;
        std::string getCompatiblePrinters(const FileData& fileData) override;

    protected:
        std::vector<FileData> m_vtFileName;
        std::vector<FileData> m_vtLoadedSuccessFileName;
        std::vector<FileData> m_vtLoadedFailFileName;

    };

}
}

#endif
