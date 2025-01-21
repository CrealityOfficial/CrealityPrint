#ifndef RemotePrint_DataCenter_hpp_
#define RemotePrint_DataCenter_hpp_
#include "nlohmann/json.hpp"
#include <string>

class DataCenter
{
public:
    DataCenter();
    void UpdateData(nlohmann::json&datas);
    const nlohmann::json GetData();

    nlohmann::json find_printer_by_mac(const std::string& device_mac);

public:
    static DataCenter&Ins();
    nlohmann::json m_data;
};

#endif /* RemotePrint_DeviceDB_hpp_ */
