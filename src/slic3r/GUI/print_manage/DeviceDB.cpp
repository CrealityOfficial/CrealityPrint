#include "DeviceDB.hpp"

#include "slic3r/GUI/I18N.hpp"
#include "nlohmann/json.hpp"
#include "slic3r/GUI/GUI_App.hpp"
namespace pt = boost::property_tree;
using json = nlohmann::json;

// Define the static members
std::unique_ptr<RemotePrint::DeviceDB> RemotePrint::DeviceDB::instance;
std::once_flag RemotePrint::DeviceDB::flag;

namespace RemotePrint {

DeviceDB::DeviceDB()  
{

}

DeviceDB::~DeviceDB() 
{

}

DeviceDB::Data        DeviceDB::get_printer_data(std::string deviceIP)
{
    for (const auto& device : devices) {
        if (device.address == deviceIP) {
            return device;
        }
    }
    throw std::runtime_error("Device with IP " + deviceIP + " not found");
}

void DeviceDB::AddDevice(Data& data) 
{ 
    devices.push_back(data); 
}

bool DeviceDB::DeviceHasBoxColor(std::string deviceIP)
{
    for (const auto& device : devices) {
        if (device.address == deviceIP) {
            return device.boxColorInfos.size() > 0;
        }
    }
    return false;
}

void DeviceDB::save_current_device_name(const std::string& device_name)
{
    m_current_device_name = device_name;
}

std::string DeviceDB::get_current_device_name()
{
    return m_current_device_name;
}

void DeviceDB::save_current_device(const std::string& device_id)
{
    m_current_device_id = device_id;

    // 构建 JSON 对象
    json j;
    j["current_device"]["mac"] = device_id;

    // 获取文件路径
    std::filesystem::path device_file = (std::filesystem::path(Slic3r::data_dir()) / "current_device.json").make_preferred();

    // 写入 JSON 文件
    try {
        std::ofstream file(device_file);
        if (!file.is_open()) {
            return;
        }
        file << std::setw(4) << j << std::endl; // 格式化输出，缩进为 4 个空格
        file.close();
        std::cout << "Current device successfully saved to " << device_file.string() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error saving current device to JSON file: " << e.what() << std::endl;
        // throw;
        return;
    }
}

void DeviceDB::load_current_device()
{
        // 获取文件路径
    std::filesystem::path device_file = (std::filesystem::path(Slic3r::data_dir()) / "current_device.json").make_preferred();

    // 检查文件是否存在
    if (!std::filesystem::exists(device_file)) {
        std::cerr << "File does not exist: " << device_file.string() << std::endl;
        return;
    }

    // 读取 JSON 文件
    try {
        std::ifstream file(device_file);
        if (!file.is_open()) {
            std::cerr << "Unable to open file: " << device_file.string() << std::endl;
            return;
        }

        json j;
        file >> j;
        file.close();

        // 更新当前设备的 device_id
        if (j.contains("current_device") && j["current_device"].contains("mac")) {
            m_current_device_id = j["current_device"]["mac"].get<std::string>();
            std::cout << "Current device successfully loaded: " << m_current_device_id << std::endl;
        } else {
            std::cerr << "Invalid JSON format in file: " << device_file.string() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading current device from JSON file: " << e.what() << std::endl;
    }
}

std::string get_account_bind_device_json(const std::vector<std::string>& device_macs)
{
    if (!Slic3r::GUI::wxGetApp().is_user_login()) {
        return "";
    }
    return "";
}

// only used for compare in the param filament panel update
bool DeviceDB::Material::operator==(const DeviceDB::Material& other) const
{
    return  type == other.type &&  color == other.color;
}

// only used for compare in the param filament panel update
bool DeviceDB::Material::operator!=(const DeviceDB::Material& other) const 
{ 
    return !(*this == other); 
}

// only used for compare in the param filament panel update
bool DeviceDB::MaterialBox::operator==(const DeviceDB::MaterialBox& other) const
{
    return box_id == other.box_id  && materials == other.materials;
}

// only used for compare in the param filament panel update
bool DeviceDB::MaterialBox::operator!=(const DeviceDB::MaterialBox& other) const 
{ 
    return !(*this == other); 
}

bool compareMaterials(const DeviceDB::Material& a, const DeviceDB::Material& b)
{
    return a.type == b.type && a.color == b.color;
}

bool compareMaterialBoxes(const DeviceDB::MaterialBox& a, const DeviceDB::MaterialBox& b)
{
    if (a.box_id != b.box_id || a.box_type != b.box_type) 
    {
        return false;
    }

    if (a.materials.size() != b.materials.size()) {
        return false;
    }

    for (const auto& materialA : a.materials) {
        auto it = std::find_if(b.materials.begin(), b.materials.end(), 
            [&materialA](const DeviceDB::Material& materialB) {
                return materialA.material_id == materialB.material_id;
            });

        if (it == b.materials.end() || !compareMaterials(materialA, *it)) {
            return false;
        }
    }

    return true;
}

bool findAndCompareMaterialBoxes(const std::vector<DeviceDB::MaterialBox>& boxesA, const DeviceDB::MaterialBox& boxB)
{
    auto it = std::find_if(boxesA.begin(), boxesA.end(), 
        [&boxB](const DeviceDB::MaterialBox& boxA) {
            return boxA.box_id == boxB.box_id && boxA.box_type == boxB.box_type;
        });

    if (it == boxesA.end()) {
        return false;
    }

    return compareMaterialBoxes(*it, boxB);
}

} // namespace RemotePrint
