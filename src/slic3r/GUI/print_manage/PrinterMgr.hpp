#ifndef slic3r_PrinterMgr_hpp_
#define slic3r_PrinterMgr_hpp_
#include <string>

#include "nlohmann/json.hpp"

namespace Slic3r {
    namespace GUI {
        class DeviceMgr {
        public:
            struct Data
            {
                int connectType;
                std::string model;
                std::string mac;
                std::string address;
                std::string name;
            };
        public:
            DeviceMgr();
            ~DeviceMgr();
            void Load();
            void Save();
            void AddDevice(std::string group, Data&data);
            void RemoveDevice(std::string name);
            void EditDeiveName(std::string name, std::string nameNew);
            void AddGroup(std::string name);
            void RemoveGroup(std::string name);
            void EditGroupName(std::string name, std::string nameNew);
            void SetMergeState(bool state);
            bool IsMergeState();
            std::string ToString(); 
            nlohmann::json GetData();
            void Get(std::map<std::string, std::vector<DeviceMgr::Data>>&store, std::vector<std::string>&order);
            bool IsGroupExist(std::string name);
            bool IsPrinterExist(std::string mac);
            std::vector<std::string> GetSamePrinter(std::string mac);
        private:
            struct priv;
            std::unique_ptr<priv> p;
        public:
            static DeviceMgr&Ins();
        };

    } // GUI
} // Slic3r

#endif /* slic3r_Tab_hpp_ */
