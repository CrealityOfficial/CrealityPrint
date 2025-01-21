#include "DataCenter.hpp"

DataCenter::DataCenter()
{

}

void DataCenter::UpdateData(nlohmann::json& datas)
{
    m_data = datas;
}

const nlohmann::json DataCenter::GetData()
{
    return m_data;
}

nlohmann::json DataCenter::find_printer_by_mac(const std::string& device_mac)
{
    try {

        nlohmann::json printerData;
        if(m_data.contains("data") && m_data["data"].contains("printerList"))
        {
            printerData = m_data["data"];
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
                    if (printer["deviceType"] == 0) {
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

DataCenter& DataCenter::Ins()
{
    static DataCenter obj;
    return obj;
}
