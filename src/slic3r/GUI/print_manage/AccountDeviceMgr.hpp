#ifndef ACCOUNT_DEVICE_MANAGER_hpp_
#define ACCOUNT_DEVICE_MANAGER_hpp_
#include <cstdint>
#include <string>
#include <unordered_map>
#include <mutex>


class AccountDeviceMgr
{
public:
    struct DeviceInfo
    {
        std::string device_unique_id;    // device id(可能是mac地址)
        std::string address;             // ip地址
        int         connectType = 3;         // 连接类型
        std::string mac;                 // mac地址
        std::string model;               // 打印机型号
        std::string name;                // 打印机名称
        std::string group;               // 打印机分组
        // 其他设备信息
    };

    struct AccountInfo
    {
        std::string             account_id; // 账号id
        std::vector<DeviceInfo> my_devices;
        // 其他账户信息
    };

    struct AccountDeviceInfo
    {
        std::unordered_map<std::string, AccountInfo> account_infos;
    };

    static AccountDeviceMgr& getInstance()
    {
        std::call_once(flag, []() { instance.reset(new AccountDeviceMgr()); });
        return *instance;
    }

    static void destroyInstance() { instance.reset(); }

    static std::mutex& getFileMutex() { return file_mutex; }

    ~AccountDeviceMgr();

public:
    void unbind_device(const std::string& device_id);
    void add_to_my_devices(const DeviceInfo& device_info);
    void load();
    std::string get_account_device_bind_json_info();
    void        reset_account_device_file_id(const std::string& fid);
    std::string get_account_device_info_for_printers_init();

private:
    AccountDeviceMgr();

    AccountDeviceMgr(const AccountDeviceMgr&)            = delete;
    AccountDeviceMgr& operator=(const AccountDeviceMgr&) = delete;

    void clear_all_account_info();
    void add_device_to_account(AccountInfo& account, const DeviceInfo& device_info);
    void save_to_file();

    void        sync_to_cloud();
    std::string get_local_device_dir();
    std::string account_device_json_file();

    AccountDeviceInfo accountDeviceInfos;

    static std::unique_ptr<AccountDeviceMgr> instance;
    static std::once_flag            flag;

    std::string m_account_device_file_id = "";   //参数包唯一id

    static std::mutex file_mutex; // 静态互斥锁
};

#endif /* ACCOUNT_DEVICE_MANAGER_hpp_ */
