#include "PrinterMgr.hpp"

#include "../I18N.hpp"
#include "PrinterMgr.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "libslic3r_version.h"
namespace pt = boost::property_tree;
using json = nlohmann::json;

namespace Slic3r {
namespace GUI {

    class DeviceLoaderV512
    {
    public:
        void Load(DeviceMgr* mgr, boost::filesystem::path& path)
        {
            boost::nowide::ifstream t(path.string());
            std::stringstream buffer;
            buffer << t.rdbuf();

            json data = json::parse(buffer);
            std::vector<std::string> vtGroup;
            if (data.contains("deviceGroupNames"))
            {
                for (auto& group : data["deviceGroupNames"])
                {
                    mgr->AddGroup(group);
                    vtGroup.push_back(group);
                }
            }

            if (data.contains("deviceInfomation"))
            {
                for (auto& device : data["deviceInfomation"])
                {
                    DeviceMgr::Data data;
                    if (device.contains("connectType"))data.connectType = device["connectType"];
                    if (device.contains("modelName"))data.model = device["modelName"];
                    if (device.contains("macAddress"))data.mac = device["macAddress"];
                    if (device.contains("ipAddress"))data.address = device["ipAddress"];
                    if (device.contains("deviceName"))data.name = device["deviceName"];

                    std::string sGroup;
                    if (device.contains("group")) {
                        int groupIndex = device["group"].get<int>() - 1;
                        sGroup = vtGroup[groupIndex];
                    }

                    if(!mgr->IsPrinterExist(data.mac))
                        mgr->AddDevice(sGroup, data);
                }
            }
        }
    };
    
    struct DeviceMgr::priv
    {
        json data;
        std::map<std::string, std::vector<DeviceMgr::Data>> store;
        std::vector<std::string> order;
    };

    DeviceMgr::DeviceMgr():p(new priv)
    {

    }

    DeviceMgr::~DeviceMgr()
    {
    }

    void DeviceMgr::Load()
    {
        boost::filesystem::path device_file = boost::filesystem::path(Slic3r::data_dir()) / "deviceInfo.json";
        if (!boost::filesystem::exists(device_file))
        {
            boost::filesystem::path device_old_file = boost::filesystem::path(Slic3r::data_dir()).parent_path().parent_path() / "Creative3D/deviceInfo.json";
            if (boost::filesystem::exists(device_old_file))
            {
                DeviceLoaderV512 loader;
                loader.Load(this, device_old_file);
                this->Save();
            }
        }
        else//load from current custom folder
        {
            boost::nowide::ifstream t(device_file.string());
            std::stringstream buffer;
            buffer << t.rdbuf();

            p->data = json::parse(buffer);

            std::vector<std::string> delIP;
            if(p->data.contains("groups"))
            {
                for (auto& group : p->data["groups"])
                {
                    if (group.contains("list"))
                    {
                        for (auto jt = group["list"].begin(); jt != group["list"].end(); jt++) {
                            std::vector<std::string> ips = this->GetSamePrinter(jt.value()["mac"].get<std::string>());
                            if(!ips.empty()){
                                for(auto& ip : ips)
                                    delIP.push_back(ip);
                            }
                        }
                    }
                }
            }

            for(auto&ip:delIP)
            {
                this->RemoveDevice(ip);
            }
        }

        if(p->data.empty())
        {
            this->AddGroup(_u8L("Default"));
        }
    }

    void DeviceMgr::Save()
    {
        boost::filesystem::path device_file = boost::filesystem::path(Slic3r::data_dir()) / "deviceInfo.json";

        boost::nowide::ofstream c;
        c.open(device_file.string(), std::ios::out | std::ios::trunc);
        c << std::setw(4) << p->data << std::endl;

    }

    void DeviceMgr::AddDevice(std::string group, Data& data)
    {
        json item;
        item["address"] = data.address;
        item["mac"] = data.mac;
        item["model"] = data.model;
        item["name"] = data.name;
        item["connectType"] = data.connectType;
 
        for(auto&g: p->data["groups"])
        {
            if (g["group"] == group)
            {
                if(!g.contains("list"))
                {
                    g["list"].push_back(item);
                }
                else
                    g["list"].insert(g["list"].begin(), item);

                this->Save();
                break;
             }
        }
    }

    void DeviceMgr::RemoveDevice(std::string address)
    {
        for (auto& group : p->data["groups"])
        {
            int index = 0;
            for(auto&item:group["list"])
            {
                std::string ip = item["address"];
                if(ip == address)
                {
                    group["list"].erase(index);
                    this->Save();
                    return;
                }
                index++;
            }
        }
    }

    void DeviceMgr::EditDeiveName(std::string address, std::string name)
    {
        for (auto& group : p->data["groups"])
        {
            int index = 0;
            for (auto& item : group["list"])
            {
                std::string ip = item["address"];
                if (ip == address)
                {
                    item["name"] = name;
                    this->Save();
                    return;
                }
                index++;
            }
        }
    }

    void DeviceMgr::AddGroup(std::string name)
    {
        auto &groups = p->data["groups"];
        
        json item;
        item["group"] = name;
        groups.push_back(item);

        this->Save();
    }

    void DeviceMgr::RemoveGroup(std::string name)
    {
        int index = 0;
        for (auto& group : p->data["groups"])
        {
            if (group["group"] == name)
            {
                p->data["groups"].erase(index);
                this->Save();
                return;
            }

            index++;
        }
    }

    void DeviceMgr::EditGroupName(std::string name, std::string nameNew)
    {
        int index = 0;
        for (auto& group : p->data["groups"])
        {
            if(group["group"] == name)
            {
                group["group"] = nameNew;
                this->Save();
                return;
            }
            
            index++;
        }
    }

    void DeviceMgr::SetMergeState(bool state)
    {
        p->data["mergeState"] = state;
        this->Save();
    }

    bool DeviceMgr::IsMergeState()
    {
        bool mergeState = false;
        if(p->data.contains("mergeState")){
            mergeState = p->data["mergeState"].get<bool>();
        }
        
        return mergeState;
    }

    std::string DeviceMgr::ToString()
    {
        return p->data["groups"].dump(-1, ' ', true);
    }

    nlohmann::json DeviceMgr::GetData()
    {
        return p->data["groups"];
    }

    void DeviceMgr::Get(std::map<std::string, std::vector<DeviceMgr::Data>>& store, std::vector<std::string>& order)
    {
        for (auto it = p->data["groups"].begin(); it != p->data["groups"].end(); it++)
        {
            auto& group = it.value();
            std::string name = group["group"];
            store[name];
            if (group.contains("list"))
            {
                for (auto jt = group["list"].begin(); jt != group["list"].end(); jt++)
                {
                    Data data;
                    data.address = jt.value()["address"].get<std::string>();
                    data.connectType = jt.value()["connectType"].get<int>();
                    data.mac = jt.value()["mac"].get<std::string>();
                    data.model = jt.value()["model"].get<std::string>();
                    data.name = jt.value()["name"].get<std::string>();

                    store[name].push_back(data);
                }
            }
            
            if(std::find(order.begin(), order.end(), name) == order.end())
                order.push_back(name);
        }
    }

    bool DeviceMgr::IsGroupExist(std::string name)
    {
        for (auto& group : p->data["groups"])
        {
            if(group["group"] == name)
                return true;
        }

        return false;
    }

    bool DeviceMgr::IsPrinterExist(std::string mac)
    {
        if(p->data.contains("groups"))
        {
            for (auto& group : p->data["groups"])
            {
                if (group.contains("list"))
                {
                    for (auto jt = group["list"].begin(); jt != group["list"].end(); jt++) {
                        if (mac == jt.value()["mac"].get<std::string>())
                            return true;
                    }
                }
            }
        }
        
        return false;
    }

    std::vector<std::string> DeviceMgr::GetSamePrinter(std::string mac)
    {
        std::vector<std::string> vt;
        int cnt = 0;
        if (p->data.contains("groups"))
        {
            for (auto& group : p->data["groups"])
            {
                if (group.contains("list"))
                {
                    for (auto jt = group["list"].begin(); jt != group["list"].end(); jt++) {
                        if (mac == jt.value()["mac"].get<std::string>())
                        {
                            if(cnt!=0)
                            {
                                std::string ip = jt.value()["address"].get<std::string>();
                                vt.push_back(ip);
                            }
                            cnt++;
                        }
                    }
                }
            }
        }
        return vt;
    }

    DeviceMgr& DeviceMgr::Ins()
    {
        static DeviceMgr mgr;
        return mgr;
    }

} // GUI
} // Slic3r
