#include "DataCenter.hpp"
namespace DM{

struct DataCenter::priv
{
    nlohmann::json m_data;

    nlohmann::json m_current_device;
    DM::Device m_current_device_data;

    bool m_current_device_changed_state=false;
};

DataCenter::DataCenter():p(new priv)
{
}

void DataCenter::update_data(const nlohmann::json& datas)
{
    p->m_data = datas;

    DM::Device current_device_last = DM::Device::deserialize(p->m_current_device);
    unsigned long last_box_info_hash = -1, cur_box_info_hash = -1;
    get_device_box_info_hash(p->m_current_device, last_box_info_hash);

    p->m_current_device = this->_get_acive_device(); 

    // When the device box info is first changed, it is also regarded as a device change
    get_device_box_info_hash(p->m_current_device, cur_box_info_hash); 
    bool box_info_hash_changed = cur_box_info_hash != -1 && last_box_info_hash == -1;  

    p->m_current_device_data = DM::Device::deserialize(p->m_current_device);
    set_cur_active_address(p->m_current_device_data.address);

    p->m_current_device_changed_state = box_info_hash_changed || p->m_current_device_data.address != current_device_last.address;
}

// return empty string or hashBoxsInfo
bool DataCenter::get_device_box_info_hash(nlohmann::json& device_json, unsigned long& hash)
{ 
    bool ret = false;
    if (device_json.contains("boxsInfo") && device_json["boxsInfo"].contains("hashBoxsInfo")) {
        hash = device_json["boxsInfo"]["hashBoxsInfo"].get<int>();
        ret  = true;
    }
    return ret;
}

void DataCenter::set_cur_active_address(std::string& address)
{ 
    if (p->m_data.contains("data"))
        p->m_data["data"]["currentActivePrinterAddress"] = address; 
}

bool DataCenter::is_current_device_changed()
{
    return p->m_current_device_changed_state;
}

const nlohmann::json DataCenter::GetData()
{
    return p->m_data;
}

const nlohmann::json& DataCenter::get_data()
{ 
    return p->m_data; 
}

nlohmann::json DataCenter::find_printer_by_mac(const std::string& device_mac)
{
    try {

        nlohmann::json printerData;
        if(p->m_data.contains("data") && p->m_data["data"].contains("printerList"))
        {
            printerData = p->m_data["data"];
        }
        else
        {
            return nullptr;
        }

        if (!printerData.contains("printerList")) {
            return nullptr;
        }

        nlohmann::json foundPrinter = nullptr;

        for (const auto& group : printerData["printerList"]) 
        {
            if (!group.contains("list"))
            {
                continue;
            }

            for (const auto& printer : group["list"]) 
            {
                if(!printer.contains("mac") || !printer.contains("deviceType"))
                {
                    continue;
                }

                if (printer["mac"] == device_mac) {
                    if (printer["deviceType"] == 0 && printer["online"].get<bool>()) {
                        return printer; // 优先返回 deviceType 为 0 的打印机
                    } else if (printer["deviceType"] == 1) {
                        foundPrinter = printer; // 记录 deviceType 为 1 的打印机
                    }
                }
            }
        }

        return foundPrinter; // 如果没有找到 deviceType 为 0 的打印机，返回 deviceType 为 1 的打印机
    } 
    catch (std::exception& e) 
    {
        return nullptr;
    }

}

nlohmann::json DataCenter::get_current_device()
{
    return p->m_current_device;
}

const DM::Device&DataCenter::get_current_device_data()
{
    return p->m_current_device_data;
}

bool DataCenter::set_current_device(const std::string& device_mac, const nlohmann::json& fallback_device)
{
    DM::Device current_device_last = p->m_current_device_data;
    unsigned long last_box_info_hash = -1, cur_box_info_hash = -1;
    get_device_box_info_hash(p->m_current_device, last_box_info_hash);

    if (p->m_data.contains("data")) {
        p->m_data["data"]["currentActivePrinterMac"] = device_mac;
    }

    nlohmann::json next_device = find_printer_by_mac(device_mac);
    if ((next_device.is_null() || next_device.empty()) && fallback_device.is_object() && !fallback_device.empty()) {
        next_device = fallback_device;
    }

    if (next_device.is_null() || next_device.empty()) {
        p->m_current_device = nullptr;
        p->m_current_device_data = DM::Device();
        p->m_current_device_changed_state = current_device_last.valid;
        if (p->m_data.contains("data")) {
            p->m_data["data"]["currentActivePrinterAddress"] = "";
        }
        return false;
    }

    if (!device_mac.empty()) {
        next_device["mac"] = device_mac;
    }
    next_device["isCurrentDevice"] = true;

    p->m_current_device = next_device;

    get_device_box_info_hash(p->m_current_device, cur_box_info_hash);
    bool box_info_hash_changed = cur_box_info_hash != -1 && last_box_info_hash == -1;

    p->m_current_device_data = DM::Device::deserialize(p->m_current_device);
    p->m_current_device_data.isCurrentDevice = true;
    set_cur_active_address(p->m_current_device_data.address);

    p->m_current_device_changed_state = box_info_hash_changed ||
        p->m_current_device_data.address != current_device_last.address ||
        p->m_current_device_data.mac != current_device_last.mac;

    return p->m_current_device_data.valid;
}

DM::Device DataCenter::get_printer_data(std::string address)
{
    DM::Device data;
    try {

        nlohmann::json printerData;
        if (p->m_data.contains("data") && p->m_data["data"].contains("printerList"))
        {
            printerData = p->m_data["data"];
        }
        else
        {
            return data;
        }

        if (!printerData.contains("printerList")) {
            return data;
        }

        for (const auto& group : printerData["printerList"])
        {
            if (!group.contains("list"))
            {
                continue;
            }

            for (const auto& printer : group["list"])
            {
                if (!printer.contains("address"))
                {
                    continue;
                }

                if (printer["address"] == address) {
                    nlohmann::json printer_tmp = nlohmann::json(printer);

                     return DM::Device::deserialize(printer_tmp);
                }
            }
        }
    }
    catch (std::exception& e)
    {
        return data;
    }

    return data;
}

bool DataCenter::DeviceHasBoxColor(std::string address)
{
    DM::Device data = get_printer_data(address);
    if(data.valid){
        return data.boxColorInfos.size()>0;
    }

    return false;
}

// if current device is a cx Cloud device and a local device:
//      if the local device is online, return to the local device; otherwise, return to the cloud device
// If the current device is not both a cloud device and a local device:
//      just return directly
nlohmann::json DataCenter::_get_acive_device()
{
    nlohmann::json device_local(nullptr), device_cxy(nullptr), device_fluidd(nullptr);
    try
    {
        if (p->m_data.contains("data") && p->m_data["data"].contains("currentActivePrinterMac") && p->m_data["data"].contains("printerList"))
        {
            std::string mac = p->m_data["data"]["currentActivePrinterMac"].get<std::string>();
            for (const auto& group : p->m_data["data"]["printerList"])
            {
                if (!group.contains("list"))
                {
                    continue;
                }

                for (const auto& printer : group["list"])
                {
                    if (!printer.contains("mac") || !printer.contains("deviceType"))
                    {
                        continue;
                    }

                    if (printer["mac"] == mac) {
                        if (printer["deviceType"] == 0/* && printer["online"].get<bool>()*/) { // now support bundle offline device
                            device_local = printer;
                        }
                        else if (printer["deviceType"] == 1) {
                            device_cxy = printer;
                        } else if (printer["deviceType"] == 1001) 
                        {
                            device_fluidd = printer;
                        }

                        if (!device_local.empty() && !device_cxy.empty())
                            break;
                    }
                }
            }
        }
    }
    catch (std::exception& e)
    {
        return nullptr;
    }

    if (!device_local.empty() && !device_cxy.empty())
    {
        return device_local["online"].get<bool>() ? device_local : device_cxy;
    }
    else
    {
        if (device_local.empty() && device_cxy.empty() && !device_fluidd.empty()) 
        {
            return device_fluidd;
        }

        if (device_local.empty()) 
        {
            return device_cxy;
        }

        return device_local;
    }
}

DataCenter& DataCenter::Ins()
{
    static DataCenter obj;
    return obj;
}
}