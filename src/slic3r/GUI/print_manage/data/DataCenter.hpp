#ifndef DM_DataCenter_hpp_
#define DM_DataCenter_hpp_
#include "nlohmann/json.hpp"
#include "DataType.hpp"

namespace DM{

class DataCenter
{
public:
    DataCenter();
    void update_data(const nlohmann::json&datas);
    bool is_current_device_changed();
    const nlohmann::json GetData();
    const nlohmann::json& get_data();
    
    nlohmann::json find_printer_by_mac(const std::string& device_mac);
    nlohmann::json get_current_device();
    const DM::Device& get_current_device_data();
    bool set_current_device(const std::string& device_mac, const nlohmann::json& fallback_device = nlohmann::json());
    DM::Device get_printer_data(std::string address);
    bool DeviceHasBoxColor(std::string address);

private:
    nlohmann::json _get_acive_device();
    void set_cur_active_address(std::string& address);
    bool get_device_box_info_hash(nlohmann::json& device_json, unsigned long& hash);
    struct priv;
    std::unique_ptr<priv> p;
public:
    static DataCenter&Ins();
};
}
#endif /* RemotePrint_DeviceDB_hpp_ */
