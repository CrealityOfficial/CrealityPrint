#ifndef RemotePrint_DeviceDB_hpp_
#define RemotePrint_DeviceDB_hpp_
#include <cstdint>
#include <string>

namespace RemotePrint {
class DeviceDB
{
public:

    // according to the device return data structure
    struct Material
    {
        int         material_id;
        std::string vendor;
        std::string type;
        std::string name;
        std::string rfid;
        std::string color;  // #RRGGBB
        double      diameter;
        int         minTemp;
        int         maxTemp;
        double      pressure;
        int         percent;
        int         state;
        int         selected;
        int         editStatus;

        bool operator==(const Material& other) const;
        bool operator!=(const Material& other) const;
    };

    struct MaterialBox
    {
        int box_id;
        int box_state;
        int box_type;
        int temp;
        int humidity;
        std::vector<Material> materials;

        bool operator==(const MaterialBox& other) const;
        bool operator!=(const MaterialBox& other) const;

    };

    struct DeviceBoxColorInfo
    {
        int boxType = 0;  // 0: normal multi-color box,  1: extra box
        std::string color;
        int boxId;
        int materialId;
        std::string filamentType;
        std::string filamentName;
        std::string cId;
    };

    struct Data
    {
        bool online;
        bool isCurrentDevice = false;
        bool isRelateToAccount = false;
        bool webrtcSupport = false;
        int deviceState;
        int deviceType = 0;  //0==local, 1==cx cloud,
        std::string model;
        std::string mac;
        std::string address;
        std::string name;
        std::string group;
        std::string tbId;
        std::string modelName;
        
        std::vector<DeviceBoxColorInfo> boxColorInfos;
        std::vector<MaterialBox> materialBoxes;
    };

    std::vector<Data> devices;

    static DeviceDB& getInstance()
    {
        std::call_once(flag, []() { instance.reset(new DeviceDB()); });
        return *instance;
    }

    static void destroyInstance() { instance.reset(); }

    ~DeviceDB();

public:
    void        AddDevice(Data& data);
    bool        DeviceHasBoxColor(std::string deviceIP);
    Data        get_printer_data(std::string deviceIP);
    void        load_current_device();
    void        save_current_device(const std::string& device_id);
    void        save_current_device_name(const std::string& device_name);
    void        save_current_device_model_name(const std::string& model_name) { m_current_device_model_name = model_name; };
    std::string get_current_device_id() { return m_current_device_id; }
    std::string get_account_bind_device_json(const std::vector<std::string>& device_macs);
    std::string get_current_device_name();
    std::string get_current_device_model_name() { return m_current_device_model_name; }

private:
    DeviceDB();

    DeviceDB(const DeviceDB&)            = delete;
    DeviceDB& operator=(const DeviceDB&) = delete;

    static std::unique_ptr<DeviceDB> instance;
    static std::once_flag            flag;

    std::string m_current_device_id;  // (currently use mac as device id) may not involved with account
    std::string m_current_device_name;  // for displaying on top left
    std::string m_current_device_model_name;  // for checking with preset match
};

bool compareMaterials(const DeviceDB::Material& a, const DeviceDB::Material& b);
bool compareMaterialBoxes(const DeviceDB::MaterialBox& a, const DeviceDB::MaterialBox& b);
bool findAndCompareMaterialBoxes(const std::vector<DeviceDB::MaterialBox>& boxesA, const DeviceDB::MaterialBox& boxB);

} // namespace RemotePrint

#endif /* RemotePrint_DeviceDB_hpp_ */
